// Compare JS CPU expansion table vs WASM expansion output
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

const RNG_TABLE = new Uint16Array(0x10000);
for (let i = 0; i <= 0xFF; i++) {
    for (let j = 0; j <= 0xFF; j++) {
        let a = i, b = j, carry;
        b = (b + a) & 0xFF;
        a = a + 0x89; carry = a > 0xFF ? 1 : 0; a &= 0xFF;
        b = b + 0x2A + carry; carry = b > 0xFF ? 1 : 0; b &= 0xFF;
        a = a + 0x21 + carry; carry = a > 0xFF ? 1 : 0; a &= 0xFF;
        b = b + 0x43 + carry; b &= 0xFF;
        RNG_TABLE[(i << 8) | j] = (a << 8) | b;
    }
}
function rngNext(seed) { return RNG_TABLE[seed & 0xFFFF]; }
function rngByte(seed) { const n = rngNext(seed); return ((n >>> 8) ^ n) & 0xFF; }

// CPU expansion table: t[s*128+i] = rngByte at step (127-i) from seed s
const key = 0x2CA5B42D;
const data = 0x0009BE9F;
const rng_seed = (key >>> 16) & 0xFFFF;  // 0x2CA5

// Compute first 16 entries of expansion table for seed 0x2CA5
let seed = rng_seed;
const fwd = new Uint8Array(128);
for (let j = 0; j < 128; j++) {
    fwd[j] = rngByte(seed);
    seed = rngNext(seed);
}
// expansion[i] = fwd[127-i]
console.log('CPU expansion bytes for seed 0x' + rng_seed.toString(16) + ':');
for (let i = 0; i < 16; i++) {
    console.log(`  [${i}] = 0x${fwd[127-i].toString(16).padStart(2,'0')} (step ${127-i})`);
}

// GPU expansion (cumulative sum)
const gpuExp = new Uint8Array(128);
{
    let s = rng_seed;
    const out = new Uint8Array(128);
    for (let i = 0; i < 128; i++) {
        const next = rngNext(s);
        out[i] = ((next >>> 8) ^ next) & 0xFF;
        s = next;
    }
    for (let lane = 0; lane < 32; lane++) {
        for (let byteIdx = 0; byteIdx < 4; byteIdx++) {
            const b = lane * 4 + byteIdx;
            const j = (b / 8) | 0;
            const k = b % 8;
            let accum = 0;
            for (let i = 0; i < j; i++) accum += out[k + i * 8];
            gpuExp[b] = accum & 0xFF;
        }
    }
}
console.log('\nGPU expansion bytes:');
for (let i = 0; i < 16; i++) {
    console.log(`  [${i}] = 0x${gpuExp[i].toString(16).padStart(2,'0')}`);
}

// WASM
const TmModule = require('../wasm/tm_64_8.js');
const Module = await TmModule();
Module.ccall('tm_wasm_init', null, ['number'], [key]);
const wasmBuf = Module._malloc(128);
Module.ccall('tm_wasm_test_expansion', null, ['number', 'number'], [data, wasmBuf]);
const wasmExp = Module.HEAPU8.slice(wasmBuf, wasmBuf + 128);

// Initial bytes (before expansion)
const initBytes = new Uint8Array(128);
for (let i = 0; i < 128; i += 8) {
    initBytes[i]   = (key >>> 24) & 0xFF;
    initBytes[i+1] = (key >>> 16) & 0xFF;
    initBytes[i+2] = (key >>> 8) & 0xFF;
    initBytes[i+3] = key & 0xFF;
    initBytes[i+4] = (data >>> 24) & 0xFF;
    initBytes[i+5] = (data >>> 16) & 0xFF;
    initBytes[i+6] = (data >>> 8) & 0xFF;
    initBytes[i+7] = data & 0xFF;
}

console.log('\nWASM expansion (wasmExp[i] - initBytes[i]):');
for (let i = 0; i < 16; i++) {
    const actual = (wasmExp[i] - initBytes[i] + 256) & 0xFF;
    const cpu = fwd[127 - i];
    const gpu = gpuExp[i];
    const match_cpu = cpu === actual ? '✓' : '✗';
    const match_gpu = gpu === actual ? '✓' : '✗';
    console.log(`  [${i.toString().padStart(2)}] actual=0x${actual.toString(16).padStart(2,'0')} cpu=0x${cpu.toString(16).padStart(2,'0')}${match_cpu} gpu=0x${gpu.toString(16).padStart(2,'0')}${match_gpu}`);
}

Module._free(wasmBuf);
