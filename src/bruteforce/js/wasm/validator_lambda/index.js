import crypto from 'crypto';
import * as tm_lib from '/opt/nodejs/index.js';
import { S3Client, GetObjectCommand } from "@aws-sdk/client-s3";

var s3_client = new S3Client({ region: "us-east-1" });

const CHALLENGE_COUNT = 100;

let _tm_core = null;
async function get_tm_core() {
    if (!_tm_core) _tm_core = await import('./tm_core.mjs');
    return _tm_core;
}

async function reject_data(workunit_id, reason)
{
    await tm_lib.db.query(
        "update workunit set metadata = metadata || $1 where workunit_id = $2",
        [JSON.stringify({ rejected_at: new Date().toISOString(), rejection_reason: reason }), workunit_id],
        "store_rejected_at"
    );
    await tm_lib.workunits.set_workunit_status([workunit_id], 'rejected_data');
}

async function check_result(workunit_id)
{
    var uuid_verify = tm_lib.general.verify_uuid_format(workunit_id);

    if (!uuid_verify)
    {
        console.log("uuid invalid");
        return;
    }

    var wu_lookup = await tm_lib.workunits.get_workunit_detail_from_workunit_ids([workunit_id]);
    if (!wu_lookup || wu_lookup.length === 0)
    {
        console.log("workunit not found");
        return;
    }

    var wu = wu_lookup[0];
    console.log(wu);

    if (wu.status_code !== 'pending_data_validation')
    {
        console.log("not in status for data validation");
        return;
    }

    var task_metadata = wu.task_metadata;
    var range_start   = task_metadata.range_start;
    var workunit_size = wu.workunit_size;
    var seed          = wu.seed;

    var response;
    try
    {
        response = await s3_client.send(new GetObjectCommand({ Bucket: "tmhackpending", Key: wu.result_name }));
    }
    catch (e)
    {
        console.log("S3 error:", e);
        await reject_data(workunit_id, 's3_error');
        return;
    }

    const body_bytes = await response.Body.transformToByteArray();
    var buf = Buffer.from(body_bytes);

    // Parse result entries into map<lsb, flags>
    var result_count = buf.readUInt32LE(0);
    var result_map   = new Map();
    var offset       = 4;

    for (var i = 0; i < result_count; i++)
    {
        var lsb   = buf.readUInt32LE(offset);
        var flags = buf[offset + 4];
        result_map.set(lsb, flags);
        offset += 5;
    }

    // Read embedded SHA256 and check it against entry bytes
    var hash_bytes      = buf.subarray(offset, offset + 32);
    var result_hash_hex = hash_bytes.toString('hex');
    offset             += 32;

    var entry_bytes       = buf.subarray(4, 4 + result_count * 5);
    var computed_hash_hex = crypto.createHash('sha256').update(entry_bytes).digest('hex');
    var sha256_match      = computed_hash_hex === result_hash_hex;

    // Verify each result entry against ground-truth computation
    const { buildKeySchedule, buildExpansionValues, processFlags, process: tmProcess } = await get_tm_core();
    const chunk_key       = task_metadata.chunk_id;
    const schedule        = buildKeySchedule(chunk_key);
    const expansionValues = buildExpansionValues(chunk_key);

    var valid_entries      = 0;
    var invalid_entries    = 0;
    var invalid_capped     = false;
    var valid_flag_value_map = {};

    for (const [lsb, submitted_flags] of result_map)
    {
        const match = tmProcess(chunk_key, lsb, schedule, expansionValues);
        if (!match)
        {
            invalid_entries++;
            if (invalid_entries >= 100) { invalid_capped = true; break; }
            continue;
        }
        const expected_world_bit = match.world === 'carnival' ? 0 : 1;
        if ((submitted_flags & 0x01) !== expected_world_bit ||
            (submitted_flags & 0xFE) !== (match.flags & 0xFE))
        {
            invalid_entries++;
            if (invalid_entries >= 100) { invalid_capped = true; break; }
            continue;
        }
        valid_entries++;
        var key = String(submitted_flags);
        valid_flag_value_map[key] = (valid_flag_value_map[key] || 0) + 1;
    }

    // Re-derive challenge offsets and verify flags against ground-truth computation for all 100
    var x = crypto.createHash('sha256').update(result_hash_hex + seed).digest();
    var challenges_valid = 0;

    for (var i = 0; i < CHALLENGE_COUNT; i++)
    {
        var challenge_offset  = x.readUInt32BE(0) % workunit_size;
        var expected_lsb      = range_start + challenge_offset;
        var carnival_flags    = buf[offset + 4];
        var other_world_flags = buf[offset + 5];
        offset               += 6;

        x = crypto.createHash('sha256').update(x.toString('hex')).digest();

        const { carnival, other } = processFlags(chunk_key, expected_lsb, schedule, expansionValues);
        if ((carnival_flags & 0xFE) === (carnival & 0xFE) &&
            (other_world_flags & 0xFE) === (other & 0xFE))
        {
            challenges_valid++;
        }
    }

    // Determine rejection reason
    var rejection_reason = null;
    if (!sha256_match)          rejection_reason = 'sha256_mismatch';
    else if (invalid_entries > 0) rejection_reason = 'result_entry_invalid';
    else if (challenges_valid < CHALLENGE_COUNT) rejection_reason = 'challenge_flags_mismatch';

    if (rejection_reason)
    {
        console.log("data rejected:", rejection_reason);
        var stats = {
            sha256_match,
            valid_entries,
            invalid_entries: invalid_capped ? '100+' : invalid_entries,
            challenges_valid,
            flag_value_map: valid_flag_value_map,
        };
        await tm_lib.db.query(
            "update workunit set metadata = metadata || $1 where workunit_id = $2",
            [JSON.stringify({ data_validation_stats: stats, rejected_at: new Date().toISOString(), rejection_reason }), workunit_id],
            "store_data_rejection"
        );
        await tm_lib.workunits.set_workunit_status([workunit_id], 'rejected_data');
        await tm_lib.workunits.increment_user_fail_count(wu.user_id);
        return;
    }

    // Store flag value map and data_validated_at timestamp in metadata
    await tm_lib.db.query(
        "update workunit set metadata = metadata || $1 where workunit_id = $2",
        [JSON.stringify({ flag_value_map: valid_flag_value_map, data_validated_at: new Date().toISOString() }), workunit_id],
        "store_data_metadata"
    );

    console.log("data valid, finalizing");
    var outcome = await tm_lib.workunits.finalize_workunit_result(workunit_id, result_hash_hex);
    console.log("outcome:", outcome);
}

export const handler = async (event) => {

    console.log(event);
    console.log(event.Records);

    for (let r of event.Records)
    {
        console.log("VALIDATION_START", r.Sns.Message);
        await check_result(r.Sns.Message);
        console.log("VALIDATION_END", r.Sns.Message);
    }

};
