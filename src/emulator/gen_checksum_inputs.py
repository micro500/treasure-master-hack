#!/usr/bin/env python3
"""Generate checksum test inputs for the World-5 decryption blocks.

Each record is a buffer that mimics a "decrypted" block: N bytes total,
the last 2 of which are a stored 16-bit checksum. The harness loads the
buffer into RAM at $06A5..$06A5+N-1, calls the checksum routine
($EE54 / .byte $1C with X=2 or X=3), and snapshots:
  - the 16-bit running sum the routine computed (read from $42/$43)
  - the stored checksum (last 2 bytes of the buffer)
  - whether they match

Per-entry buffer sizes follow the actual decryption layout:
  - Entry 0 → checksum X=2: 114 bytes total, sums first 112, stored at last 2.
  - Entry 1 → checksum X=3:  83 bytes total, sums first  81, stored at last 2.

Tier 1 patterns construct buffers that exercise both pass and fail paths:
  - All-zero: sum=0, stored=00 00 → match.
  - All-FF: stored=FF FF, sum=(N-2)*0xFF mod 0x10000 → likely no-match.
  - Walking-1, walking-0, address-in-address.
  - Constructed-valid: random body + computed-correct stored checksum.
  - Off-by-one variants: stored = sum±1, sum±0xFF.

Tier 2 is bulk random bytes — most won't match (random checksum vs. random
sum), but that's fine; we just want to verify the routine's two outputs
agree with the buffer's bytes.

TMTV: test_type=0x09 (checksum), subtype=entry_idx (0 or 1).
"""

import os
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
TIER1_TARGET   = 2_000
TIER2_RANDOM   = 18_000
TOTAL_RECORDS  = TIER1_TARGET + TIER2_RANDOM    # 20,000

PRNG_SEED = 0x544D530B          # "TMS\x0B" — checksum test type

# Per-entry buffer geometry. SUM_LEN = bytes summed; the buffer also
# includes 2 trailing bytes (lo, hi) of stored checksum. RECORD_SIZE =
# SUM_LEN + 2.
SUM_LEN     = {0: 112, 1: 81}
RECORD_SIZE = {0: 114, 1: 83}

OUT_DIR = ttp.output_dir()


# ============================================================================
# Tier 1 patterns
# ============================================================================

def _solid_with_stored(N, val, stored):
    """N-byte buffer with body=`val` and trailing 2-byte stored checksum."""
    return bytes([val] * (N - 2)) + stored

def _walking_1(N):
    """Walking-1 over body, stored checksum = 00 00."""
    for body in ttp.walking_1_buffers(N - 2):
        yield body + b'\x00\x00'

def _walking_0(N):
    """Walking-0 over body, stored checksum = FF FF."""
    for body in ttp.walking_0_buffers(N - 2):
        yield body + b'\xFF\xFF'

def _byte_index(N):
    """Address-in-address × stored ∈ {00 00, FF FF}."""
    for body in ttp.byte_index_buffers(N - 2):
        yield body + b'\x00\x00'
        yield body + b'\xFF\xFF'

def _constructed_valid(N, prng):
    """Random body with stored checksum = computed sum (so match=YES)."""
    body = prng.bytes(N - 2)
    s = sum(body) & 0xFFFF
    return body + bytes([s & 0xFF, (s >> 8) & 0xFF])

def _off_by_one(N, prng):
    """Random body with stored = computed_sum + delta. delta=1 / -1 / 0xFF / -0xFF."""
    body = prng.bytes(N - 2)
    s = sum(body) & 0xFFFF
    for delta in (1, 0xFFFF, 0xFF, 0xFF01):
        d = (s + delta) & 0xFFFF
        yield body + bytes([d & 0xFF, (d >> 8) & 0xFF])

def tier1_records(N):
    sum_len = N - 2

    # PRIORITY 1: positive-pass tests (match=1). 100 records with random
    # body and a correctly-computed stored checksum. Tests the success
    # path through the routine (where the lo and hi compares both pass).
    pass_rng = ttp.Xoshiro256(PRNG_SEED ^ N ^ 0xCAFE)
    for _ in range(100):
        yield _constructed_valid(N, pass_rng)

    # PRIORITY 2: near-miss failures (stored = computed ± delta). Tests
    # that the routine correctly distinguishes match from off-by-tiny.
    # 60 sets × 4 deltas = 240 records.
    for _ in range(60):
        yield from _off_by_one(N, pass_rng)

    # PRIORITY 3: solid byte values × stored variants. Includes one match
    # (solid 0x00 / stored 00 00) and otherwise tests sum-saturation
    # boundaries for various solid input values.
    for val in (0x00, 0xFF, 0x55, 0xAA, 0x01, 0x80, 0x7F):
        for stored in (b'\x00\x00', b'\xFF\xFF', b'\x55\xAA', b'\xAA\x55'):
            yield _solid_with_stored(N, val, stored)

    # PRIORITY 4: address-in-address.
    yield from _byte_index(N)

    # PRIORITY 5: walking-1/0 across all bits. Bulk coverage; truncates
    # depending on TIER1_TARGET.
    yield from _walking_1(N)
    yield from _walking_0(N)


def gen_tier1(N):
    records = list(tier1_records(N))
    if len(records) > TIER1_TARGET:
        return records[:TIER1_TARGET]
    pad_rng = ttp.Xoshiro256(PRNG_SEED ^ N ^ 0xDEADBEEF)
    while len(records) < TIER1_TARGET:
        records.append(pad_rng.bytes(N))
    return records


# ============================================================================
# Tier 2 — bulk random
# ============================================================================

def gen_tier2(N, count):
    rng = ttp.Xoshiro256(PRNG_SEED ^ N)
    return [rng.bytes(N) for _ in range(count)]


# ============================================================================
# Output writer
# ============================================================================

def write_inputs_file(entry_idx, records):
    N = RECORD_SIZE[entry_idx]
    path = os.path.join(OUT_DIR, f'checksum{entry_idx}.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_CHECKSUM, entry_idx,
                              N, len(records))
        for rec in records:
            assert len(rec) == N
            f.write(rec)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, '
          f'{os.path.getsize(path):,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating checksum test inputs.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  tier1={TIER1_TARGET}, tier2={TIER2_RANDOM}, total={TOTAL_RECORDS}')

    for entry_idx in (0, 1):
        N = RECORD_SIZE[entry_idx]
        sum_len = SUM_LEN[entry_idx]
        print(f'\nEntry {entry_idx} (sum {sum_len} bytes, total {N} bytes):')

        tier1 = gen_tier1(N)
        tier2 = gen_tier2(N, TIER2_RANDOM)
        records = tier1 + tier2
        assert len(records) == TOTAL_RECORDS
        write_inputs_file(entry_idx, records)

    print('\nDone.')

if __name__ == '__main__':
    sys.exit(main() or 0)
