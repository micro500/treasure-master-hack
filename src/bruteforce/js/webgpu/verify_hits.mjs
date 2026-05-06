// Take GPU hits and verify each one's checksum via WASM pipeline
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

const KEY = 3055187383;  // 0xB61A75B7

// Full hit list from run_sample
import { readFileSync } from 'fs';
const txt = readFileSync('C:/Users/Peter/.claude/projects/N--prgm-treasure-master-hack/f7e1ef61-a077-49d8-ae9c-d8040f7fb562/tool-results/b3revz43p.txt', 'utf8');
const hits = [];
for (const line of txt.split('\n')) {
    const m = line.match(/0x([0-9a-f]{8})\s+flags=0x([0-9a-f]+)/);
    if (m) hits.push({ data: parseInt(m[1], 16), flags: parseInt(m[2], 16) });
}
console.log(`Loaded ${hits.length} hits`);

const TmModule = require('../wasm/tm_64_8.js');
const Module = await TmModule();
Module.ccall('tm_wasm_init', null, ['number'], [KEY]);

const CARNIVAL_LEN = 0x72;
const OTHER_LEN    = 0x53;

// u32 LE — same values as in tm_seq.wgsl and tm_seq.mjs
const CARNIVAL_U32 = new Uint32Array([
    0x00000000, 0x00000000, 0x00000000, 0x5E3D0000,
    0x23C8A6A1, 0x7C3F6ED7, 0x9F1B46D2, 0x9B5CD2AB,
    0x30674332, 0xF323A4A0, 0x21EABF27, 0x1A31130F,
    0x3439A115, 0x6E52D2E4, 0x43F6F7A6, 0xD84128D1,
    0xC5E155DC, 0x84D4F549, 0xAB901F52, 0xC32AE426,
    0x81AC59C2, 0xC37A3558, 0x04019A51, 0xA7FBE2F5,
    0x9A468BAE, 0xDDFA4127, 0x7E237263, 0x0B5A441B,
    0xFA093C2A, 0xA13C59A3, 0x464F90F0, 0xF4D7D19E,
]);
const OTHER_U32 = new Uint32Array([
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xC168CA00,
    0x04D24466, 0x8681900B, 0xE2D2F4C7, 0x0CE322F1,
    0xFFFB54D9, 0x7281CF0A, 0x989A940A, 0x80ABFFD3,
    0x45B7E59A, 0xF0D28F6E, 0xAEB3FF67, 0x069CBB49,
    0xA3494012, 0x7B32DB9A, 0xB95AA158, 0x6E2D2B2B,
    0x1A1C9336, 0xE4180352, 0xBDC1B15E, 0x50F1FB44,
]);
const CARNIVAL_WORLD = new Uint8Array(CARNIVAL_U32.buffer);
const OTHER_WORLD    = new Uint8Array(OTHER_U32.buffer);

function checksumOk(bytes128, length) {
    const total = (bytes128[127-(length-1)] << 8) | bytes128[127-(length-2)];
    if (total > (length-2)*255) return false;
    let sum = 0;
    for (let i = 0; i < length-2; i++) sum += bytes128[127-i];
    return sum === total;
}

const buf = Module._malloc(128);
let falsePos = 0, truePos = 0;
const fpExamples = [];

for (const { data, flags } of hits) {
    Module.ccall('tm_wasm_test_bruteforce_data', null, ['number','number'], [data, buf]);
    const out = Module.HEAPU8.slice(buf, buf+128);

    const decCarnival = new Uint8Array(128);
    const decOther    = new Uint8Array(128);
    for (let i = 0; i < 128; i++) {
        decCarnival[i] = out[i] ^ CARNIVAL_WORLD[i];
        decOther[i]    = out[i] ^ OTHER_WORLD[i];
    }

    const carnivalOk = checksumOk(decCarnival, CARNIVAL_LEN);
    const otherOk    = checksumOk(decOther,    OTHER_LEN);

    const gpuWorld = (flags & 0x01) ? 'other' : 'carnival';
    const cpuOk    = gpuWorld === 'carnival' ? carnivalOk : otherOk;

    if (!cpuOk) {
        falsePos++;
        if (fpExamples.length < 10) fpExamples.push({ data: data.toString(16), flags: flags.toString(16), gpuWorld });
    } else {
        truePos++;
    }
}

Module._free(buf);
console.log(`True positives:  ${truePos}`);
console.log(`False positives: ${falsePos}`);
if (fpExamples.length) {
    console.log('First false positives:');
    for (const e of fpExamples)
        console.log(`  0x${e.data}  flags=0x${e.flags}  world=${e.gpuWorld}`);
}
