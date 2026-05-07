#!/usr/bin/env python3
"""Generate Stage 10 algorithm-chain test inputs.

A "chain" record applies a sequence of N Stage-10 algorithms in order to
the same 128-byte buffer + 16-bit RNG state, capturing only the final
state. Tests that a C++ chained-loop implementation matches the
single-algorithm path applied N times.

Produces 6 files: wc_alg_multi_2.inputs.bin .. wc_alg_multi_16.inputs.bin in TMTV format.

  - 32-byte TMTV header (test_type=0x06 schedule_chain, subtype=N,
    record_size = 130 + N, record_kind=0x01)
  - 20,000 records of (130 + N) bytes each:
      [2]   rng_seed_in
      [128] buffer_in
      [N]   alg_ids       (each byte 0..7; routine ID = 0x1F + id)

The FCEUX harness reads these and writes the full output corpus
(chain<N>.bin) with rng_state_out and buffer_out appended (record_size
becomes 260 + N).

Buffer + RNG patterns are shared with gen_test_inputs.py via
tm_test_patterns.py. The chain templates (the per-record alg_id
sequences) are specific to this corpus.
"""

import os
import struct
import sys

import tm_test_patterns as ttp

# ============================================================================
# Configuration
# ============================================================================
CHAIN_LENGTHS    = [2, 3, 4, 8, 11, 16]
TOTAL_RECORDS    = 20_000
TIER1_TARGET     = 3_000
TIER2_RANDOM     = TOTAL_RECORDS - TIER1_TARGET    # 17,000

# Each chain length file uses a distinct PRNG sequence so the random
# section diverges across files.
CHAIN_PRNG_SEED_BASE = 0x544D5307     # "TMS\x07" — schedule_chain test type

OUT_DIR = ttp.output_dir()


# ============================================================================
# Tier 1 — hand-crafted chain templates of length N
# ============================================================================

def tier1_chain_templates(N):
    """Yield bytes-of-length-N chain templates targeted at common impl bugs."""
    # Uniform chains: [k, k, ..., k] for k=0..7.
    # Tests "running one algo N times" — should match a single-algo run
    # iterated N times in the C++ harness.
    for k in range(8):
        yield bytes([k] * N)

    # Walking through all 8 algorithms (truncated/extended to N).
    yield bytes([i % 8 for i in range(N)])
    yield bytes([(7 - (i % 8)) for i in range(N)])

    # Alternating pairs. (alg 7 = NOT, no RNG draws — pairing it with
    # other algs detects RNG desync if NOT wrongly advances RNG.)
    for a, b in [(2, 5), (1, 4), (0, 6), (7, 0), (7, 3), (3, 6), (5, 2)]:
        yield bytes([a if i % 2 == 0 else b for i in range(N)])

    # Triplets that exercise carry/RNG interactions (1, 4 are ADC/SBC mirrors;
    # 2, 5 are the pair-rotators with single-rng-bit chain).
    yield bytes([(1, 4, 7)[i % 3] for i in range(N)])
    yield bytes([(2, 5, 7)[i % 3] for i in range(N)])

    # Mostly-NOTs with an occasional non-NOT. NOT is its own inverse, so
    # an even count of consecutive 7s should produce buffer-identity for
    # those positions; this exercises the C++ chain's accumulator.
    yield bytes([7 if i != N // 2 else 0 for i in range(N)])
    yield bytes([7 if i != 0 and i != N - 1 else 3 for i in range(N)])

    # All-zero (alg 0) and all-six (alg 6) — directional pair, so chains
    # of length 2*k of [0, 6] should... not return to identity (RNG
    # advances), but should be consistent across impls.
    yield bytes([0 if i % 2 == 0 else 6 for i in range(N)])

    # Boundary: leading/trailing run of one algo.
    if N >= 4:
        yield bytes([3] * (N - 1) + [7])
        yield bytes([7] + [3] * (N - 1))


def gen_tier1_chain_records(N, target):
    """Cross-product of tier1 buffer records × tier1 chain templates,
    capped at target. Padded with deterministic random if short.
    """
    bufs    = list(ttp.tier1_buffer_records())   # all tier1 buffer/rng pairs
    chains  = list(tier1_chain_templates(N))
    records = []

    # Cross-product, in order: each buffer paired with each chain.
    for rng_seed, buf in bufs:
        for chain in chains:
            records.append((rng_seed, buf, chain))
            if len(records) >= target:
                break
        if len(records) >= target:
            break

    if len(records) > target:
        records = records[:target]

    if len(records) < target:
        pad_rng = ttp.Xoshiro256(CHAIN_PRNG_SEED_BASE ^ N ^ 0xDEADBEEF)
        while len(records) < target:
            records.append((
                pad_rng.u16(),
                pad_rng.bytes(128),
                bytes(pad_rng.u8() & 7 for _ in range(N)),
            ))

    return records


# ============================================================================
# Tier 2 — bulk random
# ============================================================================

def gen_tier2_chain_records(N, count, prng_seed):
    rng = ttp.Xoshiro256(prng_seed)
    out = []
    for _ in range(count):
        out.append((
            rng.u16(),
            rng.bytes(128),
            bytes(rng.u8() & 7 for _ in range(N)),
        ))
    return out


# ============================================================================
# Output writer
# ============================================================================

def write_inputs_file(N, records):
    record_size = 130 + N
    path = os.path.join(OUT_DIR, f'wc_alg_multi_{N}.inputs.bin')
    with open(path, 'wb') as f:
        ttp.write_tmtv_header(f, ttp.TEST_TYPE_WC_ALG_MULTI, N,
                              record_size, len(records))
        for rng_in, buf_in, alg_ids in records:
            assert len(buf_in) == 128
            assert len(alg_ids) == N
            assert all(0 <= b <= 7 for b in alg_ids), 'alg_id out of range'
            f.write(struct.pack('<H', rng_in))
            f.write(buf_in)
            f.write(alg_ids)
    print(f'  wrote {os.path.basename(path)}: {len(records)} records, '
          f'{os.path.getsize(path):,} bytes')


# ============================================================================
# Main
# ============================================================================

def main():
    print('Generating Stage 10 algorithm-chain test input corpus.')
    print(f'  output dir: {OUT_DIR}')
    print(f'  chain lengths: {CHAIN_LENGTHS}')
    print(f'  per-file: tier1={TIER1_TARGET}, tier2={TIER2_RANDOM}, total={TOTAL_RECORDS}')

    for N in CHAIN_LENGTHS:
        print(f'\nChain length N={N}:')
        tier1 = gen_tier1_chain_records(N, TIER1_TARGET)
        tier2 = gen_tier2_chain_records(N, TIER2_RANDOM, CHAIN_PRNG_SEED_BASE ^ N)
        records = tier1 + tier2
        assert len(records) == TOTAL_RECORDS
        write_inputs_file(N, records)

    total = TOTAL_RECORDS * len(CHAIN_LENGTHS)
    print(f'\nDone. {TOTAL_RECORDS:,} records x {len(CHAIN_LENGTHS)} files = '
          f'{total:,} chain test inputs.')

if __name__ == '__main__':
    sys.exit(main() or 0)
