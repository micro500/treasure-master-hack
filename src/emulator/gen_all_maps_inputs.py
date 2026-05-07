#!/usr/bin/env python3
"""Generate 128-byte in-game chain test inputs.

Each record is 8 bytes — a candidate password key. The harness:
  1. Runs Stage 6 expansion on the 8-byte key → produces 128-byte buffer
     and the initial $0191..$0194 = key[0..3].
  2. Threads (4 bytes, 128 bytes) through 28 in-game mutation events
     (event 10 silently chained, events 14/20 split via 2-savestate
     pattern for their delayed 128-byte step).
  3. Final test result = the 128-byte buffer after the last event.

Output: chain_full.inputs.bin in TMTV format:
  - 32-byte header (test_type=0x07 schedule_in_game_chain_full,
    subtype=27, record_size=8, record_kind=0x01)
  - 3,000 records of 8 bytes each.
"""

import os
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
TIER1_TARGET   = 300
TIER2_RANDOM   = 2_700
TOTAL_RECORDS  = TIER1_TARGET + TIER2_RANDOM    # 3,000

PRNG_SEED   = 0x544D5309        # "TMS\x09" — schedule_in_game_chain_full
SUBTYPE_N   = 27                # number of 128-byte mutations applied (= 4-byte emitted events)

OUT_DIR = ttp.output_dir()


# ============================================================================
# Tier 1 — hand-crafted 8-byte keys
# ============================================================================

SINGLE_BYTE_VALUES = (0x01, 0x02, 0x04, 0x08, 0x10, 0x80, 0xAA, 0xFF)

def _byte_index_extras():
    """Two reverse variants beyond the standard buf[i]=i / buf[i]=~i. 2 records."""
    yield bytes(7 - i for i in range(8))       # 07 06 05 04 03 02 01 00
    yield bytes(0xF8 + i for i in range(8))    # F8 F9 FA FB ...

def _seed_pair_stress():
    """Vary key[0]/key[1] (these seed the Stage 6 RNG) with a fixed tail.
    7 * 7 = 49 records."""
    pairs = [0x00, 0x01, 0x55, 0x7F, 0x80, 0xAA, 0xFF]
    for k0 in pairs:
        for k1 in pairs:
            yield bytes([k0, k1, 0, 0, 0, 0, 0, 0])

def _real_captures():
    """Recognizable test fingerprints. 4 records."""
    yield bytes([0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE])
    yield bytes([0x2C, 0xA5, 0xB4, 0x2D, 0x00, 0x00, 0x00, 0x00])  # gameplay key[0..3] + zero tail
    yield bytes([0x2C, 0xA5, 0xB4, 0x2D, 0xFF, 0xFF, 0xFF, 0xFF])  # gameplay key[0..3] + FF tail
    yield bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0])

def tier1_records():
    yield from ttp.solid_buffers(8)
    yield from ttp.walking_1_buffers(8)
    yield from ttp.walking_0_buffers(8)
    yield from ttp.single_byte_nonzero_buffers(8, SINGLE_BYTE_VALUES)
    # Original byte_index emitted: range, reverse, ~i, 0xF8+i. Centralized
    # primitive yields {range, ~i}; keep the two extras local.
    yield from ttp.byte_index_buffers(8)
    yield from _byte_index_extras()
    yield from _seed_pair_stress()
    yield from _real_captures()

def gen_tier1():
    records = list(tier1_records())
    if len(records) > TIER1_TARGET:
        return records[:TIER1_TARGET]
    pad_rng = ttp.Xoshiro256(PRNG_SEED ^ 0xDEADBEEF)
    while len(records) < TIER1_TARGET:
        records.append(pad_rng.bytes(8))
    return records


# ============================================================================
# Tier 2 — bulk random
# ============================================================================

def gen_tier2(count):
    rng = ttp.Xoshiro256(PRNG_SEED)
    return [rng.bytes(8) for _ in range(count)]


# ============================================================================
# Output writer
# ============================================================================

INPUTS_RECORD_SIZE = 8

def write_inputs_file(records):
    path = os.path.join(OUT_DIR, 'all_maps.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_WC_ALL_MAPS, SUBTYPE_N,
                              INPUTS_RECORD_SIZE, len(records))
        for rec in records:
            assert len(rec) == 8
            f.write(rec)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, '
          f'{os.path.getsize(path):,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating 128-byte in-game chain test inputs.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  tier1={TIER1_TARGET}, tier2={TIER2_RANDOM}, total={TOTAL_RECORDS}')

    tier1 = gen_tier1()
    print(f'  tier 1: {len(tier1)} records')
    tier2 = gen_tier2(TIER2_RANDOM)
    print(f'  tier 2: {len(tier2)} records')

    records = tier1 + tier2
    assert len(records) == TOTAL_RECORDS
    write_inputs_file(records)
    print(f'\nDone. {TOTAL_RECORDS} records, each will be threaded through '
          f'Stage 6 + 28 events (27 emitted), producing a 128-byte final result.')

if __name__ == '__main__':
    sys.exit(main() or 0)
