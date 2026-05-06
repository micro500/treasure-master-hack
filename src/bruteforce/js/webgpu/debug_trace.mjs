// Trace per-iteration state for map 0 comparing CPU and GPU sims
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
function rngNext(s) { return RNG_TABLE[s & 0xFFFF]; }

const ALL_MAPS = [0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07,
    0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11];

function buildKeySchedule(key) {
    const d = [(key>>>24)&0xFF,(key>>>16)&0xFF,(key>>>8)&0xFF,key&0xFF];
    function alg0(map) { let temp=(map^d[1])&0xFF,carry=((temp+d[0])>0xFF)?1:0; temp=(temp+d[0])&0xFF; const sub=temp-d[2]-(1-carry); let nc=sub<0?0:1; temp=(sub+256)&0xFF; carry=nc; let rolling=temp; for(let i=3;i>=0;i--){const s=rolling+d[i]+carry;nc=s>0xFF?1:0;rolling=s&0xFF;d[i]=rolling;carry=nc;} }
    function alg1(map) { let carry=1,rolling=map&0xFF; for(let i=3;i>=0;i--){const s=rolling+d[i]+carry;const nc=s>0xFF?1:0;rolling=s&0xFF;d[i]=rolling;carry=nc;} }
    function alg2(map) { d[0]=(d[0]+map)&0xFF; const[a,b,c,e]=[d[0],d[1],d[2],d[3]]; d[0]=e;d[1]=c;d[2]=b;d[3]=a; }
    function alg5(map) { let temp=(((map<<1)&0xFF)^0xFF); temp=(temp+d[0])&0xFF; temp=(temp-map+256)&0xFF; d[0]=temp; const[a,b,c]=[d[1],d[2],d[3]]; d[1]=c;d[2]=a;d[3]=b; }
    const algFns=[alg0,(m)=>alg1(m),(m)=>alg2(m),(m)=>{alg2(m);alg1(m);},(m)=>{alg2(m);alg0(m);},(m)=>alg5(m),(m)=>{alg5(m);alg1(m);},(m)=>{alg5(m);alg0(m);}];
    function runAndCapture(map,algOverride) { const algNum=algOverride>=0?algOverride:(map===0x06?0:(d[(map>>4)&3]>>2)&7); algFns[algNum](map); return{rng1:d[0],rng2:d[1],nibble_selector:((d[3]<<8)|d[2])&0xFFFF}; }
    function generateEntry(map,algOverride=-1) { if(algOverride<0&&map===0x1B)algFns[6](map); return runAndCapture(map,algOverride); }
    const entries=[];
    for(const map of ALL_MAPS){entries.push(generateEntry(map));if(map===0x22)entries.push(generateEntry(map,4));}
    return entries;
}

function buildMapRng(entries) {
    const n=entries.length; const mapRng=new Uint8Array(n*2048); const nibbleSel=new Uint16Array(n);
    for(let m=0;m<n;m++){let seed=(entries[m].rng1<<8)|entries[m].rng2; for(let i=0;i<2048;i++){const next=rngNext(seed);mapRng[m*2048+i]=((next>>>8)^next)&0xFF;seed=next;} nibbleSel[m]=entries[m].nibble_selector;}
    return{mapRng,nibbleSel};
}

function bswap32(x){return(((x&0xFF)<<24)|(((x>>8)&0xFF)<<16)|(((x>>16)&0xFF)<<8)|((x>>24)&0xFF))>>>0;}
function byteAdd(a,b){a>>>=0;b>>>=0;return((((a&0x00FF00FF)+(b&0x00FF00FF))&0x00FF00FF)|(((a&0xFF00FF00)+(b&0xFF00FF00))&0xFF00FF00))>>>0;}

function buildExpansion(key) {
    let s=(key>>>16)&0xFFFF; const out=new Uint8Array(128);
    for(let i=0;i<128;i++){const n=rngNext(s);out[i]=((n>>>8)^n)&0xFF;s=n;}
    const exp=new Uint8Array(128);
    for(let lane=0;lane<32;lane++){for(let byteIdx=0;byteIdx<4;byteIdx++){const b=lane*4+byteIdx,j=(b/8)|0,k=b%8;let acc=0;for(let i=0;i<j;i++)acc+=out[k+i*8];exp[b]=acc&0xFF;}}
    return exp;
}

const key = 0x2CA5B42D;
const data_val = 0x0009BE9F;
const entries = buildKeySchedule(key);
const { mapRng, nibbleSel } = buildMapRng(entries);
const expansion = buildExpansion(key);

// Init GPU vals
const gpuVals = new Array(32);
for (let ci = 0; ci < 32; ci++) {
    const v = (ci%2===0) ? bswap32(key) : bswap32(data_val);
    const exp32 = (expansion[ci*4]|(expansion[ci*4+1]<<8)|(expansion[ci*4+2]<<16)|(expansion[ci*4+3]<<24))>>>0;
    gpuVals[ci] = byteAdd(v, exp32);
}

// Also init CPU flat bytes (same state)
const cpuData = new Uint8Array(128);
for (let ci = 0; ci < 32; ci++) {
    cpuData[ci*4]   = gpuVals[ci] & 0xFF;
    cpuData[ci*4+1] = (gpuVals[ci] >> 8) & 0xFF;
    cpuData[ci*4+2] = (gpuVals[ci] >> 16) & 0xFF;
    cpuData[ci*4+3] = (gpuVals[ci] >> 24) & 0xFF;
}

// Run ONE map (map 0) with per-iteration tracing for BOTH CPU and GPU
function runOneMapCPUTrace(data, m, mapRng, nibbleSel) {
    let nibble_selector = nibbleSel[m];
    let localPos = 0;
    const snapshots = [];
    for (let i = 0; i < 16; i++) {
        const nibble = (nibble_selector >> 15) & 1;
        nibble_selector = (nibble_selector << 1) & 0xFFFF;
        let current_byte = data[i] & 0xFF;
        if (nibble === 1) current_byte >>= 4;
        const alg_id = (current_byte >> 1) & 7;
        const base = m * 2048 + localPos;

        if (alg_id === 2) {
            const cb = mapRng[base];
            let carry = (cb >> 7) & 1;
            for (let k = 127; k >= 0; k -= 2) {
                const nc = data[k-1] & 0x01;
                data[k-1] = ((data[k-1] >> 1) | (data[k] & 0x80)) & 0xFF;
                data[k] = ((data[k] << 1) | carry) & 0xFF;
                carry = nc;
            }
            localPos += 1;
        } else if (alg_id === 5) {
            const cb = mapRng[base];
            let carry = cb & 0x80;
            for (let k = 127; k >= 0; k -= 2) {
                const nc = data[k-1] & 0x80;
                data[k-1] = ((data[k-1] << 1) | (data[k] & 0x01)) & 0xFF;
                data[k] = ((data[k] >> 1) | carry) & 0xFF;
                carry = nc;
            }
            localPos += 1;
        } else if (alg_id === 7) {
            for (let j = 0; j < 128; j++) data[j] ^= 0xFF;
        } else {
            for (let j = 0; j < 128; j++) {
                const step = (alg_id === 6) ? localPos + j : localPos + (127 - j);
                const rv = mapRng[m * 2048 + step];
                switch (alg_id) {
                    case 0: data[j] = (((data[j]<<1)&0xFE)|((rv>>7)&1))&0xFF; break;
                    case 1: data[j] = (data[j]+rv)&0xFF; break;
                    case 3: data[j] = (data[j]^rv)&0xFF; break;
                    case 4: data[j] = (data[j]-rv+256)&0xFF; break;
                    case 6: data[j] = (((data[j]>>1)&0x7F)|(rv&0x80))&0xFF; break;
                }
            }
            localPos += 128;
        }
        snapshots.push({alg_id, nibble, localPos, byte0: data[0], byte1: data[1], byte2: data[2], byte3: data[3]});
    }
    return snapshots;
}

function byteSub(a,b){a>>>=0;b>>>=0;return((((a&0x00FF00FF)-(b&0x00FF00FF))&0x00FF00FF)|(((a&0xFF00FF00)-(b&0xFF00FF00))&0xFF00FF00))>>>0;}

function runOneMapGPUTrace(vals, m, mapRng, nibbleSel) {
    const newVals = [...vals];
    let localPos = 0;
    const mapBase = m * 2048;
    let nibbleSelector = nibbleSel[m];
    const snapshots = [];
    for (let i = 0; i < 16; i++) {
        const srcLane = (i/4)|0;
        const byteShift = (i&3)*8;
        const currentByte = (vals[srcLane] >> byteShift) & 0xFF;
        const nibble = (nibbleSelector >> 15) & 1;
        nibbleSelector = (nibbleSelector << 1) & 0xFFFF;
        const effectiveByte = nibble === 1 ? (currentByte >> 4) : currentByte;
        const algId = (effectiveByte >> 1) & 7;
        const base = mapBase + localPos;
        if (algId === 2 || algId === 5) {
            const carryByte = mapRng[mapBase + localPos];
            for (let ci = 0; ci < 32; ci++) {
                const neighbor = ci < 31 ? vals[ci+1] : 0;
                if (algId === 2) {
                    const carry = (ci===31)?(((carryByte>>7)&1)<<24):((neighbor&1)<<24);
                    newVals[ci]=(((vals[ci]&0x00010000)>>8)|carry|((vals[ci]>>1)&0x007F007F)|((vals[ci]<<1)&0xFE00FE00)|((vals[ci]>>8)&0x00800080))>>>0;
                } else {
                    const carry = (ci===31)?(((carryByte&0x80)<<24)>>>0):(((neighbor&0x80)<<24)>>>0);
                    newVals[ci]=(((vals[ci]&0x00800000)>>8)|carry|((vals[ci]>>1)&0x7F007F00)|((vals[ci]<<1)&0x00FE00FE)|((vals[ci]>>8)&0x00010001))>>>0;
                }
            }
            localPos += 1;
        } else {
            for (let ci = 0; ci < 32; ci++) {
                const v = vals[ci]>>>0;
                switch (algId) {
                    case 0: {const p0=base+(127-ci*4),p1=base+(126-ci*4),p2=base+(125-ci*4),p3=base+(124-ci*4);newVals[ci]=(((v<<1)&0xFEFEFEFE)|((mapRng[p0]>>7)&1)|(((mapRng[p1]>>7)&1)<<8)|(((mapRng[p2]>>7)&1)<<16)|(((mapRng[p3]>>7)&1)<<24))>>>0;break;}
                    case 1: {const p0=base+(127-ci*4),p1=base+(126-ci*4),p2=base+(125-ci*4),p3=base+(124-ci*4);newVals[ci]=byteAdd(v,(mapRng[p0]|(mapRng[p1]<<8)|(mapRng[p2]<<16)|(mapRng[p3]<<24))>>>0);break;}
                    case 3: {const p0=base+(127-ci*4),p1=base+(126-ci*4),p2=base+(125-ci*4),p3=base+(124-ci*4);newVals[ci]=(v^((mapRng[p0]|(mapRng[p1]<<8)|(mapRng[p2]<<16)|(mapRng[p3]<<24))>>>0))>>>0;break;}
                    case 4: {const p0=base+(127-ci*4),p1=base+(126-ci*4),p2=base+(125-ci*4),p3=base+(124-ci*4);newVals[ci]=byteSub(v,(mapRng[p0]|(mapRng[p1]<<8)|(mapRng[p2]<<16)|(mapRng[p3]<<24))>>>0);break;}
                    case 6: {const p0=base+(ci*4),p1=base+(ci*4+1),p2=base+(ci*4+2),p3=base+(ci*4+3);newVals[ci]=(((v>>1)&0x7F7F7F7F)|((mapRng[p0]&0x80)|((mapRng[p1]&0x80)<<8)|((mapRng[p2]&0x80)<<16)|((mapRng[p3]&0x80)<<24)))>>>0;break;}
                    case 7: {newVals[ci]=(v^0xFFFFFFFF)>>>0;break;}
                }
            }
            if (algId !== 7) localPos += 128;
        }
        for (let ci = 0; ci < 32; ci++) vals[ci] = newVals[ci];
        snapshots.push({algId, nibble, localPos,
            lane0: vals[0], lane1: vals[1], lane2: vals[2], lane3: vals[3]});
    }
    return snapshots;
}

const cpuSnaps = runOneMapCPUTrace(cpuData, 0, mapRng, nibbleSel);
const gpuValsCopy = new Array(32);
for (let ci = 0; ci < 32; ci++) gpuValsCopy[ci] = gpuVals[ci];
const gpuSnaps = runOneMapGPUTrace(gpuValsCopy, 0, mapRng, nibbleSel);

console.log('Per-iteration comparison for map 0:');
for (let i = 0; i < 16; i++) {
    const cs = cpuSnaps[i];
    const gs = gpuSnaps[i];
    const cpuLane0 = (cpuSnaps[i].byte0|(cpuSnaps[i].byte1<<8)|(cpuSnaps[i].byte2<<16)|(cpuSnaps[i].byte3<<24))>>>0;
    const match = cpuLane0 === gs.lane0;
    console.log(`iter ${i}: alg=${cs.alg_id}/${gs.algId} nibble=${cs.nibble}/${gs.nibble} localPos=${cs.localPos}/${gs.localPos}`);
    console.log(`  cpu lane0=0x${cpuLane0.toString(16).padStart(8,'0')}  gpu lane0=0x${gs.lane0.toString(16).padStart(8,'0')}  ${match?'✓':'✗ DIFFER'}`);
    if (!match) {
        console.log(`  cpu bytes: ${[cs.byte0,cs.byte1,cs.byte2,cs.byte3].map(x=>'0x'+x.toString(16)).join(' ')}`);
        console.log(`  gpu bytes: ${[gs.lane0&0xFF,(gs.lane0>>8)&0xFF,(gs.lane0>>16)&0xFF,(gs.lane0>>24)&0xFF].map(x=>'0x'+x.toString(16)).join(' ')}`);
    }
}
