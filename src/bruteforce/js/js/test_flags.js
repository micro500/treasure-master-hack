import {
      buildKeySchedule, buildExpansionValues,
      process, processFlags,
  } from './tm_core.js';

  const key  = 0x2CA5B42D;
  const data = 0x7443E16;

  const schedule        = buildKeySchedule(key);
  const expansionValues = buildExpansionValues(key);

  const result = process(key, data, schedule, expansionValues);
  if (result) {
      console.log(`world: ${result.world}`);
      console.log(`flags: 0x${result.flags.toString(16)}`);
  } else {
      console.log('no checksum match');
  }

  const { carnival, other } = processFlags(key, data, schedule, expansionValues);
  console.log(`carnival flags: 0x${carnival.toString(16)}`);
  console.log(`other flags:    0x${other.toString(16)}`);