import { buildKeySchedule, buildExpansionValues, rngTable, expand, runAllMaps } from './tm_core.js';

const key  = 561930401;
const data = 3388997632;

const schedule        = buildKeySchedule(key);
const expansionValues = buildExpansionValues(key);

const code = expand(rngTable, key, data, expansionValues);
runAllMaps(rngTable, code, schedule);

const hex = Array.from(code).map(b => b.toString(16).padStart(2,'0'));
for (let i = 0; i < 128; i += 16)
    console.log(hex.slice(i, i+16).join(' '));