#!/usr/bin/env python3
"""Generate Stage 6 expansion test inputs.

Stage 6 (`$5:B112`) takes the 8-byte key at $0200..$0207, reseeds the global
RNG ($0436/$0437) from key[0]/key[1], and fills $0208..$027F via the
recurrence buf[Y] = buf[Y-8] + rng() for Y=8..127. There is NO separate RNG
seed input — it is derived from the key — so each record is just 8 bytes.

Output: expansion.inputs.bin, TMTV format:
  - 32-byte header (test_type=0x04, subtype=0, record_size=8, record_kind=0x01)
  - record_count records of 8 bytes each (the key)

The FCEUX harness consumes this and writes expansion.bin (record_kind=0x00,
136-byte records: [8] key_in + [128] buffer_out).
"""

import os
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
TIER1_TARGET   = 1_000
TIER2_RANDOM   = 19_000
TOTAL_RECORDS  = TIER1_TARGET + TIER2_RANDOM     # 20,000

PRNG_SEED = 0x544D5306        # "TMS" + stage 6

OUT_DIR = ttp.output_dir()


# ============================================================================
# Tier 1 — hand-crafted keys
# ============================================================================

PAIR_STRESS = [0x00, 0x01, 0x55, 0x7F, 0x80, 0xAA, 0xFF]
SINGLE_BYTE_VALUES = (0x01, 0x55, 0xAA, 0xFF)

def _seed_pair_stress():
    """Vary (key[0], key[1]) since they alone seed the internal RNG.
    Rest of key is zero. 7 * 7 = 49 records."""
    for k0 in PAIR_STRESS:
        for k1 in PAIR_STRESS:
            yield bytes([k0, k1, 0, 0, 0, 0, 0, 0])

def _seed_pair_stress_ff_tail():
    """Same pair stress, but with non-zero tail (0xFF) so the recurrence
    propagates a non-trivial input. 49 records."""
    for k0 in PAIR_STRESS:
        for k1 in PAIR_STRESS:
            yield bytes([k0, k1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF])

def tier1_records():
    yield from ttp.solid_buffers(8)
    yield from ttp.walking_1_buffers(8)
    yield from ttp.walking_0_buffers(8)
    yield from ttp.single_byte_nonzero_buffers(8, SINGLE_BYTE_VALUES)
    yield from _seed_pair_stress()
    yield from _seed_pair_stress_ff_tail()
    yield from ttp.byte_index_buffers(8)

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

INPUTS_RECORD_SIZE = 8        # [8] key_in

def write_inputs_file(records):
    path = os.path.join(OUT_DIR, 'expansion.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_EXPANSION, 0,
                              INPUTS_RECORD_SIZE, len(records))
        for key in records:
            assert len(key) == 8
            f.write(key)
    size = os.path.getsize(path)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, {size:,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating Stage 6 expansion test input corpus.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  tier1={TIER1_TARGET}, tier2_random={TIER2_RANDOM}, total={TOTAL_RECORDS}')

    tier1 = gen_tier1()
    print(f'  tier 1: {len(tier1)} records')
    tier2 = gen_tier2(TIER2_RANDOM)
    print(f'  tier 2: {len(tier2)} records')

    records = tier1 + tier2
    assert len(records) == TOTAL_RECORDS

    # Sanity: no duplicate-record check (8-byte space is huge; collisions
    # extremely unlikely from a 20k random draw).
    write_inputs_file(records)
    print(f'\nDone. {TOTAL_RECORDS:,} 8-byte keys.')

if __name__ == '__main__':
    sys.exit(main() or 0)
