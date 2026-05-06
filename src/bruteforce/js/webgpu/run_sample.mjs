import { readFileSync } from 'fs';
import { createRequire } from 'module';
import { fileURLToPath } from 'url';

const require = createRequire(import.meta.url);
const dir = fileURLToPath(new URL('.', import.meta.url));

if (typeof navigator === 'undefined' || !navigator.gpu) {
    const { create, globals } = await import('webgpu');
    Object.assign(globalThis, globals);
    Object.defineProperty(globalThis, 'navigator', { value: { gpu: create([]) }, configurable: true });
}

import { TmWebGPU } from './tm_seq.mjs';

const KEY        = 3055187383;  // 0xB61B13B7
const START      = 2415919104;  // 0x90000000
const SIZE       = 1 << 27;     // 134217728

const shaderSource = readFileSync(`${dir}tm_seq.wgsl`, 'utf8');

console.log('Initialising WebGPU...');
const tm = await TmWebGPU.create(shaderSource);
tm.init(KEY);

const outBuf = new Uint8Array(1 << 20);  // 1MB — generous for expected hit count
console.log(`Running: key=0x${KEY.toString(16)} start=0x${START.toString(16)} size=2^27...`);
const t0 = Date.now();
const gpuBytes = await tm.run(START, SIZE, outBuf, outBuf.length);
const t1 = Date.now();
console.log(`Done in ${((t1-t0)/1000).toFixed(1)}s, ${gpuBytes/5} hits`);

for (let i = 0; i < gpuBytes; i += 5) {
    const data  = (outBuf[i] | (outBuf[i+1]<<8) | (outBuf[i+2]<<16) | (outBuf[i+3]<<24)) >>> 0;
    const flags = outBuf[i+4];
    console.log(`  0x${data.toString(16).padStart(8,'0')}  flags=0x${flags.toString(16).padStart(2,'0')}`);
}

tm.destroy();
