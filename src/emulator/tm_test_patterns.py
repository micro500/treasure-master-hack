"""Shared TMTV test-corpus pattern generators.

Centralizes the tier-1 buffer/RNG patterns, the bulk-random Xoshiro256
generator, and the TMTV header writer so every downstream gen_*.py
emits headers with identical layout and consistent constants.

The tier-1 generators yield (rng_seed_u16, buf128) pairs in a fixed
deterministic order. Adding entries here changes the contents of every
downstream corpus — be careful and re-run all generators when modifying.
"""

import os
import struct


# ============================================================================
# TMTV header constants and writer (shared across all gen_*.py)
# ============================================================================

TMTV_MAGIC   = b'TMTV'
TMTV_VERSION = 1

# test_type values per ALGORITHM_TEST_PLAN.md
TEST_TYPE_KS_DISPATCH  = 0x01
TEST_TYPE_KS_ALG       = 0x02
TEST_TYPE_KS_ALL_MAPS  = 0x03
TEST_TYPE_EXPANSION    = 0x04
TEST_TYPE_WC_ALG       = 0x05
TEST_TYPE_WC_ALG_MULTI = 0x06
TEST_TYPE_WC_ALL_MAPS  = 0x07
TEST_TYPE_DECRYPT      = 0x08
TEST_TYPE_CHECKSUM     = 0x09

RECORD_KIND_FULL        = 0x00
RECORD_KIND_INPUTS_ONLY = 0x01

TMTV_HEADER_SIZE = 32
_TMTV_HEADER_FMT = '<4sHBBHIIB13s'


def write_tmtv_header(f, test_type, subtype, record_size, record_count,
                      shared_count=0,
                      record_kind=RECORD_KIND_INPUTS_ONLY):
    """Write a 32-byte TMTV header to file-like `f`.

    Layout (per ALGORITHM_TEST_PLAN.md):
      0  4  magic           "TMTV"
      4  2  version         1
      6  1  test_type
      7  1  subtype
      8  2  record_size
     10  4  record_count
     14  4  shared_count
     18  1  record_kind
     19 13  reserved (zeros)
    """
    header = struct.pack(
        _TMTV_HEADER_FMT,
        TMTV_MAGIC,
        TMTV_VERSION,
        test_type,
        subtype,
        record_size,
        record_count,
        shared_count,
        record_kind,
        b'\x00' * 13,
    )
    assert len(header) == TMTV_HEADER_SIZE
    f.write(header)


def output_dir():
    """Canonical input-file directory: <emulator>/test_inputs/."""
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'test_inputs')


# ============================================================================
# Xoshiro256** PRNG (with SplitMix64 seeding). Bit-deterministic.
# ============================================================================

class Xoshiro256:
    MASK = 0xFFFFFFFFFFFFFFFF

    def __init__(self, seed):
        self.s = list(self._splitmix(seed, 4))

    @staticmethod
    def _splitmix(seed, count):
        z = seed & Xoshiro256.MASK
        for _ in range(count):
            z = (z + 0x9E3779B97F4A7C15) & Xoshiro256.MASK
            z2 = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & Xoshiro256.MASK
            z2 = ((z2 ^ (z2 >> 27)) * 0x94D049BB133111EB) & Xoshiro256.MASK
            yield z2 ^ (z2 >> 31)

    @staticmethod
    def _rotl(x, k):
        return ((x << k) | (x >> (64 - k))) & Xoshiro256.MASK

    def next64(self):
        s = self.s
        result = (self._rotl((s[1] * 5) & self.MASK, 7) * 9) & self.MASK
        t = (s[1] << 17) & self.MASK
        s[2] ^= s[0]
        s[3] ^= s[1]
        s[1] ^= s[2]
        s[0] ^= s[3]
        s[2] ^= t
        s[3] = self._rotl(s[3], 45)
        return result

    def u8(self):
        return self.next64() & 0xFF

    def u16(self):
        return self.next64() & 0xFFFF

    def bytes(self, n):
        out = bytearray()
        v = 0
        bits_left = 0
        while len(out) < n:
            if bits_left == 0:
                v = self.next64()
                bits_left = 64
            out.append(v & 0xFF)
            v >>= 8
            bits_left -= 8
        return bytes(out)


# ============================================================================
# Buffer pattern primitives (size-parameterized; shared across all corpora)
# ============================================================================

# Group A solid bytes per ALGORITHM_TEST_PLAN.md ("RAM zero/one" patterns).
SOLID_BYTES = [0x00, 0xFF, 0x01, 0x80, 0x7F, 0x55, 0xAA]


def solid_buffers(n, values=SOLID_BYTES):
    """Yield N-byte buffers, each filled with one of `values`."""
    for v in values:
        yield bytes([v] * n)


def walking_1_buffers(n):
    """Yield n*8 buffers, each with exactly one bit set."""
    for bit in range(n * 8):
        buf = bytearray(n)
        buf[bit // 8] = 1 << (bit % 8)
        yield bytes(buf)


def walking_0_buffers(n):
    """Yield n*8 buffers, each with exactly one bit cleared from all-FF."""
    for bit in range(n * 8):
        buf = bytearray([0xFF] * n)
        buf[bit // 8] &= (~(1 << (bit % 8))) & 0xFF
        yield bytes(buf)


def single_byte_nonzero_buffers(n, values):
    """Yield buffers with exactly one non-zero byte, sweeping idx × value."""
    for idx in range(n):
        for v in values:
            buf = bytearray(n)
            buf[idx] = v
            yield bytes(buf)


def byte_index_buffers(n):
    """Address-in-address: buf[i] = i, then buf[i] = ~i. 2 records."""
    yield bytes(i & 0xFF for i in range(n))
    yield bytes((~i) & 0xFF for i in range(n))


# ============================================================================
# RNG seed stress list (Group F — folded into A/B as a cross-product)
# ============================================================================

RNG_STRESS_SEEDS = [
    (0x00, 0x00),     # lowest
    (0xFF, 0xFF),     # highest
    (0x01, 0x00),     # asymmetric (lo only)
    (0x00, 0x01),     # asymmetric (hi only)
    (0x55, 0xAA),     # bit-pattern stress
    (0xAA, 0x55),     # bit-pattern stress (inverse)
    (0x12, 0x34),     # arbitrary regression anchor
]


def _seed_u16(lo, hi):
    return (hi << 8) | lo


# ============================================================================
# Tier 1 — hand-crafted patterned buffer + RNG records
# ============================================================================

def _group_a():
    """Solid patterns crossed with all stress seeds. 7 * 7 = 49 records."""
    for buf in solid_buffers(128):
        for lo, hi in RNG_STRESS_SEEDS:
            yield _seed_u16(lo, hi), buf

def _group_b():
    """Checkerboards crossed with stress seeds. 7 * 7 = 49 records."""
    patterns = [
        b'\xAA\x55' * 64,
        b'\x55\xAA' * 64,
        b'\xAA\xAA\x55\x55' * 32,
        b'\x55\x55\xAA\xAA' * 32,
        (b'\xAA' * 4 + b'\x55' * 4) * 16,
        (b'\x55' * 4 + b'\xAA' * 4) * 16,
        bytes([0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80]) * 16,
    ]
    for buf in patterns:
        for lo, hi in RNG_STRESS_SEEDS:
            yield _seed_u16(lo, hi), buf

def _group_c_walking_1():
    """Walking-1: 1024 records, one bit set per record."""
    for buf in walking_1_buffers(128):
        yield _seed_u16(0x12, 0x34), buf

def _group_c_walking_0():
    """Walking-0: 1024 records, one bit cleared per record."""
    for buf in walking_0_buffers(128):
        yield _seed_u16(0x12, 0x34), buf

def _group_d_galpat():
    """Address-in-address style. 6 records."""
    addr, addr_inv = byte_index_buffers(128)
    yield _seed_u16(0x12, 0x34), addr
    yield _seed_u16(0x12, 0x34), addr_inv
    yield _seed_u16(0x00, 0x00), addr
    yield _seed_u16(0xFF, 0xFF), addr
    yield _seed_u16(0x00, 0x00), addr_inv
    yield _seed_u16(0xFF, 0xFF), addr_inv

def _group_e_boundary():
    """Single non-zero byte at significant indices, plus half-half splits."""
    indices = [0, 1, 2, 3, 0x40, 0x41, 0x7C, 0x7D, 0x7E, 0x7F]
    values  = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xFF, 0x55, 0xAA]
    for idx in indices:
        for val in values:
            buf = bytearray(128)
            buf[idx] = val
            yield _seed_u16(0x12, 0x34), bytes(buf)

    for lo, hi in [(0x00, 0xFF), (0xFF, 0x00), (0x55, 0xAA), (0xAA, 0x55)]:
        yield _seed_u16(0x12, 0x34), bytes([lo] * 64 + [hi] * 64)


def tier1_buffer_records():
    """Yield all tier-1 (rng_seed, buf128) pairs in deterministic order."""
    yield from _group_a()
    yield from _group_b()
    yield from _group_c_walking_1()
    yield from _group_c_walking_0()
    yield from _group_d_galpat()
    yield from _group_e_boundary()


def gen_tier1_buffer_records(target_count, pad_seed):
    """Materialize tier 1 and pad to target_count with deterministic random.

    pad_seed seeds the Xoshiro256 used to generate any padding records
    needed to reach target_count.
    """
    records = list(tier1_buffer_records())
    if len(records) > target_count:
        return records[:target_count]
    pad_rng = Xoshiro256(pad_seed)
    while len(records) < target_count:
        records.append((pad_rng.u16(), pad_rng.bytes(128)))
    return records


# ============================================================================
# Tier 2 — bulk random buffer + RNG records
# ============================================================================

def gen_tier2_buffer_records(prng_seed, count):
    rng = Xoshiro256(prng_seed)
    return [(rng.u16(), rng.bytes(128)) for _ in range(count)]
