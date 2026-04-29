#include <stdio.h>
#include <stdint.h>
#include "alignment2.h"
#include "rng_obj.h"

RNG::RNG()
{
    _rng_table_ref = generate_rng_table();
}

std::shared_ptr<void> RNG::generate_rng_table()
{
    auto s = rng_table.lock();
    if (s) return s;
    rng_table.ptr = static_cast<uint16_t*>(aligned_malloc(256 * 256 * sizeof(uint16_t), 64));
    for (int i = 0; i <= 0xFF; i++)
    {
        for (int j = 0; j <= 0xFF; j++)
        {
            unsigned int rngA = i;
            unsigned int rngB = j;

            uint8_t carry = 0;

            rngB = (rngB + rngA) & 0xFF;

            rngA = rngA + 0x89;
            carry = rngA > 0xFF ? 1 : 0;
            rngA = rngA & 0xFF;

            rngB = rngB + 0x2A + carry;
            carry = rngB > 0xFF ? 1 : 0;
            rngB = rngB & 0xFF;

            rngA = rngA + 0x21 + carry;
            carry = rngA > 0xFF ? 1 : 0;
            rngA = rngA & 0xFF;

            rngB = rngB + 0x43 + carry;
            carry = rngB > 0xFF ? 1 : 0;
            rngB = rngB & 0xFF;

            rng_table.ptr[(i * 0x100) + j] = static_cast<uint16_t>((rngA << 8) | rngB);
        }
    }
    return rng_table.commit();
}

uint8_t RNG::run_rng(uint16_t * rng_seed)
{
    uint16_t result = rng_table.ptr[*rng_seed];
    *rng_seed = result;
    return static_cast<uint8_t>(((result >> 8) ^ (result)) & 0xFF);
}

void RNG::_generate_regular_rng_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16)
{
    uint16_t rng_seed = seed;
    for (int j = 0; j < entries; j++)
    {
        uint8_t val = run_rng(&rng_seed);
        packing_store(out, entries - 1 - j, val, packing_16);
    }
}

AlignedPtr<uint8_t> RNG::generate_regular_rng_values_for_seeds(uint16_t* seeds, int seed_count, bool packing_16)
{
    const int entries = 2048;
    int stride = entries * (packing_16 ? 2 : 1);
    auto p = make_aligned<uint8_t>(seed_count * stride);
    for (int i = 0; i < seed_count; i++)
        _generate_regular_rng_values_for_seed(p.get() + i * stride, seeds[i], entries, packing_16);
    return p;
}

AlignedPtr<uint8_t> RNG::generate_regular_rng_values_for_seeds_8(uint16_t* seeds, int seed_count)
{
    return generate_regular_rng_values_for_seeds(seeds, seed_count, false);
}

std::shared_ptr<void> RNG::_generate_regular_rng_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = packing_alloc(0x10000 * 128, packing_16);
    for (int i = 0; i < 0x10000; i++)
    {
        uint8_t* seed_out = table.ptr + i * stride;
        _generate_regular_rng_values_for_seed(seed_out, (uint16_t)i, 128, packing_16);
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_regular_rng_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = reinterpret_cast<uint16_t*>(packing_alloc(0x10000 * 128, packing_16));
    for (int i = 0; i < 0x10000; i++)
    {
        uint8_t* seed_out = reinterpret_cast<uint8_t*>(table.ptr) + i * stride;
        _generate_regular_rng_values_for_seed(seed_out, (uint16_t)i, 128, packing_16);
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

void RNG::_split(uint8_t** rng_values, bool hi)
{
    for (int i = 0; i < 0x10000; i++)
    {
        for (int j = 0; j < 128; j += 2)
        {
            if (hi)
                (*rng_values)[i * 128 + (127 - j)] = 0;
            else
                (*rng_values)[i * 128 + (127 - j) - 1] = 0;
        }
    }
}

std::shared_ptr<void> RNG::_generate_regular_rng_values_8_split(RNGTable<uint8_t>& table, bool shuffle, int bits, bool hi)
{
    auto s = table.lock();
    if (s) return s;
    s = _generate_regular_rng_values(table, shuffle, bits, false);
    _split(&table.ptr, hi);
    return s;
}

std::shared_ptr<void> RNG::generate_regular_rng_values_8()
{
    return _generate_regular_rng_values(regular_rng_values_8, false, -1, false);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_128_8_shuffled()
{
    return _generate_regular_rng_values(regular_rng_values_128_8_shuffled, true, 128, false);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_256_8_shuffled()
{
    return _generate_regular_rng_values(regular_rng_values_256_8_shuffled, true, 256, false);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_512_8_shuffled()
{
    return _generate_regular_rng_values(regular_rng_values_512_8_shuffled, true, 512, false);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_8_hi()
{
    return _generate_regular_rng_values_8_split(regular_rng_values_8_hi, false, -1, true);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_256_8_shuffled_hi()
{
    return _generate_regular_rng_values_8_split(regular_rng_values_256_8_shuffled_hi, true, 256, true);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_8_lo()
{
    return _generate_regular_rng_values_8_split(regular_rng_values_8_lo, false, -1, false);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_256_8_shuffled_lo()
{
    return _generate_regular_rng_values_8_split(regular_rng_values_256_8_shuffled_lo, true, 256, false);
}

std::shared_ptr<void> RNG::generate_regular_rng_values_16()
{
    return _generate_regular_rng_values(regular_rng_values_16, false, -1, true);
}

void RNG::_generate_expansion_values_for_seed(uint8_t* rng_values, uint16_t rng_seed, bool shuffle, int bits, bool packing_16)
{
    uint8_t temp_values[8];
    for (int j = 0; j < 16; j++)
    {
        for (int k = 0; k < 8; k++)
        {
            if (j == 0)
                temp_values[k] = 0;
            else
                temp_values[k] = static_cast<uint8_t>(temp_values[k] + run_rng(&rng_seed));
            int offset = j * 8 + k;
            if (shuffle)
                offset = shuffle_8(offset, bits);
            packing_store(rng_values, offset, temp_values[k], packing_16);
        }
    }
}

std::shared_ptr<void> RNG::_generate_expansion_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    table.ptr = packing_alloc(0x10000 * 128, packing_16);
    uint16_t rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = static_cast<uint16_t>(i);
        _generate_expansion_values_for_seed(table.ptr + rng_seed * 128 * (packing_16 ? 2 : 1), rng_seed, shuffle, bits, packing_16);
    }
    return table.commit();
}

void RNG::_generate_expansion_values_for_seed_8(AlignedPtr<uint8_t>& out, uint16_t rng_seed, bool shuffle, int bits)
{
    out = make_aligned<uint8_t>(128);
    _generate_expansion_values_for_seed(out.get(), rng_seed, shuffle, bits, false);
}

std::shared_ptr<void> RNG::generate_expansion_values_8()
{
    return _generate_expansion_values(expansion_values_8, false, -1, false);
}

std::shared_ptr<void> RNG::generate_expansion_values_128_8_shuffled()
{
    return _generate_expansion_values(expansion_values_128_8_shuffled, true, 128, false);
}

std::shared_ptr<void> RNG::generate_expansion_values_256_8_shuffled()
{
    return _generate_expansion_values(expansion_values_256_8_shuffled, true, 256, false);
}

std::shared_ptr<void> RNG::generate_expansion_values_512_8_shuffled()
{
    return _generate_expansion_values(expansion_values_512_8_shuffled, true, 512, false);
}

void RNG::_generate_alg0_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16)
{
    uint16_t rng_seed = seed;
    for (int j = 0; j < entries; j++)
    {
        uint8_t val = static_cast<uint8_t>((run_rng(&rng_seed) >> 7) & 0x01);
        packing_store(out, entries - 1 - j, val, packing_16);
    }
}

AlignedPtr<uint8_t> RNG::generate_alg0_values_for_seeds(uint16_t* seeds, int seed_count, bool packing_16)
{
    const int entries = 2048;
    int stride = entries * (packing_16 ? 2 : 1);
    auto p = make_aligned<uint8_t>(seed_count * stride);
    for (int i = 0; i < seed_count; i++)
        _generate_alg0_values_for_seed(p.get() + i * stride, seeds[i], entries, packing_16);
    return p;
}

AlignedPtr<uint8_t> RNG::generate_alg0_values_for_seeds_8(uint16_t* seeds, int seed_count)
{
    return generate_alg0_values_for_seeds(seeds, seed_count, false);
}

std::shared_ptr<void> RNG::_generate_alg0_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = packing_alloc(0x10000 * 128, packing_16);
    for (int i = 0; i < 0x10000; i++)
    {
        uint8_t* seed_out = table.ptr + i * stride;
        _generate_alg0_values_for_seed(seed_out, (uint16_t)i, 128, packing_16);
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg0_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = reinterpret_cast<uint16_t*>(packing_alloc(0x10000 * 128, packing_16));
    for (int i = 0; i < 0x10000; i++)
    {
        uint8_t* seed_out = reinterpret_cast<uint8_t*>(table.ptr) + i * stride;
        _generate_alg0_values_for_seed(seed_out, (uint16_t)i, 128, packing_16);
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::generate_alg0_values_8()
{
    return _generate_alg0_values(alg0_values_8, false, -1, false);
}

std::shared_ptr<void> RNG::generate_alg0_values_128_8_shuffled()
{
    return _generate_alg0_values(alg0_values_128_8_shuffled, true, 128, false);
}

std::shared_ptr<void> RNG::generate_alg0_values_256_8_shuffled()
{
    return _generate_alg0_values(alg0_values_256_8_shuffled, true, 256, false);
}

std::shared_ptr<void> RNG::generate_alg0_values_512_8_shuffled()
{
    return _generate_alg0_values(alg0_values_512_8_shuffled, true, 512, false);
}

std::shared_ptr<void> RNG::generate_alg0_values_16()
{
    return _generate_alg0_values(alg0_values_16, false, -1, true);
}

void RNG::_generate_alg2_values_for_seed(uint8_t* out, uint16_t seed, int entries, int bits, bool packing_16)
{
    int bytes = bits / 8;
    uint16_t rng_seed = seed;
    for (int j = 0; j < entries; j++)
    {
        uint8_t* entry = out + (entries - 1 - j) * bytes;
        for (int k = 0; k < bytes; k++)
            entry[k] = 0;
        entry[bytes - 1 + (packing_16 ? -1 : 0)] = static_cast<uint8_t>((run_rng(&rng_seed) & 0x80) >> 7);
    }
}

AlignedPtr<uint8_t> RNG::generate_alg2_values_for_seeds(uint16_t* seeds, int seed_count, int bits, bool packing_16)
{
    const int entries = 2048;
    int bytes = bits / 8;
    int stride = entries * bytes;
    auto p = make_aligned<uint8_t>(seed_count * stride);
    for (int i = 0; i < seed_count; i++)
        _generate_alg2_values_for_seed(p.get() + i * stride, seeds[i], entries, bits, packing_16);
    return p;
}

AlignedPtr<uint8_t> RNG::generate_alg2_values_for_seeds_128_8(uint16_t* seeds, int seed_count)
{
    return generate_alg2_values_for_seeds(seeds, seed_count, 128, false);
}

std::shared_ptr<void> RNG::_generate_alg2_values(RNGTable<uint8_t>& table, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int bytes = bits / 8;
    table.ptr = static_cast<uint8_t*>(aligned_malloc(0x10000 * bytes, 64));
    for (int i = 0; i < 0x10000; i++)
        _generate_alg2_values_for_seed(table.ptr + i * bytes, (uint16_t)i, 1, bits, packing_16);
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg2_values(RNGTable<uint32_t>& table, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int bytes = bits / 8;
    table.ptr = static_cast<uint32_t*>(aligned_malloc(0x10000 * bytes, 64));
    for (int i = 0; i < 0x10000; i++)
        _generate_alg2_values_for_seed(reinterpret_cast<uint8_t*>(table.ptr) + i * bytes, (uint16_t)i, 1, bits, packing_16);
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg2_values(RNGTable<uint64_t>& table, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int bytes = bits / 8;
    table.ptr = static_cast<uint64_t*>(aligned_malloc(0x10000 * bytes, 64));
    for (int i = 0; i < 0x10000; i++)
        _generate_alg2_values_for_seed(reinterpret_cast<uint8_t*>(table.ptr) + i * bytes, (uint16_t)i, 1, bits, packing_16);
    return table.commit();
}

std::shared_ptr<void> RNG::generate_alg2_values_8_8()   { return _generate_alg2_values(alg2_values_8_8,   8,  false); }
std::shared_ptr<void> RNG::generate_alg2_values_32_8()  { return _generate_alg2_values(alg2_values_32_8,  32, false); }
std::shared_ptr<void> RNG::generate_alg2_values_32_16() { return _generate_alg2_values(alg2_values_32_16, 32, true);  }
std::shared_ptr<void> RNG::generate_alg2_values_64_8()  { return _generate_alg2_values(alg2_values_64_8,  64, false); }
std::shared_ptr<void> RNG::generate_alg2_values_64_16() { return _generate_alg2_values(alg2_values_64_16, 64, true);  }
std::shared_ptr<void> RNG::generate_alg2_values_128_8()  { return _generate_alg2_values(alg2_values_128_8,  128, false); }
std::shared_ptr<void> RNG::generate_alg2_values_128_16() { return _generate_alg2_values(alg2_values_128_16, 128, true);  }
std::shared_ptr<void> RNG::generate_alg2_values_256_8()  { return _generate_alg2_values(alg2_values_256_8,  256, false); }
std::shared_ptr<void> RNG::generate_alg2_values_256_16() { return _generate_alg2_values(alg2_values_256_16, 256, true);  }
std::shared_ptr<void> RNG::generate_alg2_values_512_8()  { return _generate_alg2_values(alg2_values_512_8,  512, false); }

std::shared_ptr<void> RNG::_generate_alg4_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = packing_alloc(0x10000 * 128, packing_16);
    for (int i = 0; i < 0x10000; i++)
    {
        uint16_t rng_seed = (uint16_t)i;
        uint8_t* seed_out = table.ptr + i * stride;
        for (int j = 0; j < 128; j++)
        {
            uint8_t val = static_cast<uint8_t>((run_rng(&rng_seed) ^ 0xFF) + 1);
            packing_store(seed_out, 127 - j, val, packing_16);
        }
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg4_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = reinterpret_cast<uint16_t*>(packing_alloc(0x10000 * 128, packing_16));
    for (int i = 0; i < 0x10000; i++)
    {
        uint16_t rng_seed = (uint16_t)i;
        uint8_t* seed_out = reinterpret_cast<uint8_t*>(table.ptr) + i * stride;
        for (int j = 0; j < 128; j++)
        {
            uint8_t val = static_cast<uint8_t>((run_rng(&rng_seed) ^ 0xFF) + 1);
            packing_store(seed_out, 127 - j, val, packing_16);
        }
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg4_values_8_split(RNGTable<uint8_t>& table, bool shuffle, int bits, bool hi)
{
    auto s = table.lock();
    if (s) return s;
    s = _generate_alg4_values(table, shuffle, bits, false);
    _split(&table.ptr, hi);
    return s;
}

std::shared_ptr<void> RNG::generate_alg4_values_8()     { return _generate_alg4_values(alg4_values_8, false, -1, false); }
std::shared_ptr<void> RNG::generate_alg4_values_128_8_shuffled() { return _generate_alg4_values(alg4_values_128_8_shuffled, true, 128, false); }
std::shared_ptr<void> RNG::generate_alg4_values_256_8_shuffled() { return _generate_alg4_values(alg4_values_256_8_shuffled, true, 256, false); }
std::shared_ptr<void> RNG::generate_alg4_values_512_8_shuffled() { return _generate_alg4_values(alg4_values_512_8_shuffled, true, 512, false); }
std::shared_ptr<void> RNG::generate_alg4_values_16()    { return _generate_alg4_values(alg4_values_16, false, -1, true); }

std::shared_ptr<void> RNG::generate_alg4_values_8_hi()
{
    return _generate_alg4_values_8_split(alg4_values_8_hi, false, -1, true);
}

std::shared_ptr<void> RNG::generate_alg4_values_256_8_shuffled_hi()
{
    return _generate_alg4_values_8_split(alg4_values_256_8_shuffled_hi, true, 256, true);
}

std::shared_ptr<void> RNG::generate_alg4_values_8_lo()
{
    return _generate_alg4_values_8_split(alg4_values_8_lo, false, -1, false);
}

std::shared_ptr<void> RNG::generate_alg4_values_256_8_shuffled_lo()
{
    return _generate_alg4_values_8_split(alg4_values_256_8_shuffled_lo, true, 256, false);
}

void RNG::_generate_alg5_values_for_seed(uint8_t* out, uint16_t seed, int entries, int bits, bool packing_16)
{
    int bytes = bits / 8;
    uint16_t rng_seed = seed;
    for (int j = 0; j < entries; j++)
    {
        uint8_t* entry = out + (entries - 1 - j) * bytes;
        for (int k = 0; k < bytes; k++)
            entry[k] = 0;
        entry[bytes - 1 + (packing_16 ? -1 : 0)] = static_cast<uint8_t>(run_rng(&rng_seed) & 0x80);
    }
}

AlignedPtr<uint8_t> RNG::generate_alg5_values_for_seeds(uint16_t* seeds, int seed_count, int bits, bool packing_16)
{
    const int entries = 2048;
    int bytes = bits / 8;
    int stride = entries * bytes;
    auto p = make_aligned<uint8_t>(seed_count * stride);
    for (int i = 0; i < seed_count; i++)
        _generate_alg5_values_for_seed(p.get() + i * stride, seeds[i], entries, bits, packing_16);
    return p;
}

AlignedPtr<uint8_t> RNG::generate_alg5_values_for_seeds_128_8(uint16_t* seeds, int seed_count)
{
    return generate_alg5_values_for_seeds(seeds, seed_count, 128, false);
}

std::shared_ptr<void> RNG::_generate_alg5_values(RNGTable<uint8_t>& table, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int bytes = bits / 8;
    table.ptr = static_cast<uint8_t*>(aligned_malloc(0x10000 * bytes, 64));
    for (int i = 0; i < 0x10000; i++)
        _generate_alg5_values_for_seed(table.ptr + i * bytes, (uint16_t)i, 1, bits, packing_16);
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg5_values(RNGTable<uint32_t>& table, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int bytes = bits / 8;
    table.ptr = static_cast<uint32_t*>(aligned_malloc(0x10000 * bytes, 64));
    for (int i = 0; i < 0x10000; i++)
        _generate_alg5_values_for_seed(reinterpret_cast<uint8_t*>(table.ptr) + i * bytes, (uint16_t)i, 1, bits, packing_16);
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg5_values(RNGTable<uint64_t>& table, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int bytes = bits / 8;
    table.ptr = static_cast<uint64_t*>(aligned_malloc(0x10000 * bytes, 64));
    for (int i = 0; i < 0x10000; i++)
        _generate_alg5_values_for_seed(reinterpret_cast<uint8_t*>(table.ptr) + i * bytes, (uint16_t)i, 1, bits, packing_16);
    return table.commit();
}

std::shared_ptr<void> RNG::generate_alg5_values_8_8()   { return _generate_alg5_values(alg5_values_8_8,   8,   false); }
std::shared_ptr<void> RNG::generate_alg5_values_32_8()  { return _generate_alg5_values(alg5_values_32_8,  32,  false); }
std::shared_ptr<void> RNG::generate_alg5_values_32_16() { return _generate_alg5_values(alg5_values_32_16, 32,  true);  }
std::shared_ptr<void> RNG::generate_alg5_values_64_8()  { return _generate_alg5_values(alg5_values_64_8,  64,  false); }
std::shared_ptr<void> RNG::generate_alg5_values_64_16() { return _generate_alg5_values(alg5_values_64_16, 64,  true);  }
std::shared_ptr<void> RNG::generate_alg5_values_128_8()  { return _generate_alg5_values(alg5_values_128_8,  128, false); }
std::shared_ptr<void> RNG::generate_alg5_values_128_16() { return _generate_alg5_values(alg5_values_128_16, 128, true);  }
std::shared_ptr<void> RNG::generate_alg5_values_256_8()  { return _generate_alg5_values(alg5_values_256_8,  256, false); }
std::shared_ptr<void> RNG::generate_alg5_values_256_16() { return _generate_alg5_values(alg5_values_256_16, 256, true);  }
std::shared_ptr<void> RNG::generate_alg5_values_512_8()  { return _generate_alg5_values(alg5_values_512_8,  512, false); }

void RNG::_generate_alg6_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16)
{
    uint16_t rng_seed = seed;
    for (int j = 0; j < entries; j++)
    {
        uint8_t val = static_cast<uint8_t>(run_rng(&rng_seed) & 0x80);
        packing_store(out, j, val, packing_16);
    }
}

AlignedPtr<uint8_t> RNG::generate_alg6_values_for_seeds(uint16_t* seeds, int seed_count, bool packing_16)
{
    const int entries = 2048;
    int stride = entries * (packing_16 ? 2 : 1);
    auto p = make_aligned<uint8_t>(seed_count * stride);
    for (int i = 0; i < seed_count; i++)
        _generate_alg6_values_for_seed(p.get() + i * stride, seeds[i], entries, packing_16);
    return p;
}

AlignedPtr<uint8_t> RNG::generate_alg6_values_for_seeds_8(uint16_t* seeds, int seed_count)
{
    return generate_alg6_values_for_seeds(seeds, seed_count, false);
}

std::shared_ptr<void> RNG::_generate_alg6_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = packing_alloc(0x10000 * 128, packing_16);
    for (int i = 0; i < 0x10000; i++)
    {
        uint8_t* seed_out = table.ptr + i * stride;
        _generate_alg6_values_for_seed(seed_out, (uint16_t)i, 128, packing_16);
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::_generate_alg6_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    int stride = 128 * (packing_16 ? 2 : 1);
    table.ptr = reinterpret_cast<uint16_t*>(packing_alloc(0x10000 * 128, packing_16));
    for (int i = 0; i < 0x10000; i++)
    {
        uint8_t* seed_out = reinterpret_cast<uint8_t*>(table.ptr) + i * stride;
        _generate_alg6_values_for_seed(seed_out, (uint16_t)i, 128, packing_16);
        if (shuffle)
        {
            uint8_t temp[128];
            for (int k = 0; k < 128; k++)
                temp[k] = packing_load(seed_out, k, packing_16);
            for (int k = 0; k < 128; k++)
                packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::generate_alg6_values_128_8_shuffled() { return _generate_alg6_values(alg6_values_128_8_shuffled, true, 128, false); }
std::shared_ptr<void> RNG::generate_alg6_values_256_8_shuffled() { return _generate_alg6_values(alg6_values_256_8_shuffled, true, 256, false); }
std::shared_ptr<void> RNG::generate_alg6_values_512_8_shuffled() { return _generate_alg6_values(alg6_values_512_8_shuffled, true, 512, false); }
std::shared_ptr<void> RNG::generate_alg6_values_8()  { return _generate_alg6_values(alg6_values_8,  false, -1, false); }
std::shared_ptr<void> RNG::generate_alg6_values_16() { return _generate_alg6_values(alg6_values_16, false, -1, true);  }

std::shared_ptr<void> RNG::_generate_alg06_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16)
{
    auto s = table.lock();
    if (s) return s;
    table.ptr = packing_alloc(0x10000 * 128, packing_16);
    uint16_t rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        for (int j = 0; j < 128; j++)
            packing_store(table.ptr, i * 128 + j, 0, packing_16);
        rng_seed = static_cast<uint16_t>(i);
        for (int j = 0; j < 128; j++)
        {
            uint8_t rng_res = run_rng(&rng_seed);
            uint8_t alg0_val = static_cast<uint8_t>((rng_res >> 7) & 0x01);
            uint8_t alg6_val = static_cast<uint8_t>(rng_res & 0x80);
            int alg0_offset = (127 - j);
            int alg6_offset = j;
            if (shuffle)
            {
                alg0_offset = shuffle_8(alg0_offset, bits);
                alg6_offset = shuffle_8(alg6_offset, bits);
            }
            uint8_t old_val = packing_load(table.ptr, i * 128 + alg0_offset, packing_16);
            packing_store(table.ptr, i * 128 + alg0_offset, static_cast<uint8_t>(old_val | alg0_val), packing_16);
            old_val = packing_load(table.ptr, i * 128 + alg6_offset, packing_16);
            packing_store(table.ptr, i * 128 + alg6_offset, static_cast<uint8_t>(old_val | alg6_val), packing_16);
        }
    }
    return table.commit();
}

std::shared_ptr<void> RNG::generate_alg06_values_8()              { return _generate_alg06_values(alg06_values_8,              false, -1,  false); }
std::shared_ptr<void> RNG::generate_alg06_values_128_8_shuffled() { return _generate_alg06_values(alg06_values_128_8_shuffled, true,  128, false); }

std::shared_ptr<void> RNG::generate_seed_forward_1()
{
    auto s = seed_forward_1.lock();
    if (s) return s;
    seed_forward_1.ptr = static_cast<uint16_t*>(aligned_malloc(256 * 256 * sizeof(uint16_t), 64));
    for (int i = 0; i < 0x10000; i++)
        seed_forward_1.ptr[i] = rng_table.ptr[i];
    return seed_forward_1.commit();
}

std::shared_ptr<void> RNG::generate_seed_forward_128()
{
    auto s = seed_forward_128.lock();
    if (s) return s;
    seed_forward_128.ptr = static_cast<uint16_t*>(aligned_malloc(256 * 256 * sizeof(uint16_t), 64));
    uint16_t rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = static_cast<uint16_t>(i);
        for (int j = 0; j < 128; j++)
            rng_seed = rng_table.ptr[rng_seed];
        seed_forward_128.ptr[i] = rng_seed;
    }
    return seed_forward_128.commit();
}

std::shared_ptr<void> RNG::generate_seed_forward()
{
    auto s = seed_forward.lock();
    if (s) return s;
    seed_forward.ptr = static_cast<uint16_t*>(aligned_malloc(256 * 256 * 2048 * sizeof(uint16_t), 64));
    uint16_t rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        uint16_t* cur_seed_ptr = &(seed_forward.ptr[i * 2048]);
        rng_seed = static_cast<uint16_t>(i);
        for (int j = 0; j < 2048; j++)
        {
            cur_seed_ptr[j] = rng_seed;
            rng_seed = rng_table.ptr[rng_seed];
        }
    }
    return seed_forward.commit();
}

std::shared_ptr<void> RNG::generate_rng_seq_tables()
{
    auto s = rng_seq_table.lock();
    if (s)
    {
        auto p = rng_pos_table.lock();
        auto bundle = std::make_shared<std::pair<std::shared_ptr<void>, std::shared_ptr<void>>>(p, s);
        return std::shared_ptr<void>(bundle, bundle.get());
    }

    const uint32_t MAX_SEQ_SIZE = 65536 + 3 * 127;
    rng_seq_table.ptr = static_cast<uint16_t*>(aligned_malloc(MAX_SEQ_SIZE * sizeof(uint16_t), 64));
    rng_pos_table.ptr = static_cast<uint32_t*>(aligned_malloc(0x10000 * sizeof(uint32_t), 64));

    bool visited[0x10000] = {};
    uint32_t write_pos = 0;

    // Z1 tail
    {
        uint16_t cur = 0x2143;
        while (cur != 0xAA6D)
        {
            rng_pos_table.ptr[cur] = write_pos;
            rng_seq_table.ptr[write_pos++] = rng_table.ptr[cur];
            visited[cur] = true;
            cur = rng_table.ptr[cur];
        }
    }
    // Z0 cycle
    {
        uint32_t z0_base = write_pos;
        uint16_t cur = 0xAA6D;
        do {
            rng_pos_table.ptr[cur] = write_pos;
            rng_seq_table.ptr[write_pos++] = rng_table.ptr[cur];
            visited[cur] = true;
            cur = rng_table.ptr[cur];
        } while (cur != 0xAA6D);
        uint32_t z0_len = write_pos - z0_base;
        for (uint32_t i = 0; i < 127; i++)
            rng_seq_table.ptr[write_pos++] = rng_seq_table.ptr[z0_base + (i % z0_len)];
    }
    // X cycle
    {
        uint32_t x_base = write_pos;
        uint16_t cur = 0x000F;
        do {
            rng_pos_table.ptr[cur] = write_pos;
            rng_seq_table.ptr[write_pos++] = rng_table.ptr[cur];
            visited[cur] = true;
            cur = rng_table.ptr[cur];
        } while (cur != 0x000F);
        uint32_t x_len = write_pos - x_base;
        for (uint32_t i = 0; i < 127; i++)
            rng_seq_table.ptr[write_pos++] = rng_seq_table.ptr[x_base + (i % x_len)];
    }
    // Y cycle
    {
        uint32_t y_base = write_pos;
        uint16_t cur = 0x0004;
        do {
            rng_pos_table.ptr[cur] = write_pos;
            rng_seq_table.ptr[write_pos++] = rng_table.ptr[cur];
            visited[cur] = true;
            cur = rng_table.ptr[cur];
        } while (cur != 0x0004);
        uint32_t y_len = write_pos - y_base;
        for (uint32_t i = 0; i < 127; i++)
            rng_seq_table.ptr[write_pos++] = rng_seq_table.ptr[y_base + (i % y_len)];
    }

    rng_seq_table_size = write_pos;

    uint32_t missed = 0;
    for (uint32_t s2 = 0; s2 < 0x10000; s2++)
    {
        if (!visited[s2])
        {
            fprintf(stderr, "generate_rng_seq_tables: unvisited seed 0x%04X\n", s2);
            missed++;
        }
    }
    if (missed == 0)
        fprintf(stderr, "generate_rng_seq_tables: all 65536 seeds OK, table size=%u\n", write_pos);
    else
        fprintf(stderr, "generate_rng_seq_tables: ERROR: %u seeds not visited!\n", missed);

    auto pos_s = rng_pos_table.commit();
    auto seq_s = rng_seq_table.commit();
    auto bundle = std::make_shared<std::pair<std::shared_ptr<void>, std::shared_ptr<void>>>(pos_s, seq_s);
    return std::shared_ptr<void>(bundle, bundle.get());
}

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

RNGTable<uint16_t> RNG::rng_table;
RNGTable<uint16_t> RNG::rng_seq_table;
RNGTable<uint32_t> RNG::rng_pos_table;
uint32_t           RNG::rng_seq_table_size = 0;
RNGTable<uint16_t> RNG::seed_forward;
RNGTable<uint16_t> RNG::seed_forward_1;
RNGTable<uint16_t> RNG::seed_forward_128;

RNGTable<uint8_t>  RNG::regular_rng_values_8;
RNGTable<uint8_t>  RNG::regular_rng_values_128_8_shuffled;
RNGTable<uint8_t>  RNG::regular_rng_values_256_8_shuffled;
RNGTable<uint8_t>  RNG::regular_rng_values_512_8_shuffled;
RNGTable<uint8_t>  RNG::regular_rng_values_8_hi;
RNGTable<uint8_t>  RNG::regular_rng_values_256_8_shuffled_hi;
RNGTable<uint8_t>  RNG::regular_rng_values_8_lo;
RNGTable<uint8_t>  RNG::regular_rng_values_256_8_shuffled_lo;
RNGTable<uint16_t> RNG::regular_rng_values_16;

RNGTable<uint8_t> RNG::expansion_values_8;
RNGTable<uint8_t> RNG::expansion_values_128_8_shuffled;
RNGTable<uint8_t> RNG::expansion_values_256_8_shuffled;
RNGTable<uint8_t> RNG::expansion_values_512_8_shuffled;

RNGTable<uint8_t>  RNG::alg0_values_8;
RNGTable<uint8_t>  RNG::alg0_values_128_8_shuffled;
RNGTable<uint8_t>  RNG::alg0_values_256_8_shuffled;
RNGTable<uint8_t>  RNG::alg0_values_512_8_shuffled;
RNGTable<uint16_t> RNG::alg0_values_16;

RNGTable<uint8_t>  RNG::alg2_values_8_8;
RNGTable<uint32_t> RNG::alg2_values_32_8;
RNGTable<uint32_t> RNG::alg2_values_32_16;
RNGTable<uint64_t> RNG::alg2_values_64_8;
RNGTable<uint64_t> RNG::alg2_values_64_16;
RNGTable<uint8_t>  RNG::alg2_values_128_8;
RNGTable<uint8_t>  RNG::alg2_values_128_16;
RNGTable<uint8_t>  RNG::alg2_values_256_8;
RNGTable<uint8_t>  RNG::alg2_values_256_16;
RNGTable<uint8_t>  RNG::alg2_values_512_8;

RNGTable<uint8_t>  RNG::alg4_values_8;
RNGTable<uint8_t>  RNG::alg4_values_8_hi;
RNGTable<uint8_t>  RNG::alg4_values_128_8_shuffled;
RNGTable<uint8_t>  RNG::alg4_values_256_8_shuffled;
RNGTable<uint8_t>  RNG::alg4_values_512_8_shuffled;
RNGTable<uint8_t>  RNG::alg4_values_256_8_shuffled_hi;
RNGTable<uint8_t>  RNG::alg4_values_8_lo;
RNGTable<uint8_t>  RNG::alg4_values_256_8_shuffled_lo;
RNGTable<uint16_t> RNG::alg4_values_16;

RNGTable<uint8_t>  RNG::alg5_values_8_8;
RNGTable<uint32_t> RNG::alg5_values_32_8;
RNGTable<uint32_t> RNG::alg5_values_32_16;
RNGTable<uint64_t> RNG::alg5_values_64_8;
RNGTable<uint64_t> RNG::alg5_values_64_16;
RNGTable<uint8_t>  RNG::alg5_values_128_8;
RNGTable<uint8_t>  RNG::alg5_values_128_16;
RNGTable<uint8_t>  RNG::alg5_values_256_8;
RNGTable<uint8_t>  RNG::alg5_values_256_16;
RNGTable<uint8_t>  RNG::alg5_values_512_8;

RNGTable<uint8_t>  RNG::alg6_values_8;
RNGTable<uint8_t>  RNG::alg6_values_128_8_shuffled;
RNGTable<uint8_t>  RNG::alg6_values_256_8_shuffled;
RNGTable<uint8_t>  RNG::alg6_values_512_8_shuffled;
RNGTable<uint16_t> RNG::alg6_values_16;

RNGTable<uint8_t> RNG::alg06_values_8;
RNGTable<uint8_t> RNG::alg06_values_128_8_shuffled;
