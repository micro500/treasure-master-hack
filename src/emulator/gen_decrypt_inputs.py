#!/usr/bin/env python3
"""Generate World-5 decryption test inputs.

Each record is a 128-byte XOR keystream. The harness:
  1. Loads the algo_test_anchor savestate.
  2. Injects the keystream at $0200..$027F.
  3. Sets $46 = entry_idx (0 or 1).
  4. Calls into the decryption routine at $7:FC65 (skipping its own PPU
     read since we injected the keystream directly).
  5. Hooks $FC88 (immediately after the XOR loop) and snapshots
     $06A5..$06A5+length, where length = 114 (entry 0) or 83 (entry 1).

We produce per-entry input files (decrypt0/decrypt1) with identical
content so the same keystream can be cross-checked against both blocks
in the C++ tests. record_size = 128 (keystream).
"""

import os
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
TIER1_TARGET   = 3_000
TIER2_RANDOM   = 17_000
TOTAL_RECORDS  = TIER1_TARGET + TIER2_RANDOM    # 20,000

PRNG_SEED = 0x544D530A          # "TMS\x0A" — decryption test type

ENTRY_COUNT  = 2                # entries 0 and 1
DECRYPT_LEN  = {0: 114, 1: 83}  # length per entry

OUT_DIR = ttp.output_dir()


# ============================================================================
# Build keystream record set
# ============================================================================

def gen_records():
    """Reuse the shared 128-byte buffer tier-1 patterns from tm_test_patterns
    (groups A..E: solids, checkerboards, walking-1/0, GALPAT, boundaries),
    then pad with deterministic random and bulk Tier 2."""
    tier1 = ttp.gen_tier1_buffer_records(TIER1_TARGET, PRNG_SEED ^ 0xDEADBEEF)
    tier2 = ttp.gen_tier2_buffer_records(PRNG_SEED, TIER2_RANDOM)
    # tier1/tier2 yield (rng_seed, buf128); we only want buf128 here.
    keystreams = [buf for _, buf in tier1] + [buf for _, buf in tier2]
    assert len(keystreams) == TOTAL_RECORDS
    return keystreams


# ============================================================================
# Output writer
# ============================================================================

INPUTS_RECORD_SIZE = 128       # [128] keystream

def write_inputs_file(entry_idx, records):
    path = os.path.join(OUT_DIR, f'decrypt{entry_idx}.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_DECRYPT, entry_idx,
                              INPUTS_RECORD_SIZE, len(records))
        for rec in records:
            assert len(rec) == 128
            f.write(rec)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, '
          f'{os.path.getsize(path):,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating World-5 decryption test inputs.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  tier1={TIER1_TARGET}, tier2={TIER2_RANDOM}, total={TOTAL_RECORDS}')
    print(f'  entries: 0 (114-byte block), 1 (83-byte block)')

    records = gen_records()
    print(f'  built {len(records)} keystream records')

    for entry_idx in range(ENTRY_COUNT):
        write_inputs_file(entry_idx, records)

    print('\nDone.')

if __name__ == '__main__':
    sys.exit(main() or 0)
