// Trace machine code walk for a mismatched hit
import { buildKeySchedule, buildExpansionValues, expand, runAllMaps, decrypt,
         CARNIVAL_WORLD, CARNIVAL_WORLD_DATA, OTHER_WORLD_DATA, rngTable } from '../wasm/validator_lambda/tm_core.mjs';

const KEY  = 3055187383;
const DATA = 0x91405a88;

const schedule        = buildKeySchedule(KEY);
const expansionValues = buildExpansionValues(KEY);
const code = expand(rngTable, KEY, DATA, expansionValues);
runAllMaps(rngTable, code, schedule);
decrypt(code, OTHER_WORLD_DATA);

const OPCODE_BYTES_USED = new Uint8Array([
    1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    3,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,0,3,0,0,
    2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,3,3,3,0,
    2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
    2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
]);
const OPCODE_TYPE = new Uint8Array([
     0, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2, 4, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2,16, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2,16, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
     4, 0, 4, 2, 0, 0, 0, 2, 0, 4, 0, 2, 0, 0, 0, 2,
    16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 0, 2, 2,
     0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
    16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
     0, 0, 4, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
     0, 0, 4, 2, 0, 0, 0, 2, 0, 0, 8, 2, 0, 0, 0, 2,
    16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
]);

const codeLength = 0x53;
const entryAddrs = [0x00, 0x05, 0x0A, 0x28, 0x50, 0xFF];

let i = 0, lastEntry = -1, nextEntryAddr = entryAddrs[0];
let result = 0;

console.log('Tracing machine code walk:');
while (i < codeLength - 2) {
    if (i === nextEntryAddr) {
        lastEntry++;
        nextEntryAddr = entryAddrs[lastEntry + 1];
        console.log(`  [entry ${lastEntry} @ i=${i}]`);
    } else if (i > nextEntryAddr) {
        lastEntry++;
        nextEntryAddr = entryAddrs[lastEntry + 1];
    }
    const opcode = code[127 - i];
    const otype  = OPCODE_TYPE[opcode];
    const bused  = OPCODE_BYTES_USED[opcode];
    const tag = otype===1?'JAM':otype===2?'ILLEGAL':otype===4?'NOP2':otype===8?'NOP':otype===16?'JUMP':'normal';
    console.log(`  i=${i} opcode=0x${opcode.toString(16).padStart(2,'0')} type=${tag} bytes=${bused}`);
    if (otype & 0x01) { result |= 0x80; break; }
    if (otype & 0x02) { result |= 0x40; break; }
    if (otype & 0x04) result |= 0x20;
    if (otype & 0x08) result |= 0x10;
    i += OPCODE_BYTES_USED[opcode] - 1;  // for loop also does i++
    i++;
}
console.log(`result = 0x${result.toString(16)}`);
