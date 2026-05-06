// CPU-style simulation (tm_8) vs GPU-style simulation comparison
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

// --- RNG table ---
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

// Advance seed by N steps
function seedFwd(seed, n) {
    for (let i = 0; i < n; i++) seed = rngNext(seed);
    return seed;
}

// --- Precomputed tables mimicking the C++ ---
// expansion_8[seed*128+i] = rng output at step (127-i) from seed
function makeExpansion8() {
    const t = new Uint8Array(0x10000 * 128);
    for (let s = 0; s < 0x10000; s++) {
        let seed = s;
        const tmp = new Uint8Array(128);
        for (let j = 0; j < 128; j++) {
            tmp[j] = rngByte(seed);
            seed = rngNext(seed);
        }
        for (let i = 0; i < 128; i++) t[s * 128 + i] = tmp[127 - i];
    }
    return t;
}

// regular_8[seed*128+i] = rng output at step (127-i) from seed
function makeRegular8() { return makeExpansion8(); }  // same structure

// alg0_8[seed*128+i] = bit7 of rng output at step (127-i) from seed
function makeAlg0_8() {
    const t = new Uint8Array(0x10000 * 128);
    for (let s = 0; s < 0x10000; s++) {
        let seed = s;
        const tmp = new Uint8Array(128);
        for (let j = 0; j < 128; j++) {
            tmp[j] = (rngByte(seed) >> 7) & 1;
            seed = rngNext(seed);
        }
        for (let i = 0; i < 128; i++) t[s * 128 + i] = tmp[127 - i];
    }
    return t;
}

// alg2_8_8[seed] = bit7 of rng output at step 0 from seed
function makeAlg2_8_8() {
    const t = new Uint8Array(0x10000);
    for (let s = 0; s < 0x10000; s++) t[s] = (rngByte(s) >> 7) & 1;
    return t;
}

// alg4_8[seed*128+i] = twos_comp(rng output at step (127-i)) = -output
function makeAlg4_8() {
    const t = new Uint8Array(0x10000 * 128);
    for (let s = 0; s < 0x10000; s++) {
        let seed = s;
        const tmp = new Uint8Array(128);
        for (let j = 0; j < 128; j++) {
            tmp[j] = ((rngByte(seed) ^ 0xFF) + 1) & 0xFF;
            seed = rngNext(seed);
        }
        for (let i = 0; i < 128; i++) t[s * 128 + i] = tmp[127 - i];
    }
    return t;
}

// alg5_8_8[seed] = (rng output at step 0 from seed) & 0x80
function makeAlg5_8_8() {
    const t = new Uint8Array(0x10000);
    for (let s = 0; s < 0x10000; s++) t[s] = rngByte(s) & 0x80;
    return t;
}

// alg6_8[seed*128+i] = (rng output at step i from seed) & 0x80  (FORWARD order)
function makeAlg6_8() {
    const t = new Uint8Array(0x10000 * 128);
    for (let s = 0; s < 0x10000; s++) {
        let seed = s;
        for (let j = 0; j < 128; j++) {
            t[s * 128 + j] = rngByte(seed) & 0x80;
            seed = rngNext(seed);
        }
    }
    return t;
}

// seed_fwd_128[seed] = seed advanced 128 steps
function makeSeedFwd128() {
    const t = new Uint16Array(0x10000);
    for (let s = 0; s < 0x10000; s++) t[s] = seedFwd(s, 128);
    return t;
}
// seed_fwd_1[seed] = seed advanced 1 step
function makeSeedFwd1() {
    const t = new Uint16Array(0x10000);
    for (let s = 0; s < 0x10000; s++) t[s] = rngNext(s);
    return t;
}

console.log('Building tables...');
const expansion8 = makeExpansion8();
const regular8   = makeRegular8();
const alg0_8     = makeAlg0_8();
const alg2_8_8   = makeAlg2_8_8();
const alg4_8     = makeAlg4_8();
const alg5_8_8   = makeAlg5_8_8();
const alg6_8     = makeAlg6_8();
const seedFwd128 = makeSeedFwd128();
const seedFwd1   = makeSeedFwd1();
console.log('Done.');

// --- CPU-style tm_8 map execution ---
function runOneMapCPU(data, entry) {
    let rng_seed = ((entry.rng1 << 8) | entry.rng2) & 0xFFFF;
    let nibble_selector = entry.nibble_selector & 0xFFFF;

    for (let i = 0; i < 16; i++) {
        const nibble = (nibble_selector >> 15) & 1;
        nibble_selector = (nibble_selector << 1) & 0xFFFF;
        let current_byte = data[i] & 0xFF;
        if (nibble === 1) current_byte >>= 4;
        const alg_id = (current_byte >> 1) & 7;

        if (alg_id === 0) {
            for (let j = 0; j < 128; j++)
                data[j] = ((data[j] << 1) | alg0_8[rng_seed * 128 + j]) & 0xFF;
            rng_seed = seedFwd128[rng_seed];
        } else if (alg_id === 1) {
            for (let j = 0; j < 128; j++)
                data[j] = (data[j] + regular8[rng_seed * 128 + j]) & 0xFF;
            rng_seed = seedFwd128[rng_seed];
        } else if (alg_id === 2) {
            let carry = alg2_8_8[rng_seed];
            for (let k = 127; k >= 0; k -= 2) {
                const next_carry = data[k - 1] & 0x01;
                data[k - 1] = ((data[k - 1] >> 1) | (data[k] & 0x80)) & 0xFF;
                data[k] = ((data[k] << 1) | (carry & 0x01)) & 0xFF;
                carry = next_carry;
            }
            rng_seed = seedFwd1[rng_seed];
        } else if (alg_id === 3) {
            for (let j = 0; j < 128; j++)
                data[j] = (data[j] ^ regular8[rng_seed * 128 + j]) & 0xFF;
            rng_seed = seedFwd128[rng_seed];
        } else if (alg_id === 4) {
            for (let j = 0; j < 128; j++)
                data[j] = (data[j] + alg4_8[rng_seed * 128 + j]) & 0xFF;
            rng_seed = seedFwd128[rng_seed];
        } else if (alg_id === 5) {
            let carry = alg5_8_8[rng_seed];
            for (let k = 127; k >= 0; k -= 2) {
                const next_carry = data[k - 1] & 0x80;
                data[k - 1] = ((data[k - 1] << 1) | (data[k] & 0x01)) & 0xFF;
                data[k] = ((data[k] >> 1) | carry) & 0xFF;
                carry = next_carry;
            }
            rng_seed = seedFwd1[rng_seed];
        } else if (alg_id === 6) {
            for (let j = 0; j < 128; j++)
                data[j] = ((data[j] >> 1) | alg6_8[rng_seed * 128 + j]) & 0xFF;
            rng_seed = seedFwd128[rng_seed];
        } else if (alg_id === 7) {
            for (let j = 0; j < 128; j++) data[j] = data[j] ^ 0xFF;
        }
    }
}

// --- Key schedule ---
const ALL_MAPS = [0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07,
    0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11];

function buildKeySchedule(key) {
    const d = [(key >>> 24)&0xFF, (key >>> 16)&0xFF, (key >>> 8)&0xFF, key&0xFF];
    function alg0(map) {
        let temp = (map ^ d[1]) & 0xFF;
        let carry = ((temp + d[0]) > 0xFF) ? 1 : 0;
        temp = (temp + d[0]) & 0xFF;
        const sub = temp - d[2] - (1 - carry);
        let nc = sub < 0 ? 0 : 1;
        temp = (sub + 256) & 0xFF;
        carry = nc;
        let rolling = temp;
        for (let i = 3; i >= 0; i--) {
            const s = rolling + d[i] + carry;
            nc = s > 0xFF ? 1 : 0;
            rolling = s & 0xFF; d[i] = rolling; carry = nc;
        }
    }
    function alg1(map) {
        let carry = 1, rolling = map & 0xFF;
        for (let i = 3; i >= 0; i--) {
            const s = rolling + d[i] + carry;
            const nc = s > 0xFF ? 1 : 0;
            rolling = s & 0xFF; d[i] = rolling; carry = nc;
        }
    }
    function alg2(map) {
        d[0] = (d[0] + map) & 0xFF;
        const [a,b,c,e] = [d[0],d[1],d[2],d[3]];
        d[0]=e; d[1]=c; d[2]=b; d[3]=a;
    }
    function alg5(map) {
        let temp = (((map << 1) & 0xFF) ^ 0xFF);
        temp = (temp + d[0]) & 0xFF;
        temp = (temp - map + 256) & 0xFF;
        d[0] = temp;
        const [a,b,c] = [d[1],d[2],d[3]];
        d[1]=c; d[2]=a; d[3]=b;
    }
    const algFns = [alg0, (m)=>alg1(m), (m)=>alg2(m), (m)=>{alg2(m);alg1(m);},
        (m)=>{alg2(m);alg0(m);}, (m)=>alg5(m), (m)=>{alg5(m);alg1(m);}, (m)=>{alg5(m);alg0(m);}];
    function runAndCapture(map, algOverride) {
        const algNum = algOverride >= 0 ? algOverride : (map === 0x06 ? 0 : (d[(map >> 4) & 3] >> 2) & 7);
        algFns[algNum](map);
        return { rng1: d[0], rng2: d[1], nibble_selector: ((d[3] << 8) | d[2]) & 0xFFFF };
    }
    function generateEntry(map, algOverride = -1) {
        if (algOverride < 0 && map === 0x1B) algFns[6](map);
        return runAndCapture(map, algOverride);
    }
    const entries = [];
    for (const map of ALL_MAPS) {
        entries.push(generateEntry(map));
        if (map === 0x22) entries.push(generateEntry(map, 4));
    }
    return entries;
}

// --- Compare CPU vs WASM ---
const key = 0x2CA5B42D;
const data = 0x0009BE9F;
const entries = buildKeySchedule(key);

// Setup initial state (CPU-style: 128 bytes)
const cpuData = new Uint8Array(128);
const rng_seed_for_expansion = (key >>> 16) & 0xFFFF;

// Expand: key/data pattern + expansion
for (let i = 0; i < 128; i += 8) {
    cpuData[i+0] = (key >>> 24) & 0xFF;
    cpuData[i+1] = (key >>> 16) & 0xFF;
    cpuData[i+2] = (key >>> 8) & 0xFF;
    cpuData[i+3] = key & 0xFF;
    cpuData[i+4] = (data >>> 24) & 0xFF;
    cpuData[i+5] = (data >>> 16) & 0xFF;
    cpuData[i+6] = (data >>> 8) & 0xFF;
    cpuData[i+7] = data & 0xFF;
}
for (let i = 0; i < 128; i++) {
    cpuData[i] = (cpuData[i] + expansion8[rng_seed_for_expansion * 128 + i]) & 0xFF;
}

console.log('After expansion (CPU):');
console.log('  bytes 0-3:', Array.from(cpuData.slice(0,4)).map(x => '0x'+x.toString(16)).join(', '));

// Also verify vs WASM
const TmModule = require('../wasm/tm_64_8.js');
const Module = await TmModule();
Module.ccall('tm_wasm_init', null, ['number'], [key]);
const wasmBuf = Module._malloc(128);
Module.ccall('tm_wasm_test_expansion', null, ['number', 'number'], [data, wasmBuf]);
const wasmExp = Module.HEAPU8.slice(wasmBuf, wasmBuf + 128);
console.log('After expansion (WASM):');
console.log('  bytes 0-3:', Array.from(wasmExp.slice(0,4)).map(x => '0x'+x.toString(16)).join(', '));

// Check match
let expMatch = true;
for (let i = 0; i < 128; i++) {
    if (cpuData[i] !== wasmExp[i]) { expMatch = false; console.log(`  mismatch at byte ${i}: cpu=0x${cpuData[i].toString(16)} wasm=0x${wasmExp[i].toString(16)}`); if (i > 5) break; }
}
console.log(`Expansion match: ${expMatch}`);

// Run CPU simulation through all maps and print after each
Module.ccall('tm_wasm_init', null, ['number'], [key]);
Module.ccall('tm_wasm_test_bruteforce_data', null, ['number', 'number'], [data, wasmBuf]);
const wasmPipe = Module.HEAPU8.slice(wasmBuf, wasmBuf + 128);
console.log('\nAfter full pipeline (WASM): bytes 0-3 =', Array.from(wasmPipe.slice(0,4)).map(x => '0x'+x.toString(16)).join(', '));

// Run cpu simulation
for (let m = 0; m < entries.length; m++) {
    runOneMapCPU(cpuData, entries[m]);
}
console.log('After full pipeline (CPU sim): bytes 0-3 =', Array.from(cpuData.slice(0,4)).map(x => '0x'+x.toString(16)).join(', '));

let pipeMatch = true;
for (let i = 0; i < 128; i++) {
    if (cpuData[i] !== wasmPipe[i]) { pipeMatch = false; console.log(`  pipeline mismatch at byte ${i}: cpu=0x${cpuData[i].toString(16)} wasm=0x${wasmPipe[i].toString(16)}`); if (i > 5) break; }
}
console.log(`Pipeline match: ${pipeMatch}`);

Module._free(wasmBuf);
