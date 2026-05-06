// run_corpus_tests.mjs — WebGPU corpus tester. Replays the per-impl TMTV
// banks (expansion, all_maps, wc_alg, wc_alg_multi) through the WebGPU impl.
//
// Impl-independent banks (key_schedule_*, decryption, checksum) are covered
// by src/bruteforce/js/js/run_corpus_tests.mjs against tm_core.js — skipped here.
//
// Run with:
//   node run_corpus_tests.mjs [corpus_dir] [bank_filter]
//   corpus_dir defaults to ../../emulator/test_outputs/
//   bank_filter is comma list: expansion, all_maps, wc_alg, wc_alg_multi

import { readFileSync, existsSync } from 'fs';
import { readFile } from 'fs/promises';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import { TmWebGPU } from './tm_seq.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));

// Node WebGPU shim (same as test_webgpu.mjs).
if (typeof navigator === 'undefined' || !navigator.gpu) {
    const { create, globals } = await import('webgpu');
    Object.assign(globalThis, globals);
    Object.defineProperty(globalThis, 'navigator',
        { value: { gpu: create([]) }, configurable: true });
}

// ─── TMTV file loading ────────────────────────────────────────────────────────

const TMTV_MAGIC = 'TMTV';

function loadTmtv(path, expectedTestType, expectedSubtype, expectedRecordSize) {
    if (!existsSync(path)) return null;
    const buf = readFileSync(path);
    if (buf.length < 32) return null;
    if (buf.slice(0, 4).toString('ascii') !== TMTV_MAGIC) return null;
    const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
    if (dv.getUint16(4, true) !== 1) return null;
    if (dv.getUint8(6) !== expectedTestType) return null;
    const subtype = dv.getUint8(7);
    if (expectedSubtype >= 0 && subtype !== expectedSubtype) return null;
    const recordSize  = dv.getUint16(8, true);
    if (expectedRecordSize !== 0 && recordSize !== expectedRecordSize) return null;
    const recordCount = dv.getUint32(10, true);
    if (dv.getUint8(18) !== 0) return null;
    const records = buf.subarray(32, 32 + recordCount * recordSize);
    if (records.length !== recordCount * recordSize) return null;
    return { records, recordCount, recordSize };
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

// ─── Bank runners ────────────────────────────────────────────────────────────

// 0x04 expansion: [4]key_be [4]data_be [128]buf_out, REC=136
async function runExpansion(tm, lf) {
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 136;
        const key  = beU32(recs, o);
        const data = beU32(recs, o + 4);
        tm.init(key);
        const out = await tm.testExpansion(data);
        if (eqBytes(out, 0, recs, o + 8, 128)) r.pass++; else r.noteFail(i);
    }
    return r;
}

// 0x07 all_maps: [4]key_be [4]data_be [128]buf_out, REC=136
async function runAllMaps(tm, lf) {
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 136;
        const key  = beU32(recs, o);
        const data = beU32(recs, o + 4);
        tm.init(key);
        const out = await tm.testPipeline(data);
        if (eqBytes(out, 0, recs, o + 8, 128)) r.pass++; else r.noteFail(i);
    }
    return r;
}

// 0x05 wc_alg: [2]seed_in [128]buf_in [2]seed_out [128]buf_out, REC=260
async function runWcAlg(tm, lf, alg) {
    const r = new Result();
    const recs = lf.records;
    const inp = new Uint8Array(128);
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 260;
        const seedIn = (recs[o] << 8) | recs[o + 1];
        for (let k = 0; k < 128; k++) inp[k] = recs[o + 2 + k];
        const out = await tm.testWcAlg(seedIn, alg, inp);
        // _map_-shaped impl: skip seed_out check, validate buffer only.
        if (eqBytes(out, 0, recs, o + 132, 128)) r.pass++; else r.noteFail(i);
    }
    return r;
}

// 0x06 wc_alg_multi: [2]seed_in [128]buf_in [N]alg_ids [2]seed_out [128]buf_out
async function runWcAlgMulti(tm, lf, n) {
    const r = new Result();
    const recs = lf.records;
    const REC  = 260 + n;
    const inp  = new Uint8Array(128);
    const algs = new Uint8Array(n);
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * REC;
        const seedIn = (recs[o] << 8) | recs[o + 1];
        for (let k = 0; k < 128; k++) inp[k]  = recs[o + 2 + k];
        for (let k = 0; k < n;   k++) algs[k] = recs[o + 130 + k];
        const out = await tm.testWcAlgMulti(seedIn, algs, inp);
        const bufOutOff = o + 130 + n + 2;
        if (eqBytes(out, 0, recs, bufOutOff, 128)) r.pass++; else r.noteFail(i);
    }
    return r;
}

// ─── Driver ───────────────────────────────────────────────────────────────────

const corpusDir  = process.argv[2] || join(__dirname, '../../emulator/test_outputs/');
const bankFilter = process.argv[3] || '';
const bankEnabled = (n) => !bankFilter || (',' + bankFilter + ',').includes(',' + n + ',');

console.log(`run_corpus_tests (webgpu): corpus dir = ${corpusDir}`);
if (bankFilter) console.log(`bank filter = ${bankFilter}`);
console.log('');

const shaderSource = await readFile(join(__dirname, 'tm_seq.wgsl'), 'utf8');
const tm = await TmWebGPU.create(shaderSource);

console.log('=== webgpu (tm_seq) [no rng tracking] ===');

if (bankEnabled('expansion')) {
    const lf = loadTmtv(join(corpusDir, 'expansion.bin'), 0x04, -1, 136);
    if (lf) {
        const r = await runExpansion(tm, lf);
        printResult('expansion', r, lf.recordCount);
    }
}
if (bankEnabled('all_maps')) {
    const lf = loadTmtv(join(corpusDir, 'all_maps.bin'), 0x07, -1, 136);
    if (lf) {
        const r = await runAllMaps(tm, lf);
        printResult('all_maps', r, lf.recordCount);
    }
}
if (bankEnabled('wc_alg')) {
    for (let a = 0; a < 8; a++) {
        const lf = loadTmtv(join(corpusDir, `wc_alg_${a}.bin`), 0x05, a, 260);
        if (!lf) continue;
        const r = await runWcAlg(tm, lf, a);
        printResult(`alg ${a}`, r, lf.recordCount);
    }
}
if (bankEnabled('wc_alg_multi')) {
    for (const n of [2, 3, 4, 8, 11, 16]) {
        const lf = loadTmtv(join(corpusDir, `wc_alg_multi_${n}.bin`), 0x06, n, 260 + n);
        if (!lf) continue;
        const r = await runWcAlgMulti(tm, lf, n);
        printResult(`wc_alg_multi${n}`, r, lf.recordCount);
    }
}

tm.destroy();
console.log(`\nsummary: ${totalFailures} test group(s) had failures`);
process.exit(totalFailures === 0 ? 0 : 1);
