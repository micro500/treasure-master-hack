// run_corpus_tests.mjs — Replays the TMTV (TM Test Vector) corpus through
// tm_core.js. Mirrors src/bruteforce/ref_test/ref_test.cpp banks:
//
//   key_schedule_alg (×8), key_schedule_dispatch, key_schedule_all_maps,
//   decryption (×2), checksum (×2), wc_alg (×8), chain (×6),
//   expansion, all_maps
//
// Run with:  node src/bruteforce/js/js/run_corpus_tests.mjs [corpus_dir]
// Default corpus_dir = N:/prgm/treasure-master-hack/src/bruteforce/emulator/test_outputs/

import { readFileSync, existsSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
import {
    rngTable,
    buildKeySchedule,
    expand,
    runAllMaps,
    runAlg,
    decrypt,
    scheduleTestAlg,
    scheduleDispatchAlg,
    CARNIVAL_WORLD_DATA, OTHER_WORLD_DATA,
} from './tm_core.js';

const CARNIVAL_WORLD_CODE_LENGTH = 0x72;
const OTHER_WORLD_CODE_LENGTH    = 0x53;

// ─── TMTV file loading ────────────────────────────────────────────────────────

const TMTV_MAGIC = 'TMTV';
const TMTV_VERSION = 1;
const RECORD_KIND_FULL = 0x00;

function loadTmtv(path, expectedTestType, expectedSubtype, expectedRecordSize) {
    if (!existsSync(path)) return null;
    const buf = readFileSync(path);
    if (buf.length < 32) { console.log(`  [skip] ${path}: short header`); return null; }
    const magic = buf.slice(0, 4).toString('ascii');
    if (magic !== TMTV_MAGIC) { console.log(`  [skip] ${path}: bad magic`); return null; }
    const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
    const version     = dv.getUint16(4, true);
    const testType    = dv.getUint8(6);
    const subtype     = dv.getUint8(7);
    const recordSize  = dv.getUint16(8, true);
    const recordCount = dv.getUint32(10, true);
    const recordKind  = dv.getUint8(18);
    if (version !== TMTV_VERSION) { console.log(`  [skip] ${path}: version ${version}`); return null; }
    if (testType !== expectedTestType) { console.log(`  [skip] ${path}: test_type 0x${testType.toString(16)} (expected 0x${expectedTestType.toString(16)})`); return null; }
    if (expectedSubtype >= 0 && subtype !== expectedSubtype) { console.log(`  [skip] ${path}: subtype ${subtype} (expected ${expectedSubtype})`); return null; }
    if (expectedRecordSize !== 0 && recordSize !== expectedRecordSize) { console.log(`  [skip] ${path}: record_size ${recordSize} (expected ${expectedRecordSize})`); return null; }
    if (recordKind !== RECORD_KIND_FULL) { console.log(`  [skip] ${path}: record_kind 0x${recordKind.toString(16)} (expected full)`); return null; }
    const records = buf.subarray(32, 32 + recordCount * recordSize);
    if (records.length !== recordCount * recordSize) { console.log(`  [skip] ${path}: short records`); return null; }
    return { records, recordCount, recordSize, subtype };
}

// ─── Result tracking ──────────────────────────────────────────────────────────

class Result {
    constructor() { this.pass = 0; this.fail = 0; this.firstFail = -1; this.failIdxs = []; }
    noteFail(i) { this.fail++; if (this.firstFail < 0) this.firstFail = i; this.failIdxs.push(i); }
}

let totalFailures = 0;

function printResult(label, r, total) {
    const tag = r.fail === 0 ? 'PASS' : 'FAIL';
    let line = `  ${label}: ${tag}  ${r.pass}/${total}`;
    if (r.fail > 0) line += `  (first_fail=${r.firstFail})`;
    console.log(line);
    if (r.fail > 0) {
        const idxs = r.failIdxs;
        let s = '    fail ranges:';
        let start = idxs[0], prev = idxs[0];
        for (let k = 1; k < idxs.length; k++) {
            const v = idxs[k];
            if (v === prev + 1) { prev = v; continue; }
            s += (start === prev) ? ` ${start}` : ` ${start}-${prev}`;
            start = prev = v;
        }
        s += (start === prev) ? ` ${start}` : ` ${start}-${prev}`;
        console.log(s);
        totalFailures++;
    }
}

function eqBytes(a, ao, b, bo, n) {
    for (let i = 0; i < n; i++) if (a[ao + i] !== b[bo + i]) return false;
    return true;
}

function beU32(buf, o) {
    return ((buf[o] << 24) | (buf[o + 1] << 16) | (buf[o + 2] << 8) | buf[o + 3]) >>> 0;
}

// ─── Banks ────────────────────────────────────────────────────────────────────

// 0x02 key_schedule_alg: [4]state_in [1]map [4]state_out, REC=9
function bankKeyScheduleAlg(corpusDir) {
    console.log('=== 0x02 key_schedule_alg ===');
    for (let alg = 0; alg < 8; alg++) {
        const lf = loadTmtv(join(corpusDir, `key_schedule_alg_${alg}.bin`), 0x02, alg, 9);
        if (!lf) continue;
        const r = new Result();
        const recs = lf.records;
        for (let i = 0; i < lf.recordCount; i++) {
            const o = i * 9;
            const out = scheduleTestAlg(recs.subarray(o, o + 4), alg, recs[o + 4]);
            if (eqBytes(out, 0, recs, o + 5, 4)) r.pass++; else r.noteFail(i);
        }
        printResult(`alg ${alg}`, r, lf.recordCount);
    }
    console.log('');
}

// 0x01 key_schedule_dispatch: [4]state_in [1]map [4]state_out [1]ran [1]routine_id, REC=11
function bankKeyScheduleDispatch(corpusDir) {
    console.log('=== 0x01 key_schedule_dispatch ===');
    const lf = loadTmtv(join(corpusDir, 'key_schedule_dispatch.bin'), 0x01, -1, 11);
    if (!lf) { console.log(''); return; }
    const stateR = new Result(), routineR = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 11;
        const map = recs[o + 4];
        const alg = scheduleDispatchAlg(recs.subarray(o, o + 4), map);
        const out = scheduleTestAlg(recs.subarray(o, o + 4), alg, map);
        if (eqBytes(out, 0, recs, o + 5, 4)) stateR.pass++; else stateR.noteFail(i);
        if (((alg + 0x27) & 0xFF) === recs[o + 10]) routineR.pass++; else routineR.noteFail(i);
    }
    printResult('state ', stateR, lf.recordCount);
    printResult('alg id', routineR, lf.recordCount);
    console.log('');
}

// 0x03 key_schedule_all_maps: [4]key_be [27×4]{rng1,rng2,nib_lo,nib_hi}, REC=112
function bankKeyScheduleAllMaps(corpusDir) {
    console.log('=== 0x03 key_schedule_all_maps ===');
    const lf = loadTmtv(join(corpusDir, 'key_schedule_all_maps.bin'), 0x03, -1, 112);
    if (!lf) { console.log(''); return; }
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 112;
        const key = beU32(recs, o);
        const sched = buildKeySchedule(key);
        let ok = (sched.length === 27);
        for (let e = 0; e < 27 && ok; e++) {
            const ent = sched[e];
            const exp0 = (ent.seed >>> 8) & 0xFF;
            const exp1 = ent.seed & 0xFF;
            const exp2 = ent.nibbleSelector & 0xFF;
            const exp3 = (ent.nibbleSelector >>> 8) & 0xFF;
            const ro = o + 4 + e * 4;
            if (recs[ro] !== exp0 || recs[ro+1] !== exp1 || recs[ro+2] !== exp2 || recs[ro+3] !== exp3) ok = false;
        }
        if (ok) r.pass++; else r.noteFail(i);
    }
    printResult('schedule', r, lf.recordCount);
    console.log('');
}

// 0x08 decryption: [128]keystream [N]decrypted, decrypted[i]=keystream[127-i]^world[127-i]
function bankDecryption(corpusDir) {
    console.log('=== 0x08 decryption ===');
    const files = [
        { fname: 'decrypt0.bin', entry: 0, N: CARNIVAL_WORLD_CODE_LENGTH, world: CARNIVAL_WORLD_DATA },
        { fname: 'decrypt1.bin', entry: 1, N: OTHER_WORLD_CODE_LENGTH,    world: OTHER_WORLD_DATA    },
    ];
    for (const s of files) {
        const lf = loadTmtv(join(corpusDir, s.fname), 0x08, s.entry, 128 + s.N);
        if (!lf) continue;
        const r = new Result();
        const recs = lf.records;
        const REC = 128 + s.N;
        for (let i = 0; i < lf.recordCount; i++) {
            const o = i * REC;
            let ok = true;
            for (let b = 0; b < s.N; b++) {
                const expected = recs[o + 127 - b] ^ s.world[127 - b];
                if (expected !== recs[o + 128 + b]) { ok = false; break; }
            }
            if (ok) r.pass++; else r.noteFail(i);
        }
        printResult(`entry ${s.entry}`, r, lf.recordCount);
    }
    console.log('');
}

// 0x09 checksum: [N]buffer [2]computed [2]stored [1]match
function bankChecksum(corpusDir) {
    console.log('=== 0x09 checksum ===');
    const files = [
        { fname: 'checksum0.bin', entry: 0, N: CARNIVAL_WORLD_CODE_LENGTH },
        { fname: 'checksum1.bin', entry: 1, N: OTHER_WORLD_CODE_LENGTH    },
    ];
    for (const s of files) {
        const lf = loadTmtv(join(corpusDir, s.fname), 0x09, s.entry, s.N + 5);
        if (!lf) continue;
        const r = new Result();
        const recs = lf.records;
        const REC = s.N + 5;
        for (let i = 0; i < lf.recordCount; i++) {
            const o = i * REC;
            let sum = 0;
            for (let b = 0; b < s.N - 2; b++) sum = (sum + recs[o + b]) & 0xFFFF;
            const stored = (recs[o + s.N - 1] << 8) | recs[o + s.N - 2];
            const expComputed = recs[o + s.N] | (recs[o + s.N + 1] << 8);
            const expStored   = recs[o + s.N + 2] | (recs[o + s.N + 3] << 8);
            const expMatch    = recs[o + s.N + 4];
            const ok = (sum === expComputed) && (stored === expStored) &&
                       (((sum === stored ? 1 : 0)) === expMatch);
            if (ok) r.pass++; else r.noteFail(i);
        }
        printResult(`entry ${s.entry}`, r, lf.recordCount);
    }
    console.log('');
}

// 0x05 wc_alg: [2]seed_in [128]buf_in [2]seed_out [128]buf_out, REC=260, subtype=alg
function bankWcAlg(corpusDir) {
    console.log('=== 0x05 wc_alg ===');
    const buf = new Uint8Array(128);
    for (let alg = 0; alg < 8; alg++) {
        const lf = loadTmtv(join(corpusDir, `wc_alg_${alg}.bin`), 0x05, alg, 260);
        if (!lf) continue;
        const r = new Result();
        const recs = lf.records;
        for (let i = 0; i < lf.recordCount; i++) {
            const o = i * 260;
            const seedIn = (recs[o] << 8) | recs[o + 1];
            for (let k = 0; k < 128; k++) buf[k] = recs[o + 2 + k];
            const seedOut = runAlg(rngTable, buf, alg, seedIn);
            const expSeed = (recs[o + 130] << 8) | recs[o + 131];
            const ok = (seedOut === expSeed) && eqBytes(buf, 0, recs, o + 132, 128);
            if (ok) r.pass++; else r.noteFail(i);
        }
        printResult(`alg ${alg}`, r, lf.recordCount);
    }
    console.log('');
}

// 0x06 wc_alg_multi (chain): [2]seed_in [128]buf_in [N]alg_ids [2]seed_out [128]buf_out
function bankChain(corpusDir) {
    console.log('=== 0x06 wc_alg_multi (chain) ===');
    const lens = [2, 3, 4, 8, 11, 16];
    const buf = new Uint8Array(128);
    for (const n of lens) {
        const REC = 260 + n;
        const lf = loadTmtv(join(corpusDir, `wc_alg_multi_${n}.bin`), 0x06, n, REC);
        if (!lf) continue;
        const r = new Result();
        const recs = lf.records;
        for (let i = 0; i < lf.recordCount; i++) {
            const o = i * REC;
            let seed = (recs[o] << 8) | recs[o + 1];
            for (let k = 0; k < 128; k++) buf[k] = recs[o + 2 + k];
            for (let j = 0; j < n; j++) seed = runAlg(rngTable, buf, recs[o + 130 + j], seed);
            const seedOutOff = o + 130 + n;
            const bufOutOff  = seedOutOff + 2;
            const expSeed = (recs[seedOutOff] << 8) | recs[seedOutOff + 1];
            const ok = (seed === expSeed) && eqBytes(buf, 0, recs, bufOutOff, 128);
            if (ok) r.pass++; else r.noteFail(i);
        }
        printResult(`chain${n}`, r, lf.recordCount);
    }
    console.log('');
}

// 0x04 expansion: [4]key_be [4]data_be [128]buf_out, REC=136
function bankExpansion(corpusDir) {
    console.log('=== 0x04 expansion ===');
    const lf = loadTmtv(join(corpusDir, 'expansion.bin'), 0x04, -1, 136);
    if (!lf) { console.log(''); return; }
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 136;
        const key  = beU32(recs, o);
        const data = beU32(recs, o + 4);
        const code = expand(rngTable, key, data);
        if (eqBytes(code, 0, recs, o + 8, 128)) r.pass++; else r.noteFail(i);
    }
    printResult('expansion', r, lf.recordCount);
    console.log('');
}

// 0x07 all_maps: [4]key_be [4]data_be [128]buf_out, REC=136
function bankAllMaps(corpusDir) {
    console.log('=== 0x07 all_maps ===');
    const lf = loadTmtv(join(corpusDir, 'all_maps.bin'), 0x07, -1, 136);
    if (!lf) { console.log(''); return; }
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 136;
        const key  = beU32(recs, o);
        const data = beU32(recs, o + 4);
        const code = expand(rngTable, key, data);
        runAllMaps(rngTable, code, buildKeySchedule(key));
        if (eqBytes(code, 0, recs, o + 8, 128)) r.pass++; else r.noteFail(i);
    }
    printResult('all_maps', r, lf.recordCount);
    console.log('');
}

// ─── Main ─────────────────────────────────────────────────────────────────────

const corpusDir = process.argv[2] ||
    join(__dirname, '../../emulator/test_outputs/');
console.log(`run_corpus_tests: corpus dir = ${corpusDir}\n`);

bankKeyScheduleAlg(corpusDir);
bankKeyScheduleDispatch(corpusDir);
bankKeyScheduleAllMaps(corpusDir);
bankDecryption(corpusDir);
bankChecksum(corpusDir);
bankWcAlg(corpusDir);
bankChain(corpusDir);
bankExpansion(corpusDir);
bankAllMaps(corpusDir);

console.log(`summary: ${totalFailures} test group(s) had failures`);
process.exit(totalFailures === 0 ? 0 : 1);
