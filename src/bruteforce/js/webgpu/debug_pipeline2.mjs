// Debug: trace per-iteration state for pipeline map 0

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
            rolling = s & 0xFF;
            d[i] = rolling;
            carry = nc;
        }
    }
    function alg1(map) {
        let carry = 1, rolling = map & 0xFF;
        for (let i = 3; i >= 0; i--) {
            const s = rolling + d[i] + carry;
            const nc = s > 0xFF ? 1 : 0;
            rolling = s & 0xFF;
            d[i] = rolling;
            carry = nc;
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

function buildMapRng(entries) {
    const n = entries.length;
    const mapRng = new Uint8Array(n * 2048);
    const nibbleSel = new Uint16Array(n);
    for (let m = 0; m < n; m++) {
        let seed = (entries[m].rng1 << 8) | entries[m].rng2;
        for (let i = 0; i < 2048; i++) {
            const next = rngNext(seed);
            mapRng[m * 2048 + i] = ((next >>> 8) ^ next) & 0xFF;
            seed = next;
        }
        nibbleSel[m] = entries[m].nibble_selector;
    }
    return { mapRng, nibbleSel };
}

function bswap32(x) {
    return (((x & 0xFF) << 24) | (((x >> 8) & 0xFF) << 16) | (((x >> 16) & 0xFF) << 8) | ((x >> 24) & 0xFF)) >>> 0;
}
function byteAdd(a, b) {
    a >>>= 0; b >>>= 0;
    return ((((a & 0x00FF00FF) + (b & 0x00FF00FF)) & 0x00FF00FF) |
            (((a & 0xFF00FF00) + (b & 0xFF00FF00)) & 0xFF00FF00)) >>> 0;
}
function byteSub(a, b) {
    a >>>= 0; b >>>= 0;
    return ((((a & 0x00FF00FF) - (b & 0x00FF00FF)) & 0x00FF00FF) |
            (((a & 0xFF00FF00) - (b & 0xFF00FF00)) & 0xFF00FF00)) >>> 0;
}

function buildExpansionVals(key) {
    let s = (key >>> 16) & 0xFFFF;
    const out = new Uint8Array(128);
    for (let i = 0; i < 128; i++) {
        const next = rngNext(s);
        out[i] = ((next >>> 8) ^ next) & 0xFF;
        s = next;
    }
    const expansion = new Uint32Array(32);
    for (let lane = 0; lane < 32; lane++) {
        let val = 0;
        for (let byteIdx = 0; byteIdx < 4; byteIdx++) {
            const b = lane * 4 + byteIdx;
            const j = (b / 8) | 0;
            const k = b % 8;
            let accum = 0;
            for (let i = 0; i < j; i++) accum += out[k + i * 8];
            val |= (accum & 0xFF) << (byteIdx * 8);
        }
        expansion[lane] = val >>> 0;
    }
    return expansion;
}

// Run one map with verbose trace
function runOneMapSeqVerbose(vals, mapIdx, mapRng, nibbleSel, verbose) {
    const newVals = [...vals];
    let localPos = 0;
    const mapBase = mapIdx * 2048;
    let nibbleSelector = nibbleSel[mapIdx];

    for (let i = 0; i < 16; i++) {
        const srcLane = (i / 4) | 0;
        const byteShift = (i & 3) * 8;
        const currentByte = (vals[srcLane] >> byteShift) & 0xFF;
        const nibble = (nibbleSelector >> 15) & 1;
        nibbleSelector = (nibbleSelector << 1) & 0xFFFF;
        const effectiveByte = nibble === 1 ? (currentByte >> 4) : currentByte;
        const algId = (effectiveByte >> 1) & 7;
        const base = mapBase + localPos;

        if (verbose) {
            console.log(`  iter ${i}: srcLane=${srcLane} byte=0x${currentByte.toString(16)} nibble=${nibble} eff=0x${effectiveByte.toString(16)} algId=${algId} localPos=${localPos}`);
        }

        if (algId === 2 || algId === 5) {
            const carryByte = mapRng[mapBase + localPos];
            for (let ci = 0; ci < 32; ci++) {
                const neighbor = ci < 31 ? vals[ci + 1] : 0;
                if (algId === 2) {
                    const carry = (ci === 31)
                        ? (((carryByte >> 7) & 1) << 24)
                        : ((neighbor & 1) << 24);
                    newVals[ci] = (((vals[ci] & 0x00010000) >> 8) | carry |
                                   ((vals[ci] >> 1) & 0x007F007F) |
                                   ((vals[ci] << 1) & 0xFE00FE00) |
                                   ((vals[ci] >> 8) & 0x00800080)) >>> 0;
                } else {
                    const carry = (ci === 31)
                        ? (((carryByte & 0x80) << 24) >>> 0)
                        : (((neighbor & 0x80) << 24) >>> 0);
                    newVals[ci] = (((vals[ci] & 0x00800000) >> 8) | carry |
                                   ((vals[ci] >> 1) & 0x7F007F00) |
                                   ((vals[ci] << 1) & 0x00FE00FE) |
                                   ((vals[ci] >> 8) & 0x00010001)) >>> 0;
                }
            }
            localPos += 1;
        } else {
            for (let ci = 0; ci < 32; ci++) {
                const v = vals[ci] >>> 0;
                switch (algId) {
                    case 0: {
                        const p0 = base + (127 - ci * 4);
                        const p1 = base + (126 - ci * 4);
                        const p2 = base + (125 - ci * 4);
                        const p3 = base + (124 - ci * 4);
                        const b0 = (mapRng[p0] >> 7) & 1;
                        const b1 = (mapRng[p1] >> 7) & 1;
                        const b2 = (mapRng[p2] >> 7) & 1;
                        const b3 = (mapRng[p3] >> 7) & 1;
                        const rng = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
                        newVals[ci] = (((v << 1) & 0xFEFEFEFE) | rng) >>> 0;
                        break;
                    }
                    case 1: {
                        const p0 = base + (127 - ci * 4);
                        const p1 = base + (126 - ci * 4);
                        const p2 = base + (125 - ci * 4);
                        const p3 = base + (124 - ci * 4);
                        const rng = (mapRng[p0] | (mapRng[p1] << 8) | (mapRng[p2] << 16) | (mapRng[p3] << 24)) >>> 0;
                        newVals[ci] = byteAdd(v, rng);
                        break;
                    }
                    case 3: {
                        const p0 = base + (127 - ci * 4);
                        const p1 = base + (126 - ci * 4);
                        const p2 = base + (125 - ci * 4);
                        const p3 = base + (124 - ci * 4);
                        const rng = (mapRng[p0] | (mapRng[p1] << 8) | (mapRng[p2] << 16) | (mapRng[p3] << 24)) >>> 0;
                        newVals[ci] = (v ^ rng) >>> 0;
                        break;
                    }
                    case 4: {
                        const p0 = base + (127 - ci * 4);
                        const p1 = base + (126 - ci * 4);
                        const p2 = base + (125 - ci * 4);
                        const p3 = base + (124 - ci * 4);
                        const rng = (mapRng[p0] | (mapRng[p1] << 8) | (mapRng[p2] << 16) | (mapRng[p3] << 24)) >>> 0;
                        newVals[ci] = byteSub(v, rng);
                        break;
                    }
                    case 6: {
                        const p0 = base + (ci * 4);
                        const p1 = base + (ci * 4 + 1);
                        const p2 = base + (ci * 4 + 2);
                        const p3 = base + (ci * 4 + 3);
                        const rng = ((mapRng[p0] & 0x80) | ((mapRng[p1] & 0x80) << 8) |
                                     ((mapRng[p2] & 0x80) << 16) | ((mapRng[p3] & 0x80) << 24)) >>> 0;
                        newVals[ci] = (((v >> 1) & 0x7F7F7F7F) | rng) >>> 0;
                        break;
                    }
                    case 7: {
                        newVals[ci] = (v ^ 0xFFFFFFFF) >>> 0;
                        break;
                    }
                }
            }
            if (algId !== 7) localPos += 128;
        }

        for (let ci = 0; ci < 32; ci++) vals[ci] = newVals[ci];
        if (verbose) {
            console.log(`    -> lane0=0x${vals[0].toString(16).padStart(8,'0')}`);
        }
    }
}

const key = 0x2CA5B42D;
const data = 0x0009BE9F;
const entries = buildKeySchedule(key);
const { mapRng, nibbleSel } = buildMapRng(entries);
const expansion = buildExpansionVals(key);

console.log(`Schedule: ${entries.length} entries`);
console.log(`Map 0: rng1=0x${entries[0].rng1.toString(16)} rng2=0x${entries[0].rng2.toString(16)} nibble_sel=0x${entries[0].nibble_selector.toString(16)}`);
console.log(`Map 0 first bytes: ${Array.from(mapRng.slice(0,8)).map(x => '0x'+x.toString(16)).join(', ')}`);

const vals = new Array(32);
for (let ci = 0; ci < 32; ci++) {
    const v = (ci % 2 === 0) ? bswap32(key) : bswap32(data);
    vals[ci] = byteAdd(v, expansion[ci]);
}

console.log('\nAfter expansion:');
console.log('  lane0: 0x' + vals[0].toString(16).padStart(8,'0'));

console.log('\nRunning map 0 (verbose):');
runOneMapSeqVerbose(vals, 0, mapRng, nibbleSel, true);
console.log('After map 0: lane0=0x' + vals[0].toString(16).padStart(8,'0'));
