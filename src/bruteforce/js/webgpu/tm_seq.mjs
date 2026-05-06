// WebGPU implementation — mirrors tm_wasm_init / tm_wasm_run interface.
//
// Usage:
//   import { TmWebGPU } from './tm_seq.mjs';
//   const tm = await TmWebGPU.create(shaderSource);  // shaderSource = tm_seq.wgsl text
//   tm.init(0x2CA5B42D);
//   const bytes = tm.run(0xF73A2612, 64, outBuf, outBuf.length);  // returns result_size

// -------------------------------------------------------------------
// RNG table — built once at module load, shared across all instances
// -------------------------------------------------------------------

const RNG_TABLE = (() => {
    const t = new Uint16Array(0x10000);
    for (let i = 0; i <= 0xFF; i++) {
        for (let j = 0; j <= 0xFF; j++) {
            let a = i, b = j, carry;
            b = (b + a) & 0xFF;
            a = a + 0x89; carry = a > 0xFF ? 1 : 0; a &= 0xFF;
            b = b + 0x2A + carry; carry = b > 0xFF ? 1 : 0; b &= 0xFF;
            a = a + 0x21 + carry; carry = a > 0xFF ? 1 : 0; a &= 0xFF;
            b = b + 0x43 + carry; b &= 0xFF;
            t[(i << 8) | j] = (a << 8) | b;
        }
    }
    return t;
})();

// -------------------------------------------------------------------
// Key schedule
// -------------------------------------------------------------------

const ALL_MAPS = [
    0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07,
    0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11,
];

function buildKeySchedule(key) {
    // 4-byte mutable state initialised from key MSB-first
    const d = [
        (key >>> 24) & 0xFF,
        (key >>> 16) & 0xFF,
        (key >>>  8) & 0xFF,
         key         & 0xFF,
    ];

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
        const [a, b, c, e] = [d[0], d[1], d[2], d[3]];
        d[0] = e; d[1] = c; d[2] = b; d[3] = a;
    }

    function alg5(map) {
        let temp = (((map << 1) & 0xFF) ^ 0xFF);
        temp = (temp + d[0]) & 0xFF;
        temp = (temp - map + 256) & 0xFF;
        d[0] = temp;
        const [a, b, c] = [d[1], d[2], d[3]];
        d[1] = c; d[2] = a; d[3] = b;
    }

    const algFns = [
        alg0,
        (m) => alg1(m),
        (m) => alg2(m),
        (m) => { alg2(m); alg1(m); },
        (m) => { alg2(m); alg0(m); },
        (m) => alg5(m),
        (m) => { alg5(m); alg1(m); },
        (m) => { alg5(m); alg0(m); },
    ];

    function runAndCapture(map, algNum) {
        algFns[algNum](map);
        return {
            rng1: d[0],
            rng2: d[1],
            nibble_selector: ((d[3] << 8) | d[2]) & 0xFFFF,
        };
    }

    function generateEntry(map, algOverride = -1) {
        // Special 0x1B: run alg6 first as side-effect, then re-derive algNum
        if (algOverride < 0 && map === 0x1B) algFns[6](map);
        const algNum = algOverride >= 0
            ? algOverride
            : (map === 0x06 ? 0 : (d[(map >> 4) & 3] >> 2) & 7);
        return runAndCapture(map, algNum);
    }

    const entries = [];
    for (const map of ALL_MAPS) {
        entries.push(generateEntry(map));
        if (map === 0x22) entries.push(generateEntry(map, 4));
    }
    return entries;
}

// -------------------------------------------------------------------
// map_rng and nibble_sel buffers
// -------------------------------------------------------------------

function buildMapRng(entries) {
    const n = entries.length;
    const mapRng = new Uint8Array(n * 2048);
    const nibbleSel = new Uint16Array(n);
    for (let m = 0; m < n; m++) {
        let seed = (entries[m].rng1 << 8) | entries[m].rng2;
        for (let i = 0; i < 2048; i++) {
            const next = RNG_TABLE[seed];
            mapRng[m * 2048 + i] = ((next >>> 8) ^ next) & 0xFF;
            seed = next;
        }
        nibbleSel[m] = entries[m].nibble_selector;
    }
    return { mapRng, nibbleSel };
}

// -------------------------------------------------------------------
// Expansion values (32 u32s, purely key-dependent)
//
// Mirrors tm_opencl_seq.cpp lines 79-99.
// pos_base in the C++ version is used to locate the seed's position in
// the global RNG sequence; for each lane byte we sum RNG output bytes
// at offsets k, k+8, k+16, ... from pos_base.
//
// Since the global sequence at pos_base is just the RNG chain starting
// from seed = (key >> 16) & 0xFFFF, we can compute those outputs
// directly by running the RNG 128 steps from that seed.
// -------------------------------------------------------------------

function buildExpansionVals(key) {
    let s = (key >>> 16) & 0xFFFF;
    const out = new Uint8Array(128);
    for (let i = 0; i < 128; i++) {
        const next = RNG_TABLE[s];
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

// -------------------------------------------------------------------
// WebGPU wrapper
// -------------------------------------------------------------------

// Packed little-endian u32 representations of carnival_world_data_k[128]
// and other_world_data_k[128] — used CPU-side for testChecksum.
const CARNIVAL_WORLD_U32 = new Uint32Array([
    0x00000000, 0x00000000, 0x00000000, 0x5E3D0000,
    0x23C8A6A1, 0x7C3F6ED7, 0x9F1B46D2, 0x9B5CD2AB,
    0x30674332, 0xF323A4A0, 0x21EABF27, 0x1A31130F,
    0x3439A115, 0x6E52D2E4, 0x43F6F7A6, 0xD84128D1,
    0xC5E155DC, 0x84D4F549, 0xAB901F52, 0xC32AE426,
    0x81AC59C2, 0xC37A3558, 0x04019A51, 0xA7FBE2F5,
    0x9A468BAE, 0xDDFA4127, 0x7E237263, 0x0B5A441B,
    0xFA093C2A, 0xA13C59A3, 0x464F90F0, 0xF4D7D19E,
]);
const OTHER_WORLD_U32 = new Uint32Array([
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xC168CA00,
    0x04D24466, 0x8681900B, 0xE2D2F4C7, 0x0CE322F1,
    0xFFFB54D9, 0x7281CF0A, 0x989A940A, 0x80ABFFD3,
    0x45B7E59A, 0xF0D28F6E, 0xAEB3FF67, 0x069CBB49,
    0xA3494012, 0x7B32DB9A, 0xB95AA158, 0x6E2D2B2B,
    0x1A1C9336, 0xE4180352, 0xBDC1B15E, 0x50F1FB44,
]);

function checksumOk(bytes128, length) {
    const total = (bytes128[127 - (length - 1)] << 8) | bytes128[127 - (length - 2)];
    if (total > (length - 2) * 255) return false;
    let sum = 0;
    for (let i = 0; i < length - 2; i++) sum += bytes128[127 - i];
    return sum === total;
}

export class TmWebGPU {
    #device;
    #pipeline;
    #pipelineExpand      = null;
    #pipelinePipeline    = null;
    #pipelineWcAlgMulti  = null;
    #shaderModule     = null;
    #key = 0;
    #entries = null;   // key schedule entries
    #mapRng = null;    // Uint8Array
    #nibbleSel = null; // Uint16Array
    #expVals = null;   // Uint32Array(32)

    // Persistent GPU buffers (invalidated on init())
    #mapRngBuf    = null;
    #nibbleSelBuf = null;
    #expValsBuf   = null;

    constructor(device, pipeline, shaderModule) {
        this.#device       = device;
        this.#pipeline     = pipeline;
        this.#shaderModule = shaderModule;
    }

    static async create(shaderSource) {
        if (!navigator.gpu) throw new Error('WebGPU not available');
        const adapter = await navigator.gpu.requestAdapter();
        const device  = await adapter.requestDevice();
        return TmWebGPU.createWithDevice(device, shaderSource);
    }

    static async createWithDevice(device, shaderSource) {
        const module = device.createShaderModule({ code: shaderSource });
        const info   = await module.getCompilationInfo();
        for (const m of info.messages) {
            if (m.type === 'error')
                throw new Error(`WGSL error at line ${m.lineNum}: ${m.message}`);
        }
        const pipeline = await device.createComputePipelineAsync({
            layout: 'auto',
            compute: { module, entryPoint: 'tm_bruteforce_seq' },
        });
        return new TmWebGPU(device, pipeline, module);
    }

    // Mirrors tm_wasm_init(key)
    init(key) {
        this.#key     = key >>> 0;
        this.#entries = buildKeySchedule(key);
        const { mapRng, nibbleSel } = buildMapRng(this.#entries);
        this.#mapRng    = mapRng;
        this.#nibbleSel = nibbleSel;
        this.#expVals   = buildExpansionVals(key);

        // Invalidate cached GPU buffers so run() re-uploads
        this.#mapRngBuf?.destroy();    this.#mapRngBuf    = null;
        this.#nibbleSelBuf?.destroy(); this.#nibbleSelBuf = null;
        this.#expValsBuf?.destroy();   this.#expValsBuf   = null;
    }

    // Mirrors tm_wasm_run(start_data, count, out_buf, out_buf_size) -> result_size.
    // out_buf: Uint8Array to write hits into (5 bytes each: u32 LE data + 1 byte flags).
    // Returns number of bytes written.
    async run(startData, count, outBuf, outBufSize) {
        if (!this.#entries) throw new Error('call init() first');

        const device  = this.#device;
        const n       = this.#entries.length;

        // Upload persistent buffers once per key
        if (!this.#mapRngBuf) {
            this.#mapRngBuf = device.createBuffer({
                size:  alignUp(this.#mapRng.byteLength, 4),
                usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
            });
            device.queue.writeBuffer(this.#mapRngBuf, 0, this.#mapRng);
        }
        if (!this.#nibbleSelBuf) {
            const u32 = new Uint32Array(n);
            for (let i = 0; i < n; i++) u32[i] = this.#nibbleSel[i];
            this.#nibbleSelBuf = device.createBuffer({
                size:  u32.byteLength,
                usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
            });
            device.queue.writeBuffer(this.#nibbleSelBuf, 0, u32);
        }
        if (!this.#expValsBuf) {
            this.#expValsBuf = device.createBuffer({
                size:  this.#expVals.byteLength,
                usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
            });
            device.queue.writeBuffer(this.#expValsBuf, 0, this.#expVals);
        }

        // WebGPU limits dispatchWorkgroups Y to maxComputeWorkgroupsPerDimension
        // (typically 65535). Each workgroup handles 64 candidates, so the max
        // candidates per dispatch is 65535 * 64 = 4,194,240. For larger ranges,
        // split into multiple dispatches with updated uniforms.
        const MAX_WG   = device.limits.maxComputeWorkgroupsPerDimension;
        const BATCH    = MAX_WG * 64;

        const SENTINEL = 0x08;
        let resultSize = 0;
        const view = new DataView(outBuf.buffer, outBuf.byteOffset, outBuf.byteLength);

        for (let offset = 0; offset < count; offset += BATCH) {
            const batchCount = Math.min(BATCH, count - offset);
            const batchStart = (startData + offset) >>> 0;
            const numWg      = Math.ceil(batchCount / 64);

            const uniformData = new Uint32Array([
                this.#key,
                batchStart,
                n,               // schedule_count
                batchCount >>> 0,
            ]);
            const uniformBuf = device.createBuffer({
                size:  uniformData.byteLength,
                usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
            });
            device.queue.writeBuffer(uniformBuf, 0, uniformData);

            const resultByteLen = numWg * 32 * 4;
            const resultBuf = device.createBuffer({
                size:  resultByteLen,
                usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC,
            });
            const readBuf = device.createBuffer({
                size:  resultByteLen,
                usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST,
            });

            const bindGroup = device.createBindGroup({
                layout: this.#pipeline.getBindGroupLayout(0),
                entries: [
                    { binding: 0, resource: { buffer: resultBuf          } },
                    { binding: 1, resource: { buffer: this.#mapRngBuf    } },
                    { binding: 2, resource: { buffer: this.#nibbleSelBuf } },
                    { binding: 3, resource: { buffer: uniformBuf         } },
                    { binding: 4, resource: { buffer: this.#expValsBuf   } },
                ],
            });

            const enc  = device.createCommandEncoder();
            const pass = enc.beginComputePass();
            pass.setPipeline(this.#pipeline);
            pass.setBindGroup(0, bindGroup);
            pass.dispatchWorkgroups(1, numWg, 1);
            pass.end();
            enc.copyBufferToBuffer(resultBuf, 0, readBuf, 0, resultByteLen);
            device.queue.submit([enc.finish()]);

            await readBuf.mapAsync(GPUMapMode.READ);
            const raw = new Uint8Array(readBuf.getMappedRange());

            for (let i = 0; i < batchCount && resultSize + 5 <= outBufSize; i++) {
                const carnival = raw[i * 2];
                const other    = raw[i * 2 + 1];
                const flags    = carnival ? carnival : other;
                if (flags !== 0) {
                    const data = (batchStart + i) >>> 0;
                    view.setUint32(resultSize, data, true);
                    outBuf[resultSize + 4] = flags & ~SENTINEL;
                    resultSize += 5;
                }
            }

            readBuf.unmap();
            resultBuf.destroy();
            readBuf.destroy();
            uniformBuf.destroy();
        }

        return resultSize;
    }

    // Mirrors tm_wasm_test_expansion(data, result_128).
    // Returns Uint8Array(128) — the 128-byte working state after expansion only.
    async testExpansion(data) {
        return this.#runTestKernel('tm_test_expand', data);
    }

    // Mirrors tm_wasm_test_bruteforce_data(data, result_128).
    // Returns Uint8Array(128) — working state after expansion + all maps.
    async testPipeline(data) {
        return this.#runTestKernel('tm_test_pipeline', data);
    }

    // wc_alg / wc_alg_multi corpus harness. Runs `algIds` (length 1..16) over
    // a 128-byte input buffer using a 2048-byte RNG window seeded from `seedIn`
    // (matches the _map_-style CPU impls). Returns the 128-byte output buffer.
    // Does not require / use the init() key — the RNG window is built per call.
    async testWcAlgMulti(seedIn, algIds, inputBytes128) {
        if (inputBytes128.length !== 128) throw new Error('input must be 128 bytes');
        if (algIds.length < 1 || algIds.length > 16) throw new Error('alg count 1..16');
        const device = this.#device;

        if (!this.#pipelineWcAlgMulti) {
            this.#pipelineWcAlgMulti = await device.createComputePipelineAsync({
                layout: 'auto',
                compute: { module: this.#shaderModule, entryPoint: 'tm_test_wc_alg_multi' },
            });
        }

        // 2048-byte RNG window from seedIn — same generation as buildMapRng.
        const rngWindow = new Uint8Array(2048);
        let s = seedIn & 0xFFFF;
        for (let i = 0; i < 2048; i++) {
            const next = RNG_TABLE[s];
            rngWindow[i] = ((next >>> 8) ^ next) & 0xFF;
            s = next;
        }

        const rngBuf = device.createBuffer({
            size:  2048,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
        });
        device.queue.writeBuffer(rngBuf, 0, rngWindow);

        const inputBuf = device.createBuffer({
            size:  128,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
        });
        device.queue.writeBuffer(inputBuf, 0, inputBytes128);

        const algIdsU32 = new Uint32Array(algIds.length);
        for (let i = 0; i < algIds.length; i++) algIdsU32[i] = algIds[i] & 0xFF;
        const algIdsBuf = device.createBuffer({
            size:  algIdsU32.byteLength,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
        });
        device.queue.writeBuffer(algIdsBuf, 0, algIdsU32);

        // uniforms: only schedule_count (= chain_length) is used by tm_test_chain.
        const uniformData = new Uint32Array([0, 0, algIds.length, 0]);
        const uniformBuf = device.createBuffer({
            size:  uniformData.byteLength,
            usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        });
        device.queue.writeBuffer(uniformBuf, 0, uniformData);

        const resultBuf = device.createBuffer({
            size:  128,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC,
        });
        const readBuf = device.createBuffer({
            size:  128,
            usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST,
        });

        const bindGroup = device.createBindGroup({
            layout: this.#pipelineWcAlgMulti.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: { buffer: resultBuf  } },
                { binding: 1, resource: { buffer: rngBuf     } },
                { binding: 3, resource: { buffer: uniformBuf } },
                { binding: 5, resource: { buffer: inputBuf   } },
                { binding: 6, resource: { buffer: algIdsBuf  } },
            ],
        });

        const enc  = device.createCommandEncoder();
        const pass = enc.beginComputePass();
        pass.setPipeline(this.#pipelineWcAlgMulti);
        pass.setBindGroup(0, bindGroup);
        pass.dispatchWorkgroups(1, 1, 1);
        pass.end();
        enc.copyBufferToBuffer(resultBuf, 0, readBuf, 0, 128);
        device.queue.submit([enc.finish()]);

        await readBuf.mapAsync(GPUMapMode.READ);
        const result = new Uint8Array(readBuf.getMappedRange().slice(0));
        readBuf.unmap();

        rngBuf.destroy();
        inputBuf.destroy();
        algIdsBuf.destroy();
        uniformBuf.destroy();
        resultBuf.destroy();
        readBuf.destroy();
        return result;
    }

    // Convenience: alg count 1 = wc_alg bank.
    async testWcAlg(seedIn, algId, inputBytes128) {
        return this.testWcAlgMulti(seedIn, new Uint8Array([algId]), inputBytes128);
    }

    // Mirrors tm_wasm_test_bruteforce_checksum(data, world) -> bool.
    // world: 0 = carnival, 1 = other.
    async testChecksum(data, world) {
        const pipelineOut = await this.testPipeline(data);
        // XOR with world data and check checksum
        const worldU32 = world === 0 ? CARNIVAL_WORLD_U32 : OTHER_WORLD_U32;
        const length   = world === 0 ? 0x72 : 0x53;
        const decrypted = new Uint8Array(128);
        for (let ci = 0; ci < 32; ci++) {
            const pVal = pipelineOut[ci * 4] | (pipelineOut[ci * 4 + 1] << 8) |
                         (pipelineOut[ci * 4 + 2] << 16) | (pipelineOut[ci * 4 + 3] << 24);
            const xored = (pVal ^ worldU32[ci]) >>> 0;
            decrypted[ci * 4 + 0] = xored & 0xFF;
            decrypted[ci * 4 + 1] = (xored >>> 8) & 0xFF;
            decrypted[ci * 4 + 2] = (xored >>> 16) & 0xFF;
            decrypted[ci * 4 + 3] = (xored >>> 24) & 0xFF;
        }
        return checksumOk(decrypted, length);
    }

    async #runTestKernel(entryPoint, data) {
        if (!this.#entries) throw new Error('call init() first');
        const device = this.#device;

        // Lazily compile test pipelines
        if (entryPoint === 'tm_test_expand' && !this.#pipelineExpand) {
            this.#pipelineExpand = await device.createComputePipelineAsync({
                layout: 'auto',
                compute: { module: this.#shaderModule, entryPoint: 'tm_test_expand' },
            });
        }
        if (entryPoint === 'tm_test_pipeline' && !this.#pipelinePipeline) {
            this.#pipelinePipeline = await device.createComputePipelineAsync({
                layout: 'auto',
                compute: { module: this.#shaderModule, entryPoint: 'tm_test_pipeline' },
            });
        }
        const pl = entryPoint === 'tm_test_expand' ? this.#pipelineExpand : this.#pipelinePipeline;

        // Ensure persistent buffers are ready
        const n = this.#entries.length;
        if (!this.#expValsBuf) {
            this.#expValsBuf = device.createBuffer({
                size:  this.#expVals.byteLength,
                usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
            });
            device.queue.writeBuffer(this.#expValsBuf, 0, this.#expVals);
        }
        if (!this.#mapRngBuf) {
            this.#mapRngBuf = device.createBuffer({
                size:  alignUp(this.#mapRng.byteLength, 4),
                usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
            });
            device.queue.writeBuffer(this.#mapRngBuf, 0, this.#mapRng);
        }
        if (!this.#nibbleSelBuf) {
            const u32 = new Uint32Array(n);
            for (let i = 0; i < n; i++) u32[i] = this.#nibbleSel[i];
            this.#nibbleSelBuf = device.createBuffer({
                size:  u32.byteLength,
                usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
            });
            device.queue.writeBuffer(this.#nibbleSelBuf, 0, u32);
        }

        // uniforms: key, data_start=data, schedule_count, chunk=0 (unused by test kernels)
        const uniformData = new Uint32Array([this.#key, data >>> 0, n, 0]);
        const uniformBuf = device.createBuffer({
            size:  uniformData.byteLength,
            usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        });
        device.queue.writeBuffer(uniformBuf, 0, uniformData);

        // result: 32 u32s = 128 bytes
        const resultBuf = device.createBuffer({
            size:  128,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC,
        });
        const readBuf = device.createBuffer({
            size:  128,
            usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST,
        });

        // Build bind group with only the bindings this entry point uses.
        // tm_test_expand uses 0/3/4; tm_test_pipeline uses all five.
        const layout = pl.getBindGroupLayout(0);
        const entries = [
            { binding: 0, resource: { buffer: resultBuf        } },
            { binding: 3, resource: { buffer: uniformBuf       } },
            { binding: 4, resource: { buffer: this.#expValsBuf } },
        ];
        if (entryPoint === 'tm_test_pipeline') {
            entries.push({ binding: 1, resource: { buffer: this.#mapRngBuf    } });
            entries.push({ binding: 2, resource: { buffer: this.#nibbleSelBuf } });
        }
        const bindGroup = device.createBindGroup({ layout, entries });

        const enc  = device.createCommandEncoder();
        const pass = enc.beginComputePass();
        pass.setPipeline(pl);
        pass.setBindGroup(0, bindGroup);
        pass.dispatchWorkgroups(1, 1, 1);
        pass.end();
        enc.copyBufferToBuffer(resultBuf, 0, readBuf, 0, 128);
        device.queue.submit([enc.finish()]);

        await readBuf.mapAsync(GPUMapMode.READ);
        const result = new Uint8Array(readBuf.getMappedRange().slice(0));
        readBuf.unmap();

        resultBuf.destroy();
        readBuf.destroy();
        uniformBuf.destroy();

        return result;
    }

    destroy() {
        this.#mapRngBuf?.destroy();
        this.#nibbleSelBuf?.destroy();
        this.#expValsBuf?.destroy();
        this.#device.destroy();
    }
}

function alignUp(n, a) { return (n + a - 1) & ~(a - 1); }

// -------------------------------------------------------------------
// Smoke test (node --experimental-vm-modules tm_seq.mjs)
// -------------------------------------------------------------------

if (typeof process !== 'undefined' && process.argv[1]?.endsWith('tm_seq.mjs')) {
    const fs = await import('fs/promises');
    const { fileURLToPath } = await import('url');
    const dir = fileURLToPath(new URL('.', import.meta.url));
    const shaderSource = await fs.readFile(`${dir}tm_seq.wgsl`, 'utf8');

    const tm = await TmWebGPU.create(shaderSource);
    tm.init(0x2CA5B42D);

    const outBuf = new Uint8Array(1024);
    const bytes  = await tm.run(0xF73A2612, 64, outBuf, outBuf.length);

    console.log(`result_size = ${bytes}`);
    for (let i = 0; i < bytes; i += 5) {
        const data  = outBuf[i] | (outBuf[i+1] << 8) | (outBuf[i+2] << 16) | (outBuf[i+3] << 24);
        const flags = outBuf[i + 4];
        console.log(`  hit data=0x${(data >>> 0).toString(16).padStart(8,'0')} flags=0x${flags.toString(16).padStart(2,'0')}`);
    }

    tm.destroy();
}
