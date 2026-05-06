// test_hash.mjs — CPU verification of the GPU warp_seq_hash function
//
// Replicates the GPU kernel's phase-1 state and hash computation:
//   1. expand() to build the initial 128-byte state
//   2. runOneMap() for the first HASH_SPLIT_MAP (5) schedule entries
//   3. warp_seq_hash() with seeds matching the kernel's h1/h2/h3
//
// Usage:
//   node test_hash.mjs <key_hex> <writer_hex> <reader_hex> [writer2 reader2 ...]
//
// GPU-reported h2/h3 values are printed alongside the CPU-computed ones so
// you can verify they agree.  Pass --gpu h2 h3 after each pair to compare:
//   node test_hash.mjs <key> 0x040029c0 0x04002940 --gpu 0xae5490d7 0x6615ace6
//
// Example using all four pairs from the debug output:
//   node test_hash.mjs 0x21795CA1 \
//     0x040029c0 0x04002940 --gpu 0xae5490d7 0x6615ace6 \
//     0x04007d00 0x04007d80 --gpu 0x64e4b3d4 0x5042e2c9 \
//     0x04003340 0x040033c0 --gpu 0x31527fbd 0x683f0d38 \
//     0x0400b300 0x0400b380 --gpu 0x5e5384be 0x7a73402b

import { buildKeySchedule, buildExpansionValues, rngTable, expand, runOneMap }
    from './tm_core.js';

// ─── Constants matching tm_seq.cl ─────────────────────────────────────────────

const HASH_SPLIT_MAP = 5;
const SEED_H1        = 0x811c9dc5;
const SEED_H2        = (0x811c9dc5 ^ 0xDEADBEEF) >>> 0;
const SEED_H3        = (0x811c9dc5 ^ 0xCAFEBABE) >>> 0;

// ─── Hash helpers ─────────────────────────────────────────────────────────────

// Read a little-endian uint32 from code[offset..offset+3]
function readU32LE(code, offset) {
    return (code[offset]
        | (code[offset + 1] << 8)
        | (code[offset + 2] << 16)
        | (code[offset + 3] << 24)) >>> 0;
}

// Sequential FNV-1a hash over all 32 lane values with position-sensitive mixing.
// Matches warp_local_hash() in tm_seq.cl exactly.
// vals[32] = per-lane uint32 values (vals[i] = readU32LE(code, i*4)).
function warpLocalHash(vals, seed) {
    let h = seed >>> 0;
    for (let i = 0; i < 32; i++) {
        const contrib = Math.imul(vals[i], (Math.imul(i, 2654435761) + 1) >>> 0) >>> 0;
        h = Math.imul((h ^ contrib) >>> 0, 16777619) >>> 0;
    }
    return h;
}

// ─── Phase-1 state computation ────────────────────────────────────────────────

function phase1State(key, data, schedule, expansionValues) {
    const code = expand(rngTable, key, data, expansionValues);
    const end  = Math.min(schedule.length, HASH_SPLIT_MAP);
    for (let i = 0; i < end; i++)
        runOneMap(rngTable, code, schedule[i]);
    return code;
}

function computeHashes(code) {
    const vals = Array.from({length: 32}, (_, lane) => readU32LE(code, lane * 4));
    return {
        h1: warpLocalHash(vals, SEED_H1),
        h2: warpLocalHash(vals, SEED_H2),
        h3: warpLocalHash(vals, SEED_H3),
    };
}

// ─── Argument parsing ─────────────────────────────────────────────────────────

const args = process.argv.slice(2);

if (args.length < 3) {
    console.error(
        'Usage: node test_hash.mjs <key_hex> <writer_hex> <reader_hex>'
        + ' [--gpu h2_hex h3_hex] [writer2 reader2 [--gpu ...]] ...'
    );
    process.exit(1);
}

const key = parseInt(args[0], 16) >>> 0;
console.log(`key = 0x${key.toString(16).padStart(8, '0')}`);

const schedule        = buildKeySchedule(key);
const expansionValues = buildExpansionValues(key);

const phase1End = Math.min(schedule.length, HASH_SPLIT_MAP);
console.log(`schedule entries: ${schedule.length}, phase-1 uses first ${phase1End}`);
console.log(`h1 seed = 0x${SEED_H1.toString(16).padStart(8,'0')}`);
console.log(`h2 seed = 0x${SEED_H2.toString(16).padStart(8,'0')}`);
console.log(`h3 seed = 0x${SEED_H3.toString(16).padStart(8,'0')}`);
console.log();

// Parse remaining args into pair records
const pairs = [];
let i = 1;
while (i < args.length) {
    const writerHex = args[i++];
    if (i >= args.length) { console.error('Odd number of data args'); process.exit(1); }
    const readerHex = args[i++];
    let gpuH2 = null, gpuH3 = null;
    if (args[i] === '--gpu') {
        i++;
        gpuH2 = parseInt(args[i++], 16) >>> 0;
        gpuH3 = parseInt(args[i++], 16) >>> 0;
    }
    pairs.push({
        writer: parseInt(writerHex, 16) >>> 0,
        reader: parseInt(readerHex, 16) >>> 0,
        gpuH2, gpuH3,
    });
}

// ─── Process each pair ────────────────────────────────────────────────────────

for (const { writer, reader, gpuH2, gpuH3 } of pairs) {
    const wState  = phase1State(key, writer, schedule, expansionValues);
    const rState  = phase1State(key, reader, schedule, expansionValues);

    const wHashes = computeHashes(wState);
    const rHashes = computeHashes(rState);

    const stateMatch = wState.every((v, j) => v === rState[j]);

    const hex8 = n => n.toString(16).padStart(8, '0');
    console.log(`writer=0x${hex8(writer)}  reader=0x${hex8(reader)}  diff=0x${hex8((writer ^ reader) >>> 0)}`);
    console.log(`  writer  h1=0x${hex8(wHashes.h1)}  h2=0x${hex8(wHashes.h2)}  h3=0x${hex8(wHashes.h3)}`);
    console.log(`  reader  h1=0x${hex8(rHashes.h1)}  h2=0x${hex8(rHashes.h2)}  h3=0x${hex8(rHashes.h3)}`);

    const cpuH2Match = wHashes.h2 === rHashes.h2;
    const cpuH3Match = wHashes.h3 === rHashes.h3;
    console.log(`  cpu: state_match=${stateMatch}  h2_match=${cpuH2Match}  h3_match=${cpuH3Match}`);

    if (gpuH2 !== null) {
        const wH2AgreeGpu = wHashes.h2 === gpuH2;
        const rH2AgreeGpu = rHashes.h2 === gpuH2;
        const wH3AgreeGpu = wHashes.h3 === gpuH3;
        const rH3AgreeGpu = rHashes.h3 === gpuH3;
        console.log(`  gpu h2=0x${hex8(gpuH2)}  h3=0x${hex8(gpuH3)}`);
        console.log(`  writer vs gpu: h2=${wH2AgreeGpu ? 'MATCH' : 'DIFFER'}  h3=${wH3AgreeGpu ? 'MATCH' : 'DIFFER'}`);
        console.log(`  reader vs gpu: h2=${rH2AgreeGpu ? 'MATCH' : 'DIFFER'}  h3=${rH3AgreeGpu ? 'MATCH' : 'DIFFER'}`);
    }

    console.log();
}
