#!/usr/bin/env python3
"""Generate Stage 9 in-game chain-replay test inputs.

Each record is just 4 bytes — the initial $0191..$0194 state. The harness
threads this through 28 in-game mutation events (events 2..29), emitting
27 logged outputs (event 10's intermediate is silently chained).

Output: key_schedule_all_maps.inputs.bin in TMTV format:
  - 32-byte header (test_type=0x03 schedule_key_schedule_all_maps, subtype=27,
    record_size=4, record_kind=0x01)
  - 20,000 records of 4 bytes each.

The first record is always 2C A5 B4 2D — the actual gameplay key bytes
0..3 from password_pipeline.md. The harness output for that record must
exactly match the user's expected 27-entry key schedule, validating the
harness end-to-end before any other records' outputs are trusted.
"""

import os
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
TIER1_TARGET   = 300
TIER2_RANDOM   = 2_700
TOTAL_RECORDS  = TIER1_TARGET + TIER2_RANDOM   # 3,000

PRNG_SEED    = 0x544D5308       # "TMS\x08" — schedule_key_schedule_all_maps test type
CHAIN_LENGTH = 27               # number of emitted in-game events

OUT_DIR = ttp.output_dir()


# ============================================================================
# Tier 1 — hand-crafted 4-byte inputs
# ============================================================================

# The known-good input MUST be first so the verifier can compare record 0
# against the user's expected schedule.
KNOWN_GOOD_KEY = bytes([0x2C, 0xA5, 0xB4, 0x2D])

SINGLE_BYTE_VALUES = (0x01, 0x02, 0x04, 0x03, 0x10, 0x20, 0x40, 0x80,
                      0x03, 0x0F, 0x55, 0xAA, 0x7F, 0xC0, 0xF0, 0xFF)

def _byte_index_extras():
    """Reversed and constant-byte variants beyond the standard buf[i]=i / ~i. 4 records."""
    yield bytes([0x03, 0x02, 0x01, 0x00])
    yield bytes([0xFC, 0xFD, 0xFE, 0xFF])
    yield bytes([0x0F, 0x0F, 0x0F, 0x0F])
    yield bytes([0xF0, 0xF0, 0xF0, 0xF0])

def _selector_sweep():
    """For each byte position, set that byte such that bits 2..4 hit each
    selector value 0..7. Stage 9 byte-picked dispatch uses
    (state[map>>4] >> 2) & 7, so this varies the chosen algorithm at
    each in-chain event independently of the other bytes.

    8 selectors * 4 positions * 4 "context" values for the other bytes
    = 128 records.
    """
    contexts = [0x00, 0xFF, 0xAA, 0x55]
    for selector in range(8):
        sel_byte = selector << 2          # bits 2..4 = selector
        for byte_idx in range(4):
            for ctx in contexts:
                buf = bytearray([ctx] * 4)
                buf[byte_idx] = sel_byte
                yield bytes(buf)

def _carry_boundaries():
    """Pairs around 7F/80 and 00/FF carry boundaries. 16 records."""
    boundary_pairs = [
        (0x7F, 0x00), (0x7F, 0xFF), (0x80, 0x00), (0x80, 0x80),
        (0xFF, 0x00), (0x00, 0xFF), (0xFF, 0xFF), (0x00, 0x00),
    ]
    for a, b in boundary_pairs:
        yield bytes([a, b, a, b])
        yield bytes([a, a, b, b])

def _real_captures():
    """The actual gameplay key, plus related captures from password_pipeline.md
    and a few common test fingerprints. Useful regression anchors."""
    yield bytes([0x2C, 0xA5, 0xB4, 0x2D])     # gameplay key bytes 0..3
    yield bytes([0xDE, 0xAD, 0xBE, 0xEF])
    yield bytes([0xCA, 0xFE, 0xBA, 0xBE])
    yield bytes([0x59, 0x7B, 0x8B, 0xDE])     # post-$2E example from doc
    yield bytes([0xDD, 0xEF, 0xAD, 0xBE])     # post-P2 example from doc

def tier1_records():
    """Concatenate all tier-1 generators in a fixed order. Record 0 must
    be the known-good key."""
    yield KNOWN_GOOD_KEY
    yield from ttp.solid_buffers(4)
    yield from ttp.walking_1_buffers(4)
    yield from ttp.walking_0_buffers(4)
    yield from ttp.single_byte_nonzero_buffers(4, SINGLE_BYTE_VALUES)
    yield from ttp.byte_index_buffers(4)
    yield from _byte_index_extras()
    yield from _selector_sweep()
    yield from _carry_boundaries()
    yield from _real_captures()

def gen_tier1():
    records = list(tier1_records())
    if len(records) > TIER1_TARGET:
        # Truncate but ensure record 0 is preserved.
        return records[:TIER1_TARGET]
    pad_rng = ttp.Xoshiro256(PRNG_SEED ^ 0xDEADBEEF)
    while len(records) < TIER1_TARGET:
        records.append(pad_rng.bytes(4))
    return records


# ============================================================================
# Tier 2 — bulk random
# ============================================================================

def gen_tier2(count):
    rng = ttp.Xoshiro256(PRNG_SEED)
    return [rng.bytes(4) for _ in range(count)]


# ============================================================================
# Output writer
# ============================================================================

INPUTS_RECORD_SIZE = 4

def write_inputs_file(records):
    path = os.path.join(OUT_DIR, 'key_schedule_all_maps.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_KS_ALL_MAPS, CHAIN_LENGTH,
                              INPUTS_RECORD_SIZE, len(records))
        for rec in records:
            assert len(rec) == 4
            f.write(rec)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, '
          f'{os.path.getsize(path):,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating in-game chain-replay test inputs.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  chain length: {CHAIN_LENGTH} emitted events')
    print(f'  tier1={TIER1_TARGET}, tier2={TIER2_RANDOM}, total={TOTAL_RECORDS}')

    tier1 = gen_tier1()
    print(f'  tier 1: {len(tier1)} records (first = known-good key 2C A5 B4 2D)')
    assert tier1[0] == KNOWN_GOOD_KEY, 'record 0 must be the known-good key'

    tier2 = gen_tier2(TIER2_RANDOM)
    print(f'  tier 2: {len(tier2)} records')

    records = tier1 + tier2
    assert len(records) == TOTAL_RECORDS
    write_inputs_file(records)
    print(f'\nDone. {TOTAL_RECORDS:,} records, each will be threaded through '
          f'{CHAIN_LENGTH} emitted events.')

if __name__ == '__main__':
    sys.exit(main() or 0)
