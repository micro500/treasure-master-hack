#!/usr/bin/env python3
"""Generate algorithm test inputs per ALGORITHM_TEST_PLAN.md.

Produces wc_alg_0.inputs.bin .. wc_alg_7.inputs.bin in TMTV format.

Each file contains:
  - 32-byte TMTV header (test_type=0x05, subtype=algo_id, record_size=130)
  - 20,000 records of 130 bytes each:
      [2]   rng_seed_in
      [128] buffer_in

The FCEUX harness reads these, runs the algorithm, and writes the full
260-byte-record output corpus (wc_alg_N.bin) with rng_state_out and
buffer_out appended.

The first SHARED_COUNT (= 10,000) records are byte-identical across all 8
files, so cross-algorithm oracle tests (alg1+alg4 = 2*buf, etc.) can zip
records 0..SHARED_COUNT-1 across two files.

Layout within each file:
  records 0      .. 2,999       Tier 1 hand-crafted    (shared)
  records 3,000  .. 9,999       Tier 2 random shared   (shared)
  records 10,000 .. 19,999      Tier 2 random per-algo (unique)

Tier 1 patterns and tier 2 PRNG live in tm_test_patterns.py and are
shared with gen_chain_inputs.py so both corpora exercise the same
buffer/RNG fault models.
"""

import os
import struct
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
TIER1_TARGET   = 3_000
TIER2_SHARED   = 7_000
TIER2_PER_ALGO = 10_000
TOTAL_RECORDS  = TIER1_TARGET + TIER2_SHARED + TIER2_PER_ALGO   # 20,000
SHARED_COUNT   = TIER1_TARGET + TIER2_SHARED                    # 10,000

SHARED_SEED = 0x544D5300        # "TMS\0", per the test plan

OUT_DIR = ttp.output_dir()


# ============================================================================
# Output writer
# ============================================================================

INPUTS_RECORD_SIZE = 130   # [2] rng_seed_in + [128] buffer_in (no outputs)

def write_inputs_file(wc_alg_id, records):
    path = os.path.join(OUT_DIR, f'wc_alg_{wc_alg_id}.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_WC_ALG, wc_alg_id,
                              INPUTS_RECORD_SIZE, len(records),
                              shared_count=SHARED_COUNT)
        for rng_in, buf_in in records:
            f.write(struct.pack('<H', rng_in))
            f.write(buf_in)
    size = os.path.getsize(path)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, {size:,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating algorithm test input corpus.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  per-file: tier1={TIER1_TARGET}, tier2_shared={TIER2_SHARED}, '
          f'tier2_per_algo={TIER2_PER_ALGO}, total={TOTAL_RECORDS}')

    print('\nGenerating shared corpus (Tier 1 + Tier 2 shared)...')
    tier1 = ttp.gen_tier1_buffer_records(TIER1_TARGET, SHARED_SEED ^ 0xDEADBEEF)
    print(f'  tier 1: {len(tier1)} records')
    tier2_shared = ttp.gen_tier2_buffer_records(SHARED_SEED, TIER2_SHARED)
    print(f'  tier 2 shared: {len(tier2_shared)} records')

    shared = tier1 + tier2_shared
    assert len(shared) == SHARED_COUNT, f'{len(shared)} != {SHARED_COUNT}'

    for algo in range(8):
        algo_seed = SHARED_SEED ^ algo
        print(f'\nAlgorithm {algo} (per-algo seed=0x{algo_seed:08X}):')
        algo_unique = ttp.gen_tier2_buffer_records(algo_seed, TIER2_PER_ALGO)
        records = shared + algo_unique
        assert len(records) == TOTAL_RECORDS
        write_inputs_file(algo, records)

    print(f'\nDone. {TOTAL_RECORDS:,} records x 8 algorithms = '
          f'{TOTAL_RECORDS * 8:,} test inputs.')

if __name__ == '__main__':
    sys.exit(main() or 0)
