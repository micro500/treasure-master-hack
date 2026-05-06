// run_corpus_tests.mjs — WASM corpus tester. Replays the per-impl TMTV banks
// (expansion, all_maps, wc_alg, chain) through every WASM impl in this dir.
//
// Impl-independent banks (key_schedule_*, decryption, checksum) are covered by
// src/bruteforce/js/js/run_corpus_tests.mjs against tm_core.js — no point re-running them
// once per WASM impl.
//
// Run with:  node run_corpus_tests.mjs [corpus_dir] [impl_filter] [bank_filter]
//   corpus_dir defaults to ../../emulator/test_outputs/
//   impl_filter substring-matches impl names (e.g. "ssse3", "tm_64_8")
//   bank_filter is comma list: expansion, all_maps, wc_alg, chain

import { readFileSync, existsSync, readdirSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import { createRequire } from 'module';

const require = createRequire(import.meta.url);
const __dirname = dirname(fileURLToPath(import.meta.url));

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

function printResult(label, r, total, suffix = '') {
    const tag = r.fail === 0 ? 'PASS' : 'FAIL';
    let line = `  ${label}: ${tag}  ${r.pass}/${total}`;
    if (r.fail > 0) line += `  (first_fail=${r.firstFail})`;
    if (suffix) line += suffix;
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

// ─── Per-impl bank runners ────────────────────────────────────────────────────

// 0x04 expansion: [4]key_be [4]data_be [128]buf_out, REC=136
function runExpansion(M, ctx, lf) {
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 136;
        const key  = ((recs[o]<<24)|(recs[o+1]<<16)|(recs[o+2]<<8)|recs[o+3]) >>> 0;
        const data = ((recs[o+4]<<24)|(recs[o+5]<<16)|(recs[o+6]<<8)|recs[o+7]) >>> 0;
        M.ccall('tm_wasm_init', null, ['number'], [key]);
        M.ccall('tm_wasm_test_expansion', null, ['number','number'], [data, ctx.buf128]);
        let ok = true;
        for (let k = 0; k < 128; k++) if (M.HEAPU8[ctx.buf128 + k] !== recs[o + 8 + k]) { ok = false; break; }
        if (ok) r.pass++; else r.noteFail(i);
    }
    return r;
}

// 0x07 all_maps: [4]key_be [4]data_be [128]buf_out, REC=136
function runAllMaps(M, ctx, lf) {
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 136;
        const key  = ((recs[o]<<24)|(recs[o+1]<<16)|(recs[o+2]<<8)|recs[o+3]) >>> 0;
        const data = ((recs[o+4]<<24)|(recs[o+5]<<16)|(recs[o+6]<<8)|recs[o+7]) >>> 0;
        M.ccall('tm_wasm_init', null, ['number'], [key]);
        M.ccall('tm_wasm_test_bruteforce_data', null, ['number','number'], [data, ctx.buf128]);
        let ok = true;
        for (let k = 0; k < 128; k++) if (M.HEAPU8[ctx.buf128 + k] !== recs[o + 8 + k]) { ok = false; break; }
        if (ok) r.pass++; else r.noteFail(i);
    }
    return r;
}

// 0x05 wc_alg: [2]seed_in [128]buf_in [2]seed_out [128]buf_out, REC=260, subtype=alg
function runWcAlg(M, ctx, lf, alg, tracksRng) {
    const r = new Result();
    const recs = lf.records;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * 260;
        const seedIn = (recs[o] << 8) | recs[o + 1];
        for (let k = 0; k < 128; k++) M.HEAPU8[ctx.buf128 + k] = recs[o + 2 + k];
        // seed is uint16 LE in WASM memory
        M.HEAPU8[ctx.seedPtr]     = seedIn & 0xFF;
        M.HEAPU8[ctx.seedPtr + 1] = (seedIn >>> 8) & 0xFF;
        M.ccall('tm_wasm_test_algorithm', null,
            ['number','number','number'], [alg, ctx.buf128, ctx.seedPtr]);
        const expSeed = (recs[o + 130] << 8) | recs[o + 131];
        const gotSeed = M.HEAPU8[ctx.seedPtr] | (M.HEAPU8[ctx.seedPtr + 1] << 8);
        let ok = true;
        for (let k = 0; k < 128; k++) if (M.HEAPU8[ctx.buf128 + k] !== recs[o + 132 + k]) { ok = false; break; }
        if (ok && tracksRng && gotSeed !== expSeed) ok = false;
        if (ok) r.pass++; else r.noteFail(i);
    }
    return r;
}

// 0x06 wc_alg_multi (chain): [2]seed_in [128]buf_in [N]alg_ids [2]seed_out [128]buf_out
function runChain(M, ctx, lf, n, tracksRng) {
    const r = new Result();
    const recs = lf.records;
    const REC = 260 + n;
    for (let i = 0; i < lf.recordCount; i++) {
        const o = i * REC;
        const seedIn = (recs[o] << 8) | recs[o + 1];
        for (let k = 0; k < 128; k++) M.HEAPU8[ctx.buf128 + k] = recs[o + 2 + k];
        for (let k = 0; k < n; k++) M.HEAPU8[ctx.algsPtr + k] = recs[o + 130 + k];
        M.HEAPU8[ctx.seedPtr]     = seedIn & 0xFF;
        M.HEAPU8[ctx.seedPtr + 1] = (seedIn >>> 8) & 0xFF;
        M.ccall('tm_wasm_test_algorithm_chain', null,
            ['number','number','number','number'],
            [ctx.algsPtr, n, ctx.buf128, ctx.seedPtr]);
        const seedOutOff = o + 130 + n;
        const bufOutOff  = seedOutOff + 2;
        const expSeed = (recs[seedOutOff] << 8) | recs[seedOutOff + 1];
        const gotSeed = M.HEAPU8[ctx.seedPtr] | (M.HEAPU8[ctx.seedPtr + 1] << 8);
        let ok = true;
        for (let k = 0; k < 128; k++) if (M.HEAPU8[ctx.buf128 + k] !== recs[bufOutOff + k]) { ok = false; break; }
        if (ok && tracksRng && gotSeed !== expSeed) ok = false;
        if (ok) r.pass++; else r.noteFail(i);
    }
    return r;
}

// ─── Driver ───────────────────────────────────────────────────────────────────

const corpusDir   = process.argv[2] || join(__dirname, '../../emulator/test_outputs/');
const implFilter  = process.argv[3] || '';
const bankFilter  = process.argv[4] || '';

const bankEnabled = (n) => !bankFilter || (',' + bankFilter + ',').includes(',' + n + ',');

console.log(`run_corpus_tests (wasm): corpus dir = ${corpusDir}`);
if (implFilter) console.log(`impl filter = ${implFilter}`);
if (bankFilter) console.log(`bank filter = ${bankFilter}`);
console.log('');

// Pre-load all corpora once (each impl re-uses them)
const expansionCorp = bankEnabled('expansion')
    ? loadTmtv(join(corpusDir, 'expansion.bin'), 0x04, -1, 136) : null;
const allMapsCorp = bankEnabled('all_maps')
    ? loadTmtv(join(corpusDir, 'all_maps.bin'), 0x07, -1, 136) : null;
const algCorp = [];
if (bankEnabled('wc_alg')) {
    for (let a = 0; a < 8; a++) {
        algCorp[a] = loadTmtv(join(corpusDir, `wc_alg_${a}.bin`), 0x05, a, 260);
    }
}
const CHAIN_LENS = [2, 3, 4, 8, 11, 16];
const chainCorp = [];
if (bankEnabled('chain')) {
    for (let k = 0; k < CHAIN_LENS.length; k++) {
        const n = CHAIN_LENS[k];
        chainCorp[k] = loadTmtv(join(corpusDir, `wc_alg_multi_${n}.bin`), 0x06, n, 260 + n);
    }
}

// Discover impls (any .js file with a sibling .wasm)
const allImpls = readdirSync(__dirname)
    .filter(f => f.endsWith('.js') && existsSync(join(__dirname, f.replace(/\.js$/, '.wasm'))))
    .map(f => f.replace(/\.js$/, ''))
    .filter(n => !implFilter || n.includes(implFilter));

if (allImpls.length === 0) {
    console.log('no impls found');
    process.exit(1);
}
console.log(`testing ${allImpls.length} impl(s):`, allImpls.join(', '), '\n');

for (const impl of allImpls) {
    const TmModule = require(`./${impl}.js`);
    const M = await TmModule();

    // tracks_rng_state requires init first
    M.ccall('tm_wasm_init', null, ['number'], [0]);
    const tracksRng = M.ccall('tm_wasm_tracks_rng_state', 'number', [], []) === 1;
    const tag = tracksRng ? '' : ' [no rng tracking]';
    console.log(`=== ${impl}${tag} ===`);

    const ctx = {
        buf128:  M._malloc(128),
        seedPtr: M._malloc(2),
        algsPtr: M._malloc(16),
    };

    if (expansionCorp) {
        const r = runExpansion(M, ctx, expansionCorp);
        printResult('expansion', r, expansionCorp.recordCount);
    }
    if (allMapsCorp) {
        const r = runAllMaps(M, ctx, allMapsCorp);
        printResult('all_maps', r, allMapsCorp.recordCount);
    }
    for (let a = 0; a < 8; a++) {
        if (!algCorp[a]) continue;
        const r = runWcAlg(M, ctx, algCorp[a], a, tracksRng);
        printResult(`alg ${a}`, r, algCorp[a].recordCount);
    }
    for (let k = 0; k < CHAIN_LENS.length; k++) {
        if (!chainCorp[k]) continue;
        const n = CHAIN_LENS[k];
        const r = runChain(M, ctx, chainCorp[k], n, tracksRng);
        printResult(`chain${n}`, r, chainCorp[k].recordCount);
    }

    M._free(ctx.buf128);
    M._free(ctx.seedPtr);
    M._free(ctx.algsPtr);
    console.log('');
}

console.log(`summary: ${totalFailures} test group(s) had failures`);
process.exit(totalFailures === 0 ? 0 : 1);
