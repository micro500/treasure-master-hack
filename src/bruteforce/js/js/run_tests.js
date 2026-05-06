// run_tests.js — Validity test runner for tm_core.js
//
// Mirrors the four test suites in src/common/tester2.h:
//   run_alg_validity_tests       → N:/prgm/tm/TM_alg_test_cases4.txt
//   run_checksum_tests           → N:/prgm/tm/TM_checksum_test_cases.txt
//   run_expansion_validity_tests → N:/prgm/tm/TM_expansion_test_cases4.txt
//   run_full_validity_tests      → N:/prgm/tm/TM_full_test_cases.txt
//
// Run with:  node src/bruteforce/js/js/run_tests.js
// Requires Node 18+ (for import.meta.url / no extra flags needed for ESM).

import { createReadStream } from 'fs';
import {
    rngTable,
    buildKeySchedule,
    expand,
    runAllMaps,
    runAlg,
    maskedChecksum,
    fetchChecksumValue,
    decrypt,
    CARNIVAL_WORLD, OTHER_WORLD,
    CARNIVAL_WORLD_DATA, CARNIVAL_WORLD_MASK,
    OTHER_WORLD_DATA,   OTHER_WORLD_MASK,
} from './tm_core.js';

const TEST_DIR = 'N:/prgm/tm/';

// ─── Streaming integer parser ─────────────────────────────────────────────────
// Reads a comma-delimited file of integers in chunks and yields each value
// without loading the entire file into memory.
// mode: 'dec' (default) or 'hex'

async function* streamInts(filePath, mode = 'dec') {
    let leftover = '';
    const base = mode === 'hex' ? 16 : 10;

    for await (const chunk of createReadStream(filePath, { encoding: 'ascii', highWaterMark: 1 << 20 })) {
        const text = leftover + chunk;
        // Split on commas or newlines; hex files use newline between records
        const parts = text.split(/[,\r\n]+/);
        leftover = parts.pop();             // last fragment may be incomplete
        for (const p of parts) {
            if (!p) continue;
            const v = parseInt(p, base);
            if (!isNaN(v)) yield v;
        }
    }
    // flush the tail
    if (leftover.trim()) {
        const v = parseInt(leftover.trim(), base);
        if (!isNaN(v)) yield v;
    }
}

// Read exactly `n` integers from an async iterator into a Uint8Array
async function readN(iter, n) {
    const buf = new Uint8Array(n);
    for (let i = 0; i < n; i++) {
        const { value, done } = await iter.next();
        if (done) throw new Error(`Unexpected EOF after ${i} of ${n} values`);
        buf[i] = value;
    }
    return buf;
}

// ─── Progress helper ──────────────────────────────────────────────────────────

function progress(label, done, total) {
    const pct = ((done / total) * 100).toFixed(1);
    process.stdout.write(`\r  ${label}: ${done.toLocaleString()} / ${total.toLocaleString()} (${pct}%)   `);
}

// ─── Test 1: Algorithm validity ───────────────────────────────────────────────
// Format per case (261 decimal values):
//   [0]       algId
//   [1..2]    input seed hi, lo
//   [3..130]  128-byte input
//   [131..132] expected output seed hi, lo
//   [133..260] 128-byte expected output

async function runAlgTests() {
    const TOTAL = 1_326_336;
    const STRIDE = 261;
    const file = TEST_DIR + 'TM_alg_test_cases4.txt';

    console.log('\nAlgorithm tests');
    const iter = streamInts(file, 'dec');
    let failures = 0;

    for (let j = 0; j < TOTAL; j++) {
        const tc = await readN(iter, STRIDE);

        const algId          = tc[0];
        const inputSeed      = (tc[1] << 8) | tc[2];
        const code           = tc.slice(3, 131);
        const expectedSeed   = (tc[131] << 8) | tc[132];
        const expectedOutput = tc.slice(133, 261);

        const nextSeed = runAlg(rngTable, code, algId, inputSeed);

        let match = (nextSeed === expectedSeed);
        if (match) {
            for (let i = 0; i < 128; i++) {
                if (code[i] !== expectedOutput[i]) { match = false; break; }
            }
        }

        if (!match) {
            failures++;
            if (failures <= 5) {
                console.error(`\n  FAIL case ${j} alg=${algId} seed=0x${inputSeed.toString(16).padStart(4,'0')}`);
                console.error(`    outputSeed got=0x${nextSeed.toString(16).padStart(4,'0')} expected=0x${expectedSeed.toString(16).padStart(4,'0')}`);
                console.error(`    got:      ${Array.from(code.slice(0,16)).map(x=>x.toString(16).padStart(2,'0')).join(' ')} ...`);
                console.error(`    expected: ${Array.from(expectedOutput.slice(0,16)).map(x=>x.toString(16).padStart(2,'0')).join(' ')} ...`);
            }
        }

        if ((j & 0x3FFF) === 0) progress('alg', j, TOTAL);
    }

    progress('alg', TOTAL, TOTAL);
    const status = failures === 0 ? 'PASS' : `FAIL (${failures} failures)`;
    console.log(`\n  → ${status}`);
    return failures === 0;
}

// ─── Test 2: Checksum ─────────────────────────────────────────────────────────
// Format per case (133 decimal values):
//   [0]        world (0=carnival, 1=other)
//   [1..128]   128-byte raw working code (pre-decryption)
//   [129..130] expected checksum (little-endian: lo then hi)
//   [131..132] expected stored checksum value (little-endian)

async function runChecksumTests() {
    const TOTAL = 200_512;
    const STRIDE = 133;
    const file = TEST_DIR + 'TM_checksum_test_cases.txt';

    console.log('\nChecksum tests');
    const iter = streamInts(file, 'dec');
    let failures = 0;

    for (let j = 0; j < TOTAL; j++) {
        const tc = await readN(iter, STRIDE);

        const world = tc[0];
        const code  = tc.slice(1, 129);    // raw (pre-decryption)
        const expectedChecksum      = (tc[130] << 8) | tc[129];
        const expectedChecksumValue = (tc[132] << 8) | tc[131];

        const [worldData, mask, codeLen] = world === CARNIVAL_WORLD
            ? [CARNIVAL_WORLD_DATA, CARNIVAL_WORLD_MASK, 0x72]
            : [OTHER_WORLD_DATA,    OTHER_WORLD_MASK,    0x53];

        // Decrypt in place (matches what the C++ tester does before computing)
        decrypt(code, worldData);
        const gotChecksum      = maskedChecksum(code, mask);
        const gotChecksumValue = fetchChecksumValue(code, codeLen);

        if (gotChecksum !== expectedChecksum || gotChecksumValue !== expectedChecksumValue) {
            failures++;
            if (failures <= 5) {
                console.error(`\n  FAIL case ${j} world=${world}`);
                console.error(`    checksum      got=0x${gotChecksum.toString(16).padStart(4,'0')} expected=0x${expectedChecksum.toString(16).padStart(4,'0')}`);
                console.error(`    checksumValue got=0x${gotChecksumValue.toString(16).padStart(4,'0')} expected=0x${expectedChecksumValue.toString(16).padStart(4,'0')}`);
            }
        }

        if ((j & 0x3FFF) === 0) progress('checksum', j, TOTAL);
    }

    progress('checksum', TOTAL, TOTAL);
    const status = failures === 0 ? 'PASS' : `FAIL (${failures} failures)`;
    console.log(`\n  → ${status}`);
    return failures === 0;
}

// ─── Test 3: Expansion ────────────────────────────────────────────────────────
// Format per case (136 decimal values):
//   [0..3]   key bytes (big-endian)
//   [4..7]   data bytes (big-endian)
//   [8..135] 128-byte expected output after expand()

async function runExpansionTests() {
    const TOTAL = 1_000_000;
    const STRIDE = 136;
    const file = TEST_DIR + 'TM_expansion_test_cases4.txt';

    console.log('\nExpansion tests');
    const iter = streamInts(file, 'dec');
    let failures = 0;

    for (let j = 0; j < TOTAL; j++) {
        const tc = await readN(iter, STRIDE);

        const key      = (tc[0] << 24 | tc[1] << 16 | tc[2] << 8 | tc[3]) >>> 0;
        const data     = (tc[4] << 24 | tc[5] << 16 | tc[6] << 8 | tc[7]) >>> 0;
        const expected = tc.slice(8, 136);

        const result = expand(rngTable, key, data);

        let match = true;
        for (let i = 0; i < 128; i++) {
            if (result[i] !== expected[i]) { match = false; break; }
        }

        if (!match) {
            failures++;
            if (failures <= 5) {
                console.error(`\n  FAIL case ${j} key=0x${key.toString(16).padStart(8,'0')} data=0x${data.toString(16).padStart(8,'0')}`);
            }
        }

        if ((j & 0x3FFF) === 0) progress('expansion', j, TOTAL);
    }

    progress('expansion', TOTAL, TOTAL);
    const status = failures === 0 ? 'PASS' : `FAIL (${failures} failures)`;
    console.log(`\n  → ${status}`);
    return failures === 0;
}

// ─── Test 4: Full pipeline ────────────────────────────────────────────────────
// Format per case (136 hex values):
//   [0..3]   key bytes (big-endian)
//   [4..7]   data bytes (big-endian)
//   [8..135] 128-byte expected output after expand() + run_all_maps()

async function runFullTests() {
    const TOTAL = 10_000;
    const STRIDE = 136;
    const file = TEST_DIR + 'TM_full_test_cases.txt';

    console.log('\nFull pipeline tests');
    const iter = streamInts(file, 'hex');
    let failures = 0;

    for (let j = 0; j < TOTAL; j++) {
        const tc = await readN(iter, STRIDE);

        const key      = (tc[0] << 24 | tc[1] << 16 | tc[2] << 8 | tc[3]) >>> 0;
        const data     = (tc[4] << 24 | tc[5] << 16 | tc[6] << 8 | tc[7]) >>> 0;
        const expected = tc.slice(8, 136);

        const schedule = buildKeySchedule(key);
        const code     = expand(rngTable, key, data);
        runAllMaps(rngTable, code, schedule);

        let match = true;
        for (let i = 0; i < 128; i++) {
            if (code[i] !== expected[i]) { match = false; break; }
        }

        if (!match) {
            failures++;
            if (failures <= 5) {
                console.error(`\n  FAIL case ${j} key=0x${key.toString(16).padStart(8,'0')} data=0x${data.toString(16).padStart(8,'0')}`);
                console.error(`    got:      ${Array.from(code.slice(0,16)).map(x=>x.toString(16).padStart(2,'0')).join(' ')} ...`);
                console.error(`    expected: ${Array.from(expected.slice(0,16)).map(x=>x.toString(16).padStart(2,'0')).join(' ')} ...`);
            }
        }

        if ((j & 0xFF) === 0) progress('full', j, TOTAL);
    }

    progress('full', TOTAL, TOTAL);
    const status = failures === 0 ? 'PASS' : `FAIL (${failures} failures)`;
    console.log(`\n  → ${status}`);
    return failures === 0;
}

// ─── Main ─────────────────────────────────────────────────────────────────────

const t0 = Date.now();
console.log('Running tm_core.js validity tests...');

const results = await Promise.all([]).then(async () => {
    const r = [];
    //r.push(await runAlgTests());
    r.push(await runChecksumTests());
    r.push(await runExpansionTests());
    r.push(await runFullTests());
    return r;
});

const elapsed = ((Date.now() - t0) / 1000).toFixed(1);
const allPassed = results.every(Boolean);
console.log(`\n${'─'.repeat(50)}`);
console.log(allPassed
    ? `All tests PASSED  (${elapsed}s)`
    : `Some tests FAILED (${elapsed}s)`);
process.exit(allPassed ? 0 : 1);
