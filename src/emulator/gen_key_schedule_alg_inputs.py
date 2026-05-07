#!/usr/bin/env python3
"""Generate Stage 9 (4-byte schedule-algorithm) test inputs.

Stage 9 covers the 8 algorithms (routine IDs $27..$2E) that operate on the
4 bytes at $0191..$0194 and read the 1-byte map index at $00F3
(current_map). They are pure arithmetic — no RNG involvement.

Produces 8 files: key_schedule_alg_0.inputs.bin .. key_schedule_alg_7.inputs.bin in TMTV format.
Each file contains:
  - 32-byte TMTV header (test_type=0x02 schedule_alg, subtype=algo_id,
    record_size=5, record_kind=0x01)
  - 20,000 records of 5 bytes each:
      [4] state_in   (initial $0191..$0194)
      [1] map_id     (current_map at $00F3)

The first SHARED_COUNT (= 10,000) records are byte-identical across all 8
files so cross-algorithm oracles (e.g. dispatch-aliasing checks) can zip
records 0..SHARED_COUNT-1 across two files.

Layout within each file:
  records 0      .. 2,999       Tier 1 hand-crafted    (shared)
  records 3,000  .. 9,999       Tier 2 random shared   (shared)
  records 10,000 .. 19,999      Tier 2 random per-algo (unique)
"""

import os
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

SHARED_SEED = 0x544D5304        # "TMS\x04" — schedule_alg test type

OUT_DIR = ttp.output_dir()


# ============================================================================
# Tier 1 — hand-crafted patterned records
# ============================================================================

# Map IDs to stress. Includes the natural in-game range (Stage 8 dispatch
# uses map>>4 to index $0191..$0194, so 0..0x3F is the meaningful gameplay
# range), boundary values, and a smattering above $40 to confirm Stage 9
# itself is range-agnostic.
MAP_STRESS = [
    0x00, 0x01, 0x02, 0x07, 0x08, 0x0F, 0x10, 0x1F,
    0x20, 0x2F, 0x30, 0x3E, 0x3F,                       # boundary at $40
    0x40, 0x41, 0x55, 0x7F, 0x80, 0xAA, 0xFE, 0xFF,
]

STATE_PATTERNS = [
    bytes([0x00, 0x00, 0x00, 0x00]),
    bytes([0xFF, 0xFF, 0xFF, 0xFF]),
    bytes([0x55, 0x55, 0x55, 0x55]),
    bytes([0xAA, 0xAA, 0xAA, 0xAA]),
    bytes([0x01, 0x02, 0x02, 0x08]),
    bytes([0x10, 0x20, 0x40, 0x80]),
    bytes([0xDE, 0xAD, 0xBE, 0xEF]),     # capture from password_pipeline.md trace
    bytes([0x2C, 0xA5, 0xB4, 0x2D]),     # canonical key bytes 0..3 from the captured run
    bytes([0x80, 0x00, 0x00, 0x00]),
    bytes([0x00, 0x80, 0x00, 0x00]),
    bytes([0x00, 0x00, 0x80, 0x00]),
    bytes([0x00, 0x00, 0x00, 0x80]),
    bytes([0x01, 0x00, 0x00, 0x00]),
    bytes([0x00, 0x01, 0x00, 0x00]),
    bytes([0x00, 0x00, 0x01, 0x00]),
    bytes([0x00, 0x00, 0x00, 0x01]),
    bytes([0x7F, 0x80, 0x7F, 0x80]),
    bytes([0xFE, 0x01, 0xFE, 0x01]),
]

def _solid_x_map():
    """Each solid/patterned state crossed with each stress map. ~18 * 21 = 378."""
    for state in STATE_PATTERNS:
        for m in MAP_STRESS:
            yield state, m

_WALK_MAP_FIXED = (0x00, 0x01, 0x3F, 0x40, 0xFF)

def _walking_1_state():
    """Walking-1 across the 32 bits of state, fixed map values. 32 * 5 = 160."""
    for buf in ttp.walking_1_buffers(4):
        for m in _WALK_MAP_FIXED:
            yield buf, m

def _walking_0_state():
    """Walking-0 across the 32 bits of state, fixed map values. 32 * 5 = 160."""
    for buf in ttp.walking_0_buffers(4):
        for m in _WALK_MAP_FIXED:
            yield buf, m

def _full_map_sweep_zero_state():
    """Every map_id 0..255 with state = 0 — isolates the map-only contribution. 256."""
    for m in range(256):
        yield bytes([0, 0, 0, 0]), m

def _full_map_sweep_ff_state():
    """Every map_id 0..255 with state = FF — isolates carry behaviour. 256."""
    for m in range(256):
        yield bytes([0xFF, 0xFF, 0xFF, 0xFF]), m

def _byte_index():
    """Address-in-address × MAP_STRESS. 2 * 21 = 42."""
    for state in ttp.byte_index_buffers(4):
        for m in MAP_STRESS:
            yield state, m

def tier1_records():
    yield from _solid_x_map()
    yield from _walking_1_state()
    yield from _walking_0_state()
    yield from _full_map_sweep_zero_state()
    yield from _full_map_sweep_ff_state()
    yield from _byte_index()

def gen_tier1():
    records = list(tier1_records())
    if len(records) > TIER1_TARGET:
        return records[:TIER1_TARGET]
    pad_rng = ttp.Xoshiro256(SHARED_SEED ^ 0xDEADBEEF)
    while len(records) < TIER1_TARGET:
        records.append((pad_rng.bytes(4), pad_rng.u8()))
    return records


# ============================================================================
# Tier 2 — bulk random
# ============================================================================

def gen_tier2(prng_seed, count):
    rng = ttp.Xoshiro256(prng_seed)
    out = []
    for _ in range(count):
        out.append((rng.bytes(4), rng.u8()))
    return out


# ============================================================================
# Output writer
# ============================================================================

INPUTS_RECORD_SIZE = 5      # [4] state_in + [1] map_id

def write_inputs_file(algorithm_id, records):
    path = os.path.join(OUT_DIR, f'key_schedule_alg_{algorithm_id}.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_KS_ALG, algorithm_id,
                              INPUTS_RECORD_SIZE, len(records),
                              shared_count=SHARED_COUNT)
        for state, map_id in records:
            assert len(state) == 4
            f.write(state)
            f.write(bytes([map_id]))
    size = os.path.getsize(path)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, {size:,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating Stage 9 schedule_alg test input corpus.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  per-file: tier1={TIER1_TARGET}, tier2_shared={TIER2_SHARED}, '
          f'tier2_per_algo={TIER2_PER_ALGO}, total={TOTAL_RECORDS}')

    print('\nGenerating shared corpus (Tier 1 + Tier 2 shared)...')
    tier1 = gen_tier1()
    print(f'  tier 1: {len(tier1)} records')
    tier2_shared = gen_tier2(SHARED_SEED, TIER2_SHARED)
    print(f'  tier 2 shared: {len(tier2_shared)} records')

    shared = tier1 + tier2_shared
    assert len(shared) == SHARED_COUNT, f'{len(shared)} != {SHARED_COUNT}'

    for algo in range(8):
        algo_seed = SHARED_SEED ^ algo
        print(f'\nAlgorithm {algo} (per-algo seed=0x{algo_seed:08X}):')
        algo_unique = gen_tier2(algo_seed, TIER2_PER_ALGO)
        records = shared + algo_unique
        assert len(records) == TOTAL_RECORDS
        write_inputs_file(algo, records)

    print(f'\nDone. {TOTAL_RECORDS:,} records x 8 algorithms = '
          f'{TOTAL_RECORDS * 8:,} test inputs.')

if __name__ == '__main__':
    sys.exit(main() or 0)
