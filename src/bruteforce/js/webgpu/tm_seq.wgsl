// WebGPU port of tm_seq.cl — hash reduction removed.
//
// Launch: global=(32, num_wg, 1), local=(32, 1, 1)
// Each workgroup processes 64 candidates sequentially.
// Result: 2 bytes per candidate, packed into result_data as u32s.
//   byte 0: carnival_flags | CHECKSUM_SENTINEL if carnival hit, else 0
//   byte 1: other_flags    | CHECKSUM_SENTINEL | OTHER_WORLD if other hit, else 0
//
// nibble_sel_buf: one u16 stored per u32 (zero-extended), one entry per map.
// map_rng_buf: byte array packed as u32 (little-endian), schedule_count * 2048 bytes.
// expansion_vals: 32 u32s, one per thread lane.

// -------------------------------------------------------------------
// Bindings
// -------------------------------------------------------------------

struct Uniforms {
    key:            u32,
    data_start:     u32,
    schedule_count: i32,
    chunk:          u32,
}

@group(0) @binding(0) var<storage, read_write> result_data:    array<u32>;
@group(0) @binding(1) var<storage, read>       map_rng_buf:    array<u32>;
@group(0) @binding(2) var<storage, read>       nibble_sel_buf: array<u32>;
@group(0) @binding(3) var<uniform>             uniforms:       Uniforms;
@group(0) @binding(4) var<storage, read>       expansion_vals: array<u32>;

// -------------------------------------------------------------------
// Workgroup shared memory
// -------------------------------------------------------------------

var<workgroup> lane_buf:           array<u32, 32>;
var<workgroup> decrypted_carnival: array<u32, 32>;
var<workgroup> decrypted_other:    array<u32, 32>;
// 64 candidates * 2 bytes = 128 bytes = 32 u32s
// candidate c: byte c*2 = low byte of local_results[c>>1] if c even, high byte if c odd
var<workgroup> local_results:      array<u32, 32>;

// -------------------------------------------------------------------
// Constants — packed as little-endian u32s from the original byte arrays
// -------------------------------------------------------------------

const carnival_world_data = array<u32, 32>(
    0x00000000u, 0x00000000u, 0x00000000u, 0x5E3D0000u,
    0x23C8A6A1u, 0x7C3F6ED7u, 0x9F1B46D2u, 0x9B5CD2ABu,
    0x30674332u, 0xF323A4A0u, 0x21EABF27u, 0x1A31130Fu,
    0x3439A115u, 0x6E52D2E4u, 0x43F6F7A6u, 0xD84128D1u,
    0xC5E155DCu, 0x84D4F549u, 0xAB901F52u, 0xC32AE426u,
    0x81AC59C2u, 0xC37A3558u, 0x04019A51u, 0xA7FBE2F5u,
    0x9A468BAEu, 0xDDFA4127u, 0x7E237263u, 0x0B5A441Bu,
    0xFA093C2Au, 0xA13C59A3u, 0x464F90F0u, 0xF4D7D19Eu,
);

const other_world_data = array<u32, 32>(
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000000u, 0xC168CA00u,
    0x04D24466u, 0x8681900Bu, 0xE2D2F4C7u, 0x0CE322F1u,
    0xFFFB54D9u, 0x7281CF0Au, 0x989A940Au, 0x80ABFFD3u,
    0x45B7E59Au, 0xF0D28F6Eu, 0xAEB3FF67u, 0x069CBB49u,
    0xA3494012u, 0x7B32DB9Au, 0xB95AA158u, 0x6E2D2B2Bu,
    0x1A1C9336u, 0xE4180352u, 0xBDC1B15Eu, 0x50F1FB44u,
);

const opcode_bytes_used = array<u32, 256>(
    1u,2u,0u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,2u,3u,3u,0u,
    3u,2u,0u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,2u,3u,3u,0u,
    1u,2u,0u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,2u,3u,3u,0u,
    1u,2u,0u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,2u,3u,3u,0u,
    2u,2u,2u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,0u,3u,0u,0u,
    2u,2u,2u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,3u,3u,3u,0u,
    2u,2u,2u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,2u,3u,3u,0u,
    2u,2u,2u,0u,2u,2u,2u,0u,1u,2u,1u,0u,3u,3u,3u,0u,
    2u,2u,0u,0u,2u,2u,2u,0u,1u,3u,1u,0u,2u,3u,3u,0u,
);

const opcode_type = array<u32, 256>(
     0u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 0u, 2u, 4u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 4u, 2u, 4u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 4u, 2u, 4u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 0u, 2u,16u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 4u, 2u, 4u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 0u, 2u,16u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 4u, 2u, 4u, 0u, 0u, 2u,
     4u, 0u, 4u, 2u, 0u, 0u, 0u, 2u, 0u, 4u, 0u, 2u, 0u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u, 2u, 0u, 2u, 2u,
     0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u,
     0u, 0u, 4u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 4u, 2u, 4u, 0u, 0u, 2u,
     0u, 0u, 4u, 2u, 0u, 0u, 0u, 2u, 0u, 0u, 8u, 2u, 0u, 0u, 0u, 2u,
    16u, 0u, 1u, 2u, 4u, 0u, 0u, 2u, 0u, 0u, 4u, 2u, 4u, 0u, 0u, 2u,
);

// -------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------

fn bswap32(x: u32) -> u32 {
    return ((x & 0xFFu) << 24u)
         | (((x >>  8u) & 0xFFu) << 16u)
         | (((x >> 16u) & 0xFFu) <<  8u)
         |  ((x >> 24u) & 0xFFu);
}

// Byte-wise wrapping add — no carry between bytes
fn byte_add(a: u32, b: u32) -> u32 {
    return (((a & 0x00FF00FFu) + (b & 0x00FF00FFu)) & 0x00FF00FFu)
         | (((a & 0xFF00FF00u) + (b & 0xFF00FF00u)) & 0xFF00FF00u);
}

// Byte-wise wrapping subtract — no borrow between bytes.
// Padding constants (0x00000100, 0x00010000) prevent borrow from propagating
// across the zero-gap between the two packed bytes in each half.
fn byte_sub(a: u32, b: u32) -> u32 {
    return (((a & 0x00FF00FFu) + 0x00000100u - (b & 0x00FF00FFu)) & 0x00FF00FFu)
         | (((a & 0xFF00FF00u) + 0x00010000u - (b & 0xFF00FF00u)) & 0xFF00FF00u);
}

// Read one byte from the map_rng storage buffer at byte index idx.
fn read_map_byte(idx: u32) -> u32 {
    return (map_rng_buf[idx >> 2u] >> ((idx & 3u) * 8u)) & 0xFFu;
}

// Read 4 bytes from map_rng as a little-endian u32 starting at byte_addr.
// Handles any alignment by reading two words when the address is not
// 4-byte aligned (which happens after alg 2/5 advances local_pos by 1).
fn read_map_u32_le(byte_addr: u32) -> u32 {
    let shift = (byte_addr & 3u) * 8u;
    if shift == 0u {
        return map_rng_buf[byte_addr >> 2u];
    }
    let w0 = map_rng_buf[byte_addr >> 2u];
    let w1 = map_rng_buf[(byte_addr >> 2u) + 1u];
    return (w0 >> shift) | (w1 << (32u - shift));
}

// Read one byte from a workgroup array at byte index idx.
fn wg_byte(buf: ptr<workgroup, array<u32, 32>>, idx: u32) -> u32 {
    return ((*buf)[idx >> 2u] >> ((idx & 3u) * 8u)) & 0xFFu;
}

// -------------------------------------------------------------------
// Algorithm step
//
// For cases 0/1/3/4: four bytes at p3..p0 = base+(124-ci*4)..base+(127-ci*4)
// form a naturally-aligned u32 in map_rng_buf.  We read one word and
// extract the bits we need, avoiding four separate byte reads.
//
// The word layout (little-endian storage): bits 7:0 = byte at p3,
//   bits 15:8 = p2, bits 23:16 = p1, bits 31:24 = p0.
// Therefore bswap32(word) reconstructs the u32 that the OpenCL cast
// `map_rng[p0] | (map_rng[p1]<<8) | ...` would produce.
//
// For cases 2/5: single byte at map_base+local_pos (arbitrary alignment).
// Neighbor value from lane+1 obtained via workgroup memory + barrier
// (replaces PTX shfl.sync.down).
// -------------------------------------------------------------------

fn alg_seq(cur_val: u32, algorithm_id: u32, local_pos: ptr<function, u32>,
           map_base: u32, code_index: u32) -> u32 {
    let base = map_base + *local_pos;
    var result = cur_val;

    switch algorithm_id {
        case 0u: {
            // rng[byte_lane] = bit7 of map byte for that lane
            let word = read_map_u32_le(base + 124u - code_index * 4u);
            let b0 = (word >> 31u) & 1u;
            let b1 = (word >> 23u) & 1u;
            let b2 = (word >> 15u) & 1u;
            let b3 = (word >>  7u) & 1u;
            let rng = b0 | (b1 << 8u) | (b2 << 16u) | (b3 << 24u);
            result = ((cur_val << 1u) & 0xFEFEFEFEu) | rng;
            *local_pos += 128u;
        }
        case 1u: {
            let rng = bswap32(read_map_u32_le(base + 124u - code_index * 4u));
            result = byte_add(cur_val, rng);
            *local_pos += 128u;
        }
        case 2u, 5u: {
            // Shuffle down: get cur_val from lane+1, zero for lane 31.
            // lane_buf[code_index] = cur_val was already written and
            // workgroupBarrier() already called in run_one_map_seq before
            // this call, so we can read directly.
            let neighbor = select(0u, lane_buf[code_index + 1u], code_index < 31u);

            let carry_byte = read_map_byte(map_base + *local_pos);

            if algorithm_id == 2u {
                let carry = select(
                    (neighbor & 1u) << 24u,
                    ((carry_byte >> 7u) & 1u) << 24u,
                    code_index == 31u);
                result = ((cur_val & 0x00010000u) >> 8u) | carry
                       | ((cur_val >> 1u) & 0x007F007Fu)
                       | ((cur_val << 1u) & 0xFE00FE00u)
                       | ((cur_val >> 8u) & 0x00800080u);
            } else {
                let carry = select(
                    (neighbor & 0x80u) << 24u,
                    (carry_byte & 0x80u) << 24u,
                    code_index == 31u);
                result = ((cur_val & 0x00800000u) >> 8u) | carry
                       | ((cur_val >> 1u) & 0x7F007F00u)
                       | ((cur_val << 1u) & 0x00FE00FEu)
                       | ((cur_val >> 8u) & 0x00010001u);
            }
            *local_pos += 1u;
        }
        case 3u: {
            let rng = bswap32(read_map_u32_le(base + 124u - code_index * 4u));
            result = cur_val ^ rng;
            *local_pos += 128u;
        }
        case 4u: {
            let rng = bswap32(read_map_u32_le(base + 124u - code_index * 4u));
            result = byte_sub(cur_val, rng);
            *local_pos += 128u;
        }
        case 6u: {
            // Forward indexing; extract MSB of each byte → rng = word & 0x80808080
            let rng = read_map_u32_le(base + code_index * 4u) & 0x80808080u;
            result = ((cur_val >> 1u) & 0x7F7F7F7Fu) | rng;
            *local_pos += 128u;
        }
        case 7u: {
            result = cur_val ^ 0xFFFFFFFFu;
        }
        default: {}
    }

    return result;
}

// -------------------------------------------------------------------
// Map execution
// -------------------------------------------------------------------

fn run_one_map_seq(cur_val: u32, code_index: u32, map_idx: u32) -> u32 {
    var local_pos: u32 = 0u;
    let map_base = map_idx * 2048u;
    var nibble_selector = nibble_sel_buf[map_idx];
    var v = cur_val;

    for (var i = 0u; i < 16u; i++) {
        let src_lane   = i / 4u;
        let byte_shift = (i & 3u) * 8u;

        lane_buf[code_index] = v;
        workgroupBarrier();

        let current_byte = (lane_buf[src_lane] >> byte_shift) & 0xFFu;

        let nibble = (nibble_selector >> 15u) & 1u;
        nibble_selector = nibble_selector << 1u;

        let effective_byte = select(current_byte, current_byte >> 4u, nibble == 1u);
        let algorithm_id = (effective_byte >> 1u) & 7u;

        v = alg_seq(v, algorithm_id, &local_pos, map_base, code_index);
    }

    return v;
}

// -------------------------------------------------------------------
// Checksum and machine code validation (thread 0 only)
// -------------------------------------------------------------------

fn checksum_ok(data: ptr<workgroup, array<u32, 32>>, length: u32) -> bool {
    let checksum_total = (wg_byte(data, 127u - (length - 1u)) << 8u)
                       |  wg_byte(data, 127u - (length - 2u));
    if checksum_total > (length - 2u) * 255u { return false; }
    var sum = 0u;
    for (var i = 0u; i < length - 2u; i++) {
        sum += wg_byte(data, 127u - i);
    }
    return sum == checksum_total;
}

fn machine_code_flags(data: ptr<workgroup, array<u32, 32>>, code_length: u32,
                      e0: u32, e1: u32, e2: u32, e3: u32, e4: u32, e5: u32) -> u32 {
    var entry_addrs   = array<u32, 6>(e0, e1, e2, e3, e4, e5);
    var active_entries = array<u32, 6>(0u, 0u, 0u, 0u, 0u, 0u);
    var hit_entries    = array<u32, 6>(0u, 0u, 0u, 0u, 0u, 0u);
    var valid_entries  = array<u32, 6>(0u, 0u, 0u, 0u, 0u, 0u);
    var last_entry: i32 = -1;
    var result: u32 = 0u;
    var next_entry_addr = e0;

    var i = 0u;
    loop {
        if i >= code_length - 2u { break; }

        if i == next_entry_addr {
            last_entry++;
            hit_entries[last_entry]    = 1u;
            active_entries[last_entry] = 1u;
            next_entry_addr = entry_addrs[last_entry + 1];
        } else if i > next_entry_addr {
            last_entry++;
            next_entry_addr = entry_addrs[last_entry + 1];
        }

        let opcode = wg_byte(data, 127u - i);
        let otype  = opcode_type[opcode];

        if (otype & 0x01u) != 0u {  // OP_JAM
            result |= 0x80u;
            break;
        }
        if (otype & 0x02u) != 0u {  // OP_ILLEGAL
            result |= 0x40u;
            break;
        }
        result |= select(0u, 0x20u, (otype & 0x04u) != 0u);  // OP_NOP2
        result |= select(0u, 0x10u, (otype & 0x08u) != 0u);  // OP_NOP_OP
        if (otype & 0x10u) != 0u {  // OP_JUMP
            for (var j = 0; j < 5; j++) {
                if active_entries[j] == 1u {
                    active_entries[j] = 0u;
                    valid_entries[j]  = 1u;
                }
            }
        }

        i += opcode_bytes_used[opcode];
    }

    var all_entries_valid = true;
    for (var ei = 0; ei < 5; ei++) {
        if !all_entries_valid { break; }
        if hit_entries[ei] == 1u {
            if valid_entries[ei] != 1u {
                all_entries_valid = false;
            }
        } else if entry_addrs[ei] == 255u {
            continue;
        } else {
            var j = entry_addrs[ei];
            loop {
                if j >= code_length - 2u { break; }
                let opcode = wg_byte(data, 127u - j);
                let otype  = opcode_type[opcode];
                if (otype & 0x01u) != 0u || (otype & 0x02u) != 0u {
                    all_entries_valid = false;
                    break;
                } else if (otype & 0x10u) != 0u {
                    break;
                }
                j += opcode_bytes_used[opcode];
            }
        }
    }

    if all_entries_valid     { result |= 0x04u; }  // ALL_ENTRIES_VALID
    if valid_entries[0] == 1u { result |= 0x02u; }  // FIRST_ENTRY_VALID

    return result;
}

// -------------------------------------------------------------------
// Production kernel
// -------------------------------------------------------------------

@compute @workgroup_size(32, 1, 1)
fn tm_bruteforce_seq(
    @builtin(local_invocation_id) lid:  vec3<u32>,
    @builtin(workgroup_id)        wgid: vec3<u32>,
) {
    let code_index = lid.x;
    let group_id   = wgid.y;
    let data_base  = group_id * 64u;

    let my_expansion = expansion_vals[code_index];
    let schedule_count = u32(uniforms.schedule_count);
    let phase1_end = min(schedule_count, 5u);  // HASH_SPLIT_MAP = 5

    for (var local_cand = 0u; local_cand < 64u; local_cand++) {
        let data_idx = data_base + local_cand;
        let data_val = uniforms.data_start + data_idx;

        if data_idx >= uniforms.chunk {
            // Out-of-bounds: local_results already zero-initialized, nothing to write
            continue;
        }

        var working_val = select(bswap32(uniforms.data_start + data_idx),
                                 bswap32(uniforms.key),
                                 (code_index % 2u) == 0u);
        working_val = byte_add(working_val, my_expansion);

        for (var i = 0u; i < phase1_end; i++) {
            working_val = run_one_map_seq(working_val, code_index, i);
        }
        for (var i = phase1_end; i < schedule_count; i++) {
            working_val = run_one_map_seq(working_val, code_index, i);
        }

        decrypted_carnival[code_index] = working_val ^ carnival_world_data[code_index];
        decrypted_other[code_index]    = working_val ^ other_world_data[code_index];
        workgroupBarrier();

        if code_index == 0u {
            var carnival_stats: u32 = 0u;
            var other_stats:    u32 = 0u;

            if checksum_ok(&decrypted_carnival, 0x72u) {
                carnival_stats = machine_code_flags(&decrypted_carnival, 0x72u,
                                                    0u, 0x2Bu, 0x33u, 0x3Eu, 0xFFu, 0xFFu);
                carnival_stats |= 0x08u;  // CHECKSUM_SENTINEL
            }
            if checksum_ok(&decrypted_other, 0x53u) {
                other_stats = machine_code_flags(&decrypted_other, 0x53u,
                                                 0u, 0x05u, 0x0Au, 0x28u, 0x50u, 0xFFu);
                other_stats |= 0x08u | 0x01u;  // CHECKSUM_SENTINEL | OTHER_WORLD
            }

            // Pack two candidates' results into one u32 slot:
            // even local_cand → lower 16 bits, odd → upper 16 bits
            let word_idx = local_cand >> 1u;
            let shift    = (local_cand & 1u) * 16u;
            local_results[word_idx] |= (carnival_stats << shift) | (other_stats << (shift + 8u));
        }
        // No barrier here: only thread 0 writes local_results.
    }

    // One barrier so all thread-0 writes to local_results are visible before write-out.
    workgroupBarrier();

    // Coalesced 128-byte write: 32 threads each write one u32.
    result_data[group_id * 32u + code_index] = local_results[code_index];
}

// -------------------------------------------------------------------
// Test kernels — mirror tm_test_expand and tm_test_pipeline from the
// OpenCL version.  Both reuse the main kernel's bindings:
//   @binding(0) result_data  — output (32 u32s = 128 bytes)
//   @binding(1) map_rng_buf  — used by tm_test_pipeline only
//   @binding(2) nibble_sel_buf — used by tm_test_pipeline only
//   @binding(3) uniforms      — key / data_start (= test data value) / schedule_count
//   @binding(4) expansion_vals
//
// With WebGPU auto-layout each pipeline only requires the bindings it
// actually references, so tm_test_expand does not need @binding(1/2).
// -------------------------------------------------------------------

@compute @workgroup_size(32, 1, 1)
fn tm_test_expand(@builtin(local_invocation_id) lid: vec3<u32>) {
    let ci = lid.x;
    var val = select(bswap32(uniforms.data_start), bswap32(uniforms.key), (ci % 2u) == 0u);
    val = byte_add(val, expansion_vals[ci]);
    result_data[ci] = val;
}

@compute @workgroup_size(32, 1, 1)
fn tm_test_pipeline(@builtin(local_invocation_id) lid: vec3<u32>) {
    let ci = lid.x;
    var val = select(bswap32(uniforms.data_start), bswap32(uniforms.key), (ci % 2u) == 0u);
    val = byte_add(val, expansion_vals[ci]);
    let sc = u32(uniforms.schedule_count);
    for (var i = 0u; i < sc; i++) {
        val = run_one_map_seq(val, ci, i);
    }
    result_data[ci] = val;
}

// -------------------------------------------------------------------
// tm_test_wc_alg_multi — wc_alg / wc_alg_multi corpus harness.
// Inputs:
//   wc_alg_input_buf (binding 5): 32 u32s = 128 bytes initial buffer.
//   wc_alg_alg_ids   (binding 6): N alg ids (one per u32 slot).
//   map_rng_buf      (binding 1): 2048-byte RNG window built host-side
//                                 from seed_in. Treated as map at offset 0.
//   uniforms.schedule_count is repurposed as the alg count (1..16).
// Count = 1 covers the wc_alg bank; count > 1 covers wc_alg_multi.
// -------------------------------------------------------------------

@group(0) @binding(5) var<storage, read> wc_alg_input_buf: array<u32>;
@group(0) @binding(6) var<storage, read> wc_alg_alg_ids:   array<u32>;

@compute @workgroup_size(32, 1, 1)
fn tm_test_wc_alg_multi(@builtin(local_invocation_id) lid: vec3<u32>) {
    let ci = lid.x;
    let alg_count = u32(uniforms.schedule_count);
    var val = wc_alg_input_buf[ci];
    var local_pos: u32 = 0u;
    for (var i = 0u; i < alg_count; i++) {
        let alg_id = wc_alg_alg_ids[i];
        lane_buf[ci] = val;
        workgroupBarrier();
        val = alg_seq(val, alg_id, &local_pos, 0u, ci);
    }
    result_data[ci] = val;
}
