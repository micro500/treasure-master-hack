// Verify GPU hits using tm_core.mjs (the actual validator logic)
import { readFileSync } from 'fs';
import { buildKeySchedule, buildExpansionValues, process as tmProcess } from '../wasm/validator_lambda/tm_core.mjs';

const KEY = 3055187383;

const txt = readFileSync('C:/Users/Peter/.claude/projects/N--prgm-treasure-master-hack/f7e1ef61-a077-49d8-ae9c-d8040f7fb562/tool-results/bszpeluut.txt', 'utf8');
const hits = [];
for (const line of txt.split('\n')) {
    const m = line.match(/0x([0-9a-f]{8})\s+flags=0x([0-9a-f]+)/);
    if (m) hits.push({ data: parseInt(m[1], 16), gpuFlags: parseInt(m[2], 16) });
}
console.log(`Loaded ${hits.length} hits`);

const schedule        = buildKeySchedule(KEY);
const expansionValues = buildExpansionValues(KEY);

let checksumFail = 0, flagMismatch = 0, ok = 0;
const examples = { checksumFail: [], flagMismatch: [] };

for (const { data, gpuFlags } of hits) {
    const match = tmProcess(KEY, data, schedule, expansionValues);
    if (!match) {
        checksumFail++;
        if (examples.checksumFail.length < 5)
            examples.checksumFail.push(`0x${data.toString(16)}  gpuFlags=0x${gpuFlags.toString(16)}`);
        continue;
    }
    const expectedWorldBit = match.world === 'carnival' ? 0 : 1;
    const submittedWorldBit = gpuFlags & 0x01;
    const gpuMachineFlags   = gpuFlags & 0xFE;
    const tmMachineFlags    = match.flags & 0xFE;
    if (submittedWorldBit !== expectedWorldBit || gpuMachineFlags !== tmMachineFlags) {
        flagMismatch++;
        if (examples.flagMismatch.length < 5)
            examples.flagMismatch.push(`0x${data.toString(16)}  gpu=0x${gpuFlags.toString(16)} tm=0x${(match.flags).toString(16)} world=${match.world}`);
        continue;
    }
    ok++;
}

console.log(`OK:             ${ok}`);
console.log(`Checksum fail:  ${checksumFail}`);
console.log(`Flag mismatch:  ${flagMismatch}`);
if (examples.checksumFail.length)  { console.log('Checksum fail examples:'); examples.checksumFail.forEach(s => console.log(' ', s)); }
if (examples.flagMismatch.length)  { console.log('Flag mismatch examples:');  examples.flagMismatch.forEach(s => console.log(' ', s)); }
