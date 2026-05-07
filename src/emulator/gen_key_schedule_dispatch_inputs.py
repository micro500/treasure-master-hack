#!/usr/bin/env python3
"""Generate Stage 8 4-byte dispatcher test inputs.

Tests the full $0:8255 → $0:828B path: gate on map_flags[map] & $80, gate on
the $0150 one-shot bitmap (always cleared by the harness so this gate
passes for every test), selector = ($0191[map>>4] >> 2) & 7, dispatch the
chosen 4-byte algorithm, and snapshot the resulting $0191..$0194.

Inputs per record (5 bytes):
  [4] state_in      initial $0191..$0194
  [1] map_id        current_map at $00F3, range 0..0x3F

Map IDs are restricted to 0..0x3F because Stage 8's selector indexes
$0191..$0194 via map>>4, and the one-shot bitmap at $0150 is similarly
keyed on map>>3; map_id >= 0x40 would index outside both regions and
cause out-of-bounds writes (potentially into the password buffer at
$016A). Real gameplay never reaches map_id >= 0x40.

Note: we intentionally test ALL map IDs in 0..0x3F regardless of whether
map_flags[map] & $80 is set. For maps where the gate would skip, the
ground-truth output is "state_out == state_in, ran=False" — that is
itself a valid test of the dispatcher's gating behavior.
"""

import os
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
TIER1_TARGET   = 3_200
TIER2_RANDOM   = 16_800
TOTAL_RECORDS  = TIER1_TARGET + TIER2_RANDOM      # 20,000

PRNG_SEED = 0x544D5305       # "TMS\x05" — schedule_key_schedule_dispatch test type

OUT_DIR = ttp.output_dir()

MAP_RANGE = range(0x40)     # 0..0x3F inclusive


# ============================================================================
# Tier 1 — hand-crafted patterned records
# ============================================================================

# Cover every selector value (0..7) at every byte position. The selector is
# bits 2..4 of $0191[map>>4], so to force selector=k we set the chosen byte
# to (k<<2). Combined with ranged map_id, this exercises every algorithm at
# every map-bank.
def _selector_sweep():
    """For each selector value 0..7, place it in each of the 4 bytes,
    crossed with all 64 maps. 8 * 4 * 64 = 2048 records."""
    for selector in range(8):
        sel_byte = selector << 2
        for byte_idx in range(4):
            state = bytearray(4)
            state[byte_idx] = sel_byte
            for m in MAP_RANGE:
                yield bytes(state), m

SOLID_STATES = [
    bytes([0x00] * 4),
    bytes([0xFF] * 4),
    bytes([0x55] * 4),
    bytes([0xAA] * 4),
    bytes([0xDE, 0xAD, 0xBE, 0xEF]),
    bytes([0x2C, 0xA5, 0xB4, 0x2D]),
    bytes([0x80, 0x80, 0x80, 0x80]),
    bytes([0x7F, 0x7F, 0x7F, 0x7F]),
    bytes([0x01, 0x02, 0x04, 0x08]),
    bytes([0x10, 0x20, 0x40, 0x80]),
    bytes([0xFC, 0xFC, 0xFC, 0xFC]),  # selector=7 in every byte
    bytes([0x1C, 0x1C, 0x1C, 0x1C]),  # selector=7 (bits 2..4 = 111)
]

def _solid_x_map():
    """Solid/notable states crossed with every map. 12 * 64 = 768."""
    for state in SOLID_STATES:
        for m in MAP_RANGE:
            yield state, m

# Boundary maps (one per quadrant of $0191..$0194 + edges).
BOUNDARY_MAPS = [
    0x00, 0x01, 0x0F,         # quadrant 0 (uses $0191): edges
    0x10, 0x11, 0x1F,         # quadrant 1 (uses $0192)
    0x20, 0x21, 0x2F,         # quadrant 2 (uses $0193)
    0x30, 0x31, 0x3E, 0x3F,   # quadrant 3 (uses $0194)
]

def _walking_state_at_boundaries():
    """Walking-1 across 32 bits of state, at boundary maps. 32 * 13 = 416."""
    for s in ttp.walking_1_buffers(4):
        for m in BOUNDARY_MAPS:
            yield s, m

def tier1_records():
    yield from _selector_sweep()
    yield from _solid_x_map()
    yield from _walking_state_at_boundaries()

def gen_tier1():
    records = list(tier1_records())
    if len(records) > TIER1_TARGET:
        return records[:TIER1_TARGET]
    pad_rng = ttp.Xoshiro256(PRNG_SEED ^ 0xDEADBEEF)
    while len(records) < TIER1_TARGET:
        state = pad_rng.bytes(4)
        m = pad_rng.u8() & 0x3F
        records.append((state, m))
    return records


# ============================================================================
# Tier 2 — bulk random
# ============================================================================

def gen_tier2(count):
    rng = ttp.Xoshiro256(PRNG_SEED)
    out = []
    for _ in range(count):
        out.append((rng.bytes(4), rng.u8() & 0x3F))
    return out


# ============================================================================
# Output writer
# ============================================================================

INPUTS_RECORD_SIZE = 5

def write_inputs_file(records):
    path = os.path.join(OUT_DIR, 'key_schedule_dispatch.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_KS_DISPATCH, 0,
                              INPUTS_RECORD_SIZE, len(records))
        for state, m in records:
            assert len(state) == 4
            assert 0 <= m < 0x40
            f.write(state)
            f.write(bytes([m]))
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, {os.path.getsize(path):,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating Stage 8 4-byte dispatcher test input corpus.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  tier1={TIER1_TARGET}, tier2_random={TIER2_RANDOM}, total={TOTAL_RECORDS}')
    print(f'  map_id range: 0..0x3F')

    tier1 = gen_tier1()
    print(f'  tier 1: {len(tier1)} records')
    tier2 = gen_tier2(TIER2_RANDOM)
    print(f'  tier 2: {len(tier2)} records')

    records = tier1 + tier2
    assert len(records) == TOTAL_RECORDS
    write_inputs_file(records)
    print(f'\nDone. {TOTAL_RECORDS:,} (state, map_id) records.')

if __name__ == '__main__':
    sys.exit(main() or 0)
