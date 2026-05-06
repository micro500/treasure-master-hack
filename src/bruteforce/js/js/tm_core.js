// tm_core.js — Treasure Master core processing pipeline (JavaScript port)
//
// Ported from:
//   src/common/rng_obj.cpp      — RNG table, expansion values, alg value tables
//   src/common/key_schedule.cpp — Key schedule mutation algorithms
//   src/bruteforce/cpu/tm_8.cpp — expand, run_alg, run_one_map, run_all_maps
//   src/common/tm_base.cpp      — checksum data, opcode tables, machine-code check
//
// Unlike the C++ version this implementation does not pre-compute the large
// per-seed algorithm tables (65536×128 bytes each). Values are generated on
// the fly, which is correct for single-pair processing and keeps memory usage
// under 1 MB (just the 128 KB RNG table).

// ─── Constants ────────────────────────────────────────────────────────────────

export const CARNIVAL_WORLD = 0;
export const OTHER_WORLD    = 1;

const CARNIVAL_WORLD_CODE_LENGTH = 0x72;
const OTHER_WORLD_CODE_LENGTH    = 0x53;

// Flags returned by checkMachineCode
export const FIRST_ENTRY_VALID    = 0x02;
export const ALL_ENTRIES_VALID    = 0x04;
export const USES_NOP             = 0x10;
export const USES_UNOFFICIAL_NOPS = 0x20;
export const USES_ILLEGAL_OPCODES = 0x40;
export const USES_JAM             = 0x80;

// Opcode classifier bits (internal)
const OP_JAM     = 0x01;
const OP_ILLEGAL = 0x02;
const OP_NOP2    = 0x04;
const OP_NOP     = 0x08;
const OP_JUMP    = 0x10;

// Map traversal order for ALL_MAPS mode (from key_schedule.cpp)
export const ALL_MAPS = [
    0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B,
    0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11,
];

// ─── World data (from tm_base.cpp) ───────────────────────────────────────────

// XOR mask applied to working code before checksum (carnival world)
const CARNIVAL_WORLD_DATA = new Uint8Array([
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3D, 0x5E, 0xA1, 0xA6, 0xC8, 0x23,
    0xD7, 0x6E, 0x3F, 0x7C, 0xD2, 0x46, 0x1B, 0x9F, 0xAB, 0xD2,
    0x5C, 0x9B, 0x32, 0x43, 0x67, 0x30, 0xA0, 0xA4, 0x23, 0xF3,
    0x27, 0xBF, 0xEA, 0x21, 0x0F, 0x13, 0x31, 0x1A, 0x15, 0xA1,
    0x39, 0x34, 0xE4, 0xD2, 0x52, 0x6E, 0xA6, 0xF7, 0xF6, 0x43,
    0xD1, 0x28, 0x41, 0xD8, 0xDC, 0x55, 0xE1, 0xC5, 0x49, 0xF5,
    0xD4, 0x84, 0x52, 0x1F, 0x90, 0xAB, 0x26, 0xE4, 0x2A, 0xC3,
    0xC2, 0x59, 0xAC, 0x81, 0x58, 0x35, 0x7A, 0xC3, 0x51, 0x9A,
    0x01, 0x04, 0xF5, 0xE2, 0xFB, 0xA7, 0xAE, 0x8B, 0x46, 0x9A,
    0x27, 0x41, 0xFA, 0xDD, 0x63, 0x72, 0x23, 0x7E, 0x1B, 0x44,
    0x5A, 0x0B, 0x2A, 0x3C, 0x09, 0xFA, 0xA3, 0x59, 0x3C, 0xA1,
    0xF0, 0x90, 0x4F, 0x46, 0x9E, 0xD1, 0xD7, 0xF4,
]);

// Byte positions included in the checksum (0xFF = included, 0x00 = excluded)
const CARNIVAL_WORLD_MASK = new Uint8Array([
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
]);

// XOR mask applied to working code before checksum (other world)
const OTHER_WORLD_DATA = new Uint8Array([
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xCA, 0x68, 0xC1, 0x66, 0x44,
    0xD2, 0x04, 0x0B, 0x90, 0x81, 0x86, 0xC7, 0xF4, 0xD2, 0xE2,
    0xF1, 0x22, 0xE3, 0x0C, 0xD9, 0x54, 0xFB, 0xFF, 0x0A, 0xCF,
    0x81, 0x72, 0x0A, 0x94, 0x9A, 0x98, 0xD3, 0xFF, 0xAB, 0x80,
    0x9A, 0xE5, 0xB7, 0x45, 0x6E, 0x8F, 0xD2, 0xF0, 0x67, 0xFF,
    0xB3, 0xAE, 0x49, 0xBB, 0x9C, 0x06, 0x12, 0x40, 0x49, 0xA3,
    0x9A, 0xDB, 0x32, 0x7B, 0x58, 0xA1, 0x5A, 0xB9, 0x2B, 0x2B,
    0x2D, 0x6E, 0x36, 0x93, 0x1C, 0x1A, 0x52, 0x03, 0x18, 0xE4,
    0x5E, 0xB1, 0xC1, 0xBD, 0x44, 0xFB, 0xF1, 0x50,
]);

const OTHER_WORLD_MASK = new Uint8Array([
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
]);

// ─── Opcode tables (from tm_base.cpp) ─────────────────────────────────────────

// Instruction length in bytes (0 = jam/illegal with unknown length)
const OPCODE_BYTES_USED = new Uint8Array([
    /* 0x00 */ 1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0x10 */ 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    /* 0x20 */ 3,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0x30 */ 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    /* 0x40 */ 1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0x50 */ 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    /* 0x60 */ 1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0x70 */ 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    /* 0x80 */ 2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0x90 */ 2,2,0,0,2,2,2,0,1,3,1,0,0,3,0,0,
    /* 0xA0 */ 2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0xB0 */ 2,2,0,0,2,2,2,0,1,3,1,0,3,3,3,0,
    /* 0xC0 */ 2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0xD0 */ 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    /* 0xE0 */ 2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
    /* 0xF0 */ 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
]);

// Per-opcode type flags (OP_JAM=1, OP_ILLEGAL=2, OP_NOP2=4, OP_NOP=8, OP_JUMP=16)
const OPCODE_TYPE = new Uint8Array([
    /* 0x00 */  0, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2, 4, 0, 0, 2,
    /* 0x10 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    /* 0x20 */ 16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
    /* 0x30 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    /* 0x40 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2,16, 0, 0, 2,
    /* 0x50 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    /* 0x60 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2,16, 0, 0, 2,
    /* 0x70 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    /* 0x80 */  4, 0, 4, 2, 0, 0, 0, 2, 0, 4, 0, 2, 0, 0, 0, 2,
    /* 0x90 */ 16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 0, 2, 2,
    /* 0xA0 */  0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
    /* 0xB0 */ 16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
    /* 0xC0 */  0, 0, 4, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
    /* 0xD0 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    /* 0xE0 */  0, 0, 4, 2, 0, 0, 0, 2, 0, 0, 8, 2, 0, 0, 0, 2,
    /* 0xF0 */ 16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
]);

// ─── RNG table ────────────────────────────────────────────────────────────────
// 65536-entry lookup: rngTable[seed] → next seed (16-bit).
// Output byte for a step = ((nextSeed >>> 8) ^ nextSeed) & 0xFF.
// Source: rng_obj.cpp generate_rng_table

export function buildRngTable() {
    const table = new Uint16Array(0x10000);
    for (let i = 0; i <= 0xFF; i++) {
        for (let j = 0; j <= 0xFF; j++) {
            let rngA = i, rngB = j, carry;

            rngB = (rngB + rngA) & 0xFF;

            rngA = rngA + 0x89; carry = rngA > 0xFF ? 1 : 0; rngA &= 0xFF;
            rngB = rngB + 0x2A + carry; carry = rngB > 0xFF ? 1 : 0; rngB &= 0xFF;
            rngA = rngA + 0x21 + carry; carry = rngA > 0xFF ? 1 : 0; rngA &= 0xFF;
            rngB = (rngB + 0x43 + carry) & 0xFF;

            table[(i << 8) | j] = (rngA << 8) | rngB;
        }
    }
    return table;
}

// Advance seed by one step and return {nextSeed, output}
function stepRng(rngTable, seed) {
    const next = rngTable[seed];
    return { nextSeed: next, output: ((next >>> 8) ^ next) & 0xFF };
}

// ─── Key schedule ─────────────────────────────────────────────────────────────
// The schedule-state mutation algorithms transform a 4-byte state (sd[0..3])
// to derive the RNG seed and nibble selector for each map pass.
// Source: key_schedule.cpp

function scheduleAlg0(sd, map) {
    let temp = (map ^ sd[1]) & 0xFF;

    let sum = temp + sd[0];
    let carry = sum > 0xFF ? 1 : 0;
    temp = sum & 0xFF;

    const diff = temp - sd[2] - (1 - carry);
    carry = diff >= 0 ? 1 : 0;
    temp = (diff + 256) & 0xFF;

    let rolling = temp;
    for (let i = 3; i >= 0; i--) {
        const s = rolling + sd[i] + carry;
        carry = s > 0xFF ? 1 : 0;
        rolling = s & 0xFF;
        sd[i] = rolling;
    }
}

function scheduleAlg1(sd, map) {
    let carry = 1;
    let rolling = map & 0xFF;
    for (let i = 3; i >= 0; i--) {
        const s = rolling + sd[i] + carry;
        carry = s > 0xFF ? 1 : 0;
        rolling = s & 0xFF;
        sd[i] = rolling;
    }
}

function scheduleAlg2(sd, map) {
    sd[0] = (sd[0] + map) & 0xFF;
    const [b0, b1, b2, b3] = sd;
    sd[0] = b3; sd[1] = b2; sd[2] = b1; sd[3] = b0;
}

function scheduleAlg3(sd, map) { scheduleAlg2(sd, map); scheduleAlg1(sd, map); }
function scheduleAlg4(sd, map) { scheduleAlg2(sd, map); scheduleAlg0(sd, map); }

function scheduleAlg5(sd, map) {
    let temp = ((map << 1) ^ 0xFF) & 0xFF;
    temp = (temp + sd[0]) & 0xFF;
    temp = (temp - map + 256) & 0xFF;
    sd[0] = temp;
    const [, b1, b2, b3] = sd;
    sd[1] = b3; sd[2] = b1; sd[3] = b2;
}

function scheduleAlg6(sd, map) { scheduleAlg5(sd, map); scheduleAlg1(sd, map); }
function scheduleAlg7(sd, map) { scheduleAlg5(sd, map); scheduleAlg0(sd, map); }

const SCHEDULE_ALGS = [
    scheduleAlg0, scheduleAlg1, scheduleAlg2, scheduleAlg3,
    scheduleAlg4, scheduleAlg5, scheduleAlg6, scheduleAlg7,
];

function emitEntry(sd) {
    return {
        seed:            ((sd[0] << 8) | sd[1]) & 0xFFFF,
        nibbleSelector:  ((sd[3] << 8) | sd[2]) & 0xFFFF,
    };
}

// Build the list of key schedule entries for a given key.
// Each entry carries the seed and nibble selector for one map pass.
export function buildKeySchedule(key, mapList = ALL_MAPS) {
    const sd = new Uint8Array(4);
    sd[0] = (key >>> 24) & 0xFF;
    sd[1] = (key >>> 16) & 0xFF;
    sd[2] = (key >>>  8) & 0xFF;
    sd[3] =  key         & 0xFF;

    const entries = [];

    for (const map of mapList) {
        // Map 0x1B: pre-apply alg 6 to mutate sd (the resulting entry is
        // discarded); algId is then recomputed from the modified sd.
        if (map === 0x1B) {
            SCHEDULE_ALGS[6](sd, map);
        }

        const algId = (map === 0x06)
            ? 0
            : (sd[(map >>> 4) & 3] >>> 2) & 7;

        SCHEDULE_ALGS[algId](sd, map);
        entries.push(emitEntry(sd));

        // Map 0x22: emit a second entry using algorithm 4
        if (map === 0x22) {
            SCHEDULE_ALGS[4](sd, map);
            entries.push(emitEntry(sd));
        }
    }

    return entries;
}

// ─── Expand ───────────────────────────────────────────────────────────────────
// Initialises the 128-byte working code from key and data, then adds a
// deterministic pseudo-random expansion derived from (key >> 16).
// Source: tm_8.cpp expand, rng_obj.cpp _generate_expansion_values

export function expand(rngTable, key, data, expansionValues = null) {
    const vals = expansionValues ?? buildExpansionValues(key);
    const code = new Uint8Array(128);
    const k0 = (key  >>> 24) & 0xFF, k1 = (key  >>> 16) & 0xFF;
    const k2 = (key  >>>  8) & 0xFF, k3 =  key          & 0xFF;
    const d0 = (data >>> 24) & 0xFF, d1 = (data >>> 16) & 0xFF;
    const d2 = (data >>>  8) & 0xFF, d3 =  data         & 0xFF;

    for (let j = 0; j < 16; j++) {
        const b = j * 8;
        code[b  ] = (k0 + vals[b  ]) & 0xFF; code[b+1] = (k1 + vals[b+1]) & 0xFF;
        code[b+2] = (k2 + vals[b+2]) & 0xFF; code[b+3] = (k3 + vals[b+3]) & 0xFF;
        code[b+4] = (d0 + vals[b+4]) & 0xFF; code[b+5] = (d1 + vals[b+5]) & 0xFF;
        code[b+6] = (d2 + vals[b+6]) & 0xFF; code[b+7] = (d3 + vals[b+7]) & 0xFF;
    }

    return code;
}

// ─── Processing algorithms ────────────────────────────────────────────────────
// Each function transforms code[0..127] in place and returns the next seed.
// Source: tm_8.cpp alg_0 .. alg_7
//
// Seed-advance rules (independent of which RNG values were consumed):
//   alg 0,1,3,4,6 → advance seed 128 steps from its value at entry
//   alg 2, 5      → advance seed 1 step  (same as the carry-fetch step)
//   alg 7         → seed unchanged
//
// alg 1 and 3 consume values starting from rngTable[seed] (one step ahead),
// so their seed advance is computed separately from the original seed.

function runAlg0(rngTable, code, seed) {
    // Shift each byte left by 1; fill LSB with bit 7 of successive RNG outputs.
    // RNG values for position i come from the (128-i)-th step, stored reversed.
    const vals = new Uint8Array(128);
    let s = seed;
    for (let j = 0; j < 128; j++) {
        const next = rngTable[s];
        vals[127 - j] = (((next >>> 8) ^ next) & 0xFF) >>> 7;   // bit 7 → 0 or 1
        s = next;
    }
    for (let i = 0; i < 128; i++) {
        code[i] = ((code[i] << 1) | vals[i]) & 0xFF;
    }
    return s;   // s == seed advanced 128 steps
}

function runAlg1(rngTable, code, seed) {
    // Add RNG values (byte-wise, wrapping). Values start from seed+1 (arithmetic).
    const vals = new Uint8Array(128);
    let s = seed;                       // arithmetic +1, not RNG step
    for (let j = 0; j < 128; j++) {
        const next = rngTable[s];
        vals[127 - j] = ((next >>> 8) ^ next) & 0xFF;
        s = next;
    }
    for (let i = 0; i < 128; i++) {
        code[i] = (code[i] + vals[i]) & 0xFF;
    }
    // Advance 128 from the original seed (not from seed+1)
    let next = seed;
    for (let j = 0; j < 128; j++) next = rngTable[next];
    return next;
}

function runAlg2(rngTable, code, seed) {
    // Right-rotate the 128-byte buffer by 1 bit. Initial carry from RNG.
    const next = rngTable[seed];
    const rngOut = ((next >>> 8) ^ next) & 0xFF;
    let carry = (rngOut & 0x80) >>> 7;                  // 0 or 1

    for (let i = 127; i >= 1; i -= 2) {
        const nextCarry = code[i - 1] & 0x01;
        code[i - 1] = ((code[i - 1] >>> 1) | (code[i] & 0x80)) & 0xFF;
        code[i]     = ((code[i]     <<  1) | (carry & 0x01))    & 0xFF;
        carry = nextCarry;
    }
    return next;    // seed advanced 1 step
}

function runAlg3(rngTable, code, seed) {
    // XOR with RNG values (same structure as alg 1, XOR instead of add).
    const vals = new Uint8Array(128);
    let s = seed;                       // arithmetic +1, not RNG step
    for (let j = 0; j < 128; j++) {
        const next = rngTable[s];
        vals[127 - j] = ((next >>> 8) ^ next) & 0xFF;
        s = next;
    }
    for (let i = 0; i < 128; i++) {
        code[i] ^= vals[i];
    }
    let next = seed;
    for (let j = 0; j < 128; j++) next = rngTable[next];
    return next;
}

function runAlg4(rngTable, code, seed) {
    // Add two's complement of RNG values (i.e. subtract). RNG starts from seed.
    const vals = new Uint8Array(128);
    let s = seed;
    for (let j = 0; j < 128; j++) {
        const next = rngTable[s];
        vals[127 - j] = (((next >>> 8) ^ next) ^ 0xFF) + 1 & 0xFF;
        s = next;
    }
    for (let i = 0; i < 128; i++) {
        code[i] = (code[i] + vals[i]) & 0xFF;
    }
    return s;   // s == seed advanced 128 steps
}

function runAlg5(rngTable, code, seed) {
    // Left-rotate the 128-byte buffer by 1 bit. Initial carry (0x00 or 0x80) from RNG.
    const next = rngTable[seed];
    const rngOut = ((next >>> 8) ^ next) & 0xFF;
    let carry = rngOut & 0x80;                          // 0 or 0x80

    for (let i = 127; i >= 1; i -= 2) {
        const nextCarry = code[i - 1] & 0x80;
        code[i - 1] = ((code[i - 1] << 1) | (code[i] & 0x01)) & 0xFF;
        code[i]     = ((code[i]     >>> 1) | carry)             & 0xFF;
        carry = nextCarry;
    }
    return next;    // seed advanced 1 step
}

function runAlg6(rngTable, code, seed) {
    // Shift each byte right by 1; fill MSB with bit 7 of RNG outputs (forward order).
    const vals = new Uint8Array(128);
    let s = seed;
    for (let j = 0; j < 128; j++) {
        const next = rngTable[s];
        vals[j] = ((next >>> 8) ^ next) & 0x80;        // 0 or 0x80, forward
        s = next;
    }
    for (let i = 0; i < 128; i++) {
        code[i] = ((code[i] >>> 1) | vals[i]) & 0xFF;
    }
    return s;   // s == seed advanced 128 steps
}

function runAlg7(_rngTable, code, seed) {
    // Invert all bytes (XOR with 0xFF). No RNG consumed.
    for (let i = 0; i < 128; i++) code[i] ^= 0xFF;
    return seed;
}

const RUN_ALGS = [
    runAlg0, runAlg1, runAlg2, runAlg3,
    runAlg4, runAlg5, runAlg6, runAlg7,
];

// ─── Map processing ───────────────────────────────────────────────────────────
// Source: tm_8.cpp run_one_map, run_all_maps

export function runOneMap(rngTable, code, entry) {
    let { seed } = entry;
    let nibbleSelector = entry.nibbleSelector;

    for (let i = 0; i < 16; i++) {
        // Extract the MSB of the nibble selector as the nibble flag
        const nibble = (nibbleSelector >>> 15) & 1;
        nibbleSelector = (nibbleSelector << 1) & 0xFFFF;

        // Read algorithm ID from the current working code byte
        let currentByte = code[i];
        if (nibble === 1) currentByte >>>= 4;
        const algId = (currentByte >>> 1) & 7;

        seed = RUN_ALGS[algId](rngTable, code, seed);
    }
}

export function runAllMaps(rngTable, code, schedule) {
    for (const entry of schedule) {
        runOneMap(rngTable, code, entry);
    }
}

// Dispatch a single algorithm call — mirrors tm_8.cpp run_alg with iterations=1.
export function runAlg(rngTable, code, algId, seed) {
    return RUN_ALGS[algId](rngTable, code, seed);
}

// Pure key-schedule algorithm application (mirrors key_schedule::test_alg).
// Mutates a copy of input4 by algId and returns the 4-byte result.
export function scheduleTestAlg(input4, algId, map) {
    const sd = new Uint8Array(4);
    sd[0] = input4[0]; sd[1] = input4[1]; sd[2] = input4[2]; sd[3] = input4[3];
    SCHEDULE_ALGS[algId](sd, map);
    return sd;
}

// Pure key-schedule dispatcher (mirrors key_schedule::dispatch_alg, no map
// special cases). Returns the algorithm id selected for the given state+map.
export function scheduleDispatchAlg(state4, map) {
    return (state4[(map >>> 4) & 3] >>> 2) & 7;
}

// ─── Checksum & decryption ────────────────────────────────────────────────────
// Source: tm_8.cpp _decrypt_*, calculate_masked_checksum, fetch_checksum_value

export function decrypt(code, worldData) {
    for (let i = 0; i < 128; i++) code[i] ^= worldData[i];
}

export function maskedChecksum(code, mask) {
    let sum = 0;
    for (let i = 0; i < 128; i++) sum = (sum + (code[i] & mask[i])) & 0xFFFF;
    return sum;
}

// The stored checksum occupies the two bytes at the start of the code region
// (high byte first), indexed via reverse_offset(x) = 127 - x.
export function fetchChecksumValue(code, codeLength) {
    return ((code[127 - (codeLength - 1)] << 8) | code[127 - (codeLength - 2)]) & 0xFFFF;
}

export { CARNIVAL_WORLD_DATA, CARNIVAL_WORLD_MASK, OTHER_WORLD_DATA, OTHER_WORLD_MASK };

export function checksumOk(code, world) {
    const [mask, len] = world === CARNIVAL_WORLD
        ? [CARNIVAL_WORLD_MASK, CARNIVAL_WORLD_CODE_LENGTH]
        : [OTHER_WORLD_MASK,    OTHER_WORLD_CODE_LENGTH];
    return maskedChecksum(code, mask) === fetchChecksumValue(code, len);
}

// ─── Machine code analysis ────────────────────────────────────────────────────
// Walks the decrypted NES 6502 code and classifies its contents.
// Source: tm_base.cpp check_machine_code

const CARNIVAL_ENTRY_ADDRS = [0x00, 0x2B, 0x33, 0x3E, 0xFF, 0xFF];
const OTHER_ENTRY_ADDRS    = [0x00, 0x05, 0x0A, 0x28, 0x50, 0xFF];

export function checkMachineCode(data, world) {
    const codeLength = world === CARNIVAL_WORLD
        ? CARNIVAL_WORLD_CODE_LENGTH : OTHER_WORLD_CODE_LENGTH;
    const entryAddrs = world === CARNIVAL_WORLD
        ? CARNIVAL_ENTRY_ADDRS : OTHER_ENTRY_ADDRS;

    const activeEntries = [0, 0, 0, 0, 0, 0];
    const hitEntries    = [0, 0, 0, 0, 0, 0];
    const validEntries  = [0, 0, 0, 0, 0, 0];
    let lastEntry = -1;
    let nextEntryAddr = entryAddrs[0];
    let result = 0;

    for (let i = 0; i < codeLength - 2; i++) {
        if (i === nextEntryAddr) {
            lastEntry++;
            hitEntries[lastEntry] = 1;
            activeEntries[lastEntry] = 1;
            nextEntryAddr = entryAddrs[lastEntry + 1];
        } else if (i > nextEntryAddr) {
            lastEntry++;
            nextEntryAddr = entryAddrs[lastEntry + 1];
        }

        const opcode = data[127 - i];   // reverse_offset(i)
        const opType = OPCODE_TYPE[opcode];

        if (opType & OP_JAM) {
            result |= USES_JAM;
            break;
        } else if (opType & OP_ILLEGAL) {
            result |= USES_ILLEGAL_OPCODES;
            break;
        } else if (opType & OP_NOP2) {
            result |= USES_UNOFFICIAL_NOPS;
        } else if (opType & OP_NOP) {
            result |= USES_NOP;
        } else if (opType & OP_JUMP) {
            for (let j = 0; j < 5; j++) {
                if (activeEntries[j]) {
                    activeEntries[j] = 0;
                    validEntries[j] = 1;
                }
            }
        }

        i += OPCODE_BYTES_USED[opcode] - 1;
    }

    // Check validity across all entry points
    let allEntriesValid = true;
    for (let i = 0; i < 5; i++) {
        if (hitEntries[i]) {
            if (!validEntries[i]) { allEntriesValid = false; break; }
        } else if (entryAddrs[i] === 0xFF) {
            // Sentinel — no more entries
        } else {
            // Entry was never reached in the main scan; do a secondary check
            // from that address to see if it could reach a jump instruction.
            let reachable = true;
            for (let j = entryAddrs[i]; j < codeLength - 2; j++) {
                const opcode = data[127 - j];
                const opType = OPCODE_TYPE[opcode];
                if ((opType & OP_JAM) || (opType & OP_ILLEGAL)) { reachable = false; break; }
                if (opType & OP_JUMP) break;
                j += OPCODE_BYTES_USED[opcode] - 1;
            }
            if (!reachable) { allEntriesValid = false; break; }
        }
    }

    if (allEntriesValid) result |= ALL_ENTRIES_VALID;
    if (validEntries[0]) result |= FIRST_ENTRY_VALID;

    return result;
}

// ─── Top-level ────────────────────────────────────────────────────────────────

// Pre-built RNG table shared across all calls
export const rngTable = buildRngTable();

// Precompute the RNG-derived offsets for expand() for repeated use with the same key.
// Returns a 128-byte Uint8Array where vals[j*8+k] is the accumulated RNG offset for
// channel k at group j. Group 0 (bytes 0-7) is all zeros — no RNG steps for the first
// group. expand() adds the key and data bytes on top of these values.
export function buildExpansionValues(key) {
    const vals = new Uint8Array(128);   // group 0 stays zero
    let seed = (key >>> 16) & 0xFFFF;
    let t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0, t7 = 0;

    for (let j = 1; j < 16; j++) {
        let n;
        n = rngTable[seed]; seed = n; t0 = (t0 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        n = rngTable[seed]; seed = n; t1 = (t1 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        n = rngTable[seed]; seed = n; t2 = (t2 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        n = rngTable[seed]; seed = n; t3 = (t3 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        n = rngTable[seed]; seed = n; t4 = (t4 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        n = rngTable[seed]; seed = n; t5 = (t5 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        n = rngTable[seed]; seed = n; t6 = (t6 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        n = rngTable[seed]; seed = n; t7 = (t7 + (((n >>> 8) ^ n) & 0xFF)) & 0xFF;
        const b = j * 8;
        vals[b  ] = t0; vals[b+1] = t1; vals[b+2] = t2; vals[b+3] = t3;
        vals[b+4] = t4; vals[b+5] = t5; vals[b+6] = t6; vals[b+7] = t7;
    }

    return vals;
}

// Process a single (key, data) pair and return flags for both worlds without
// checking the checksum. Returns { carnival: flags, other: flags }.
// Pass a pre-built schedule and/or expansionValues to avoid re-deriving them.
export function processFlags(key, data, schedule = null, expansionValues = null) {
    const sched = schedule ?? buildKeySchedule(key);
    const code = expand(rngTable, key, data, expansionValues);
    runAllMaps(rngTable, code, sched);

    const carn = code.slice();
    decrypt(carn, CARNIVAL_WORLD_DATA);

    const othr = code.slice();
    decrypt(othr, OTHER_WORLD_DATA);

    return {
        carnival: checkMachineCode(carn, CARNIVAL_WORLD),
        other:    checkMachineCode(othr, OTHER_WORLD) | OTHER_WORLD,
    };
}

// Process a single (key, data) pair. Returns null if neither checksum passes,
// or { world, data: Uint8Array(128), flags } on a match.
// Pass a pre-built schedule and/or expansionValues to avoid re-deriving them.
export function process(key, data, schedule = null, expansionValues = null) {
    const sched = schedule ?? buildKeySchedule(key);
    const code = expand(rngTable, key, data, expansionValues);
    runAllMaps(rngTable, code, sched);

    for (const world of [CARNIVAL_WORLD, OTHER_WORLD]) {
        const worldData = world === CARNIVAL_WORLD ? CARNIVAL_WORLD_DATA : OTHER_WORLD_DATA;
        const decrypted = code.slice();
        decrypt(decrypted, worldData);
        if (checksumOk(decrypted, world)) {
            return {
                world: world === CARNIVAL_WORLD ? 'carnival' : 'other',
                data:  decrypted,
                flags: checkMachineCode(decrypted, world) | world,
            };
        }
    }
    return null;
}
