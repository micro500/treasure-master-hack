#include <stdio.h>
#include <stdint.h>
#include "data_sizes.h"
#include "alignment2.h"
#include "rng_obj.h"

RNG::RNG()
{
	if (rng_table == nullptr)
	{
		generate_rng_table();
	}
}

void RNG::generate_rng_table()
{
	rng_table = new uint16[256 * 256];
	for (int i = 0; i <= 0xFF; i++)
	{
		for (int j = 0; j <= 0xFF; j++)
		{
			unsigned int rngA = i;
			unsigned int rngB = j;

			uint8 carry = 0;

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

			rng_table[(i * 0x100) + j] = (rngA << 8) | rngB;
		}
	}
}

uint8 RNG::run_rng(uint16 * rng_seed)
{
	uint16 result = rng_table[*rng_seed];
	*rng_seed = result;

	return ((result >> 8) ^ (result)) & 0xFF;
}

void RNG::_generate_regular_rng_values_for_seed(uint8* out, uint16 seed, int entries, bool packing_16)
{
	uint16 rng_seed = seed;
	for (int j = 0; j < entries; j++)
	{
		uint8 val = run_rng(&rng_seed);
		packing_store(out, entries - 1 - j, val, packing_16);
	}
}

void RNG::generate_regular_rng_values_for_seeds(uint8** out, uint16* seeds, int seed_count, bool packing_16)
{
	const int entries = 2048;
	int stride = entries * (packing_16 ? 2 : 1);
	aligned_free(*out);
	*out = packing_alloc(seed_count * entries, packing_16, stride);
	for (int i = 0; i < seed_count; i++)
	{
		_generate_regular_rng_values_for_seed(*out + i * stride, seeds[i], entries, packing_16);
	}
}

void RNG::generate_regular_rng_values_for_seeds_8(uint8_t** out, uint16* seeds, int seed_count)
{
	generate_regular_rng_values_for_seeds(out, seeds, seed_count, false);
}

void RNG::_generate_regular_rng_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int stride = 128 * (packing_16 ? 2 : 1);
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		for (int i = 0; i < 0x10000; i++)
		{
			uint8* seed_out = *rng_values + i * stride;
			_generate_regular_rng_values_for_seed(seed_out, (uint16)i, 128, packing_16);

			if (shuffle)
			{
				uint8 temp[128];
				for (int k = 0; k < 128; k++)
					temp[k] = packing_load(seed_out, k, packing_16);
				for (int k = 0; k < 128; k++)
					packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
			}
		}
	}
}


void RNG::_split(uint8** rng_values, bool hi)
{
	for (int i = 0; i < 0x10000; i++)
	{
		for (int j = 0; j < 128; j += 2)
		{
			if (hi)
			{
				(*rng_values)[i * 128 + (127 - j)] = 0;
			}
			else
			{
				(*rng_values)[i * 128 + (127 - j) - 1] = 0;
			}
		}
	}
}

void RNG::_generate_regular_rng_values_8_split(uint8** rng_values, bool shuffle, int bits, bool hi)
{
	if (*rng_values == nullptr)
	{
		_generate_regular_rng_values(rng_values, shuffle, bits, false);
		_split(rng_values, hi);
	}
}

void RNG::generate_regular_rng_values_8()
{
	_generate_regular_rng_values(&regular_rng_values_8, false, -1, false);
}

void RNG::generate_regular_rng_values_128_8_shuffled()
{
	_generate_regular_rng_values(&regular_rng_values_128_8_shuffled, true, 128, false);
}

void RNG::generate_regular_rng_values_256_8_shuffled()
{
	_generate_regular_rng_values(&regular_rng_values_256_8_shuffled, true, 256, false);
}

void RNG::generate_regular_rng_values_512_8_shuffled()
{
	_generate_regular_rng_values(&regular_rng_values_512_8_shuffled, true, 512, false);
}

void RNG::generate_regular_rng_values_8_hi()
{
	_generate_regular_rng_values_8_split(&regular_rng_values_8_hi, false, -1, true);
}

void RNG::generate_regular_rng_values_256_8_shuffled_hi()
{
	_generate_regular_rng_values_8_split(&regular_rng_values_256_8_shuffled_hi, true, 256, true);
}

void RNG::generate_regular_rng_values_8_lo()
{
	_generate_regular_rng_values_8_split(&regular_rng_values_8_lo, false, -1, false);
}

void RNG::generate_regular_rng_values_256_8_shuffled_lo()
{
	_generate_regular_rng_values_8_split(&regular_rng_values_256_8_shuffled_lo, true, 256, false);
}

void RNG::generate_regular_rng_values_16()
{
	_generate_regular_rng_values((uint8**)&regular_rng_values_16, false, -1, true);
}

void RNG::_generate_expansion_values_for_seed(uint8* rng_values, uint16_t rng_seed, bool shuffle, int bits, bool packing_16)
{
	uint8_t temp_values[8];
	for (int j = 0; j < 16; j++)
	{
		for (int k = 0; k < 8; k++)
		{
			if (j == 0)
			{
				temp_values[k] = 0;
			}
			else
			{
				temp_values[k] = temp_values[k] + run_rng(&rng_seed);
			}

			int offset = j * 8 + k;
			if (shuffle)
			{
				offset = shuffle_8(offset, bits);
			}
			packing_store(rng_values, offset, temp_values[k], packing_16);
		}
	}
}


void RNG::_generate_expansion_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		uint16 rng_seed;

		for (int i = 0; i < 0x10000; i++)
		{
			rng_seed = i;
			_generate_expansion_values_for_seed(*rng_values + rng_seed * 128 * (packing_16 ? 2 : 1), rng_seed, shuffle, bits, packing_16);
		}
	}
}

void RNG::generate_expansion_values_8()
{
	_generate_expansion_values(&expansion_values_8, false, -1, false);
}

void RNG::generate_expansion_values_128_8_shuffled()
{
	_generate_expansion_values(&expansion_values_128_8_shuffled, true, 128, false);
}

void RNG::generate_expansion_values_256_8_shuffled()
{
	_generate_expansion_values(&expansion_values_256_8_shuffled, true, 256, false);
}

void RNG::_generate_expansion_values_for_seed_8(uint8** rng_values, uint16_t rng_seed, bool shuffle, int bits)
{
	aligned_free(*rng_values);
	*rng_values = packing_alloc(128, false);

	_generate_expansion_values_for_seed(*rng_values, rng_seed, shuffle, bits, false);
}


void RNG::_generate_alg0_values_for_seed(uint8* out, uint16 seed, int entries, bool packing_16)
{
	uint16 rng_seed = seed;
	for (int j = 0; j < entries; j++)
	{
		uint8 val = (run_rng(&rng_seed) >> 7) & 0x01;
		packing_store(out, entries - 1 - j, val, packing_16);
	}
}

void RNG::generate_alg0_values_for_seeds(uint8** out, uint16* seeds, int seed_count, bool packing_16)
{
	const int entries = 2048;
	int stride = entries * (packing_16 ? 2 : 1);
	aligned_free(*out);
	*out = packing_alloc(seed_count * entries, packing_16, stride);
	for (int i = 0; i < seed_count; i++)
	{
		_generate_alg0_values_for_seed(*out + i * stride, seeds[i], entries, packing_16);
	}
}

void RNG::generate_alg0_values_for_seeds_8(uint8_t** out, uint16* seeds, int seed_count)
{
	generate_alg0_values_for_seeds(out, seeds, seed_count, false);
}

void RNG::_generate_alg0_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int stride = 128 * (packing_16 ? 2 : 1);
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		for (int i = 0; i < 0x10000; i++)
		{
			uint8* seed_out = *rng_values + i * stride;
			_generate_alg0_values_for_seed(seed_out, (uint16)i, 128, packing_16);

			if (shuffle)
			{
				uint8 temp[128];
				for (int k = 0; k < 128; k++)
					temp[k] = packing_load(seed_out, k, packing_16);
				for (int k = 0; k < 128; k++)
					packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
			}
		}
	}
}

void RNG::generate_alg0_values_8()
{
	_generate_alg0_values(&alg0_values_8, false, -1, false);
}

void RNG::generate_alg0_values_128_8_shuffled()
{
	_generate_alg0_values(&alg0_values_128_8_shuffled, true, 128, false);
}

void RNG::generate_alg0_values_256_8_shuffled()
{
	_generate_alg0_values(&alg0_values_256_8_shuffled, true, 256, false);
}

void RNG::generate_alg0_values_512_8_shuffled()
{
	_generate_alg0_values(&alg0_values_512_8_shuffled, true, 512, false);
}

void RNG::generate_alg0_values_16()
{
	_generate_alg0_values((uint8**)&alg0_values_16, false, -1, true);
}

void RNG::_generate_alg2_values_for_seed(uint8* out, uint16 seed, int entries, int bits, bool packing_16)
{
	int bytes = bits / 8;
	uint16 rng_seed = seed;
	for (int j = 0; j < entries; j++)
	{
		uint8* entry = out + (entries - 1 - j) * bytes;
		for (int k = 0; k < bytes; k++)
			entry[k] = 0;
		entry[bytes - 1 + (packing_16 ? -1 : 0)] = (run_rng(&rng_seed) & 0x80) >> 7;
	}
}

void RNG::generate_alg2_values_for_seeds(uint8** out, uint16* seeds, int seed_count, int bits, bool packing_16)
{
	const int entries = 2048;
	int bytes = bits / 8;
	int stride = entries * bytes;
	aligned_free(*out);
	*out = (uint8*)aligned_malloc(seed_count * stride, stride);
	for (int i = 0; i < seed_count; i++)
	{
		_generate_alg2_values_for_seed(*out + i * stride, seeds[i], entries, bits, packing_16);
	}
}

void RNG::generate_alg2_values_for_seeds_128_8(uint8_t** out, uint16* seeds, int seed_count)
{
	generate_alg2_values_for_seeds(out, seeds, seed_count, 128, false);
}

void RNG::_generate_alg2_values(uint8** rng_values, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int bytes = bits / 8;
		*rng_values = (uint8*)aligned_malloc(0x10000 * bytes, 64);
		for (int i = 0; i < 0x10000; i++)
		{
			_generate_alg2_values_for_seed(*rng_values + i * bytes, (uint16)i, 1, bits, packing_16);
		}
	}
}

void RNG::generate_alg2_values_8_8()
{
	_generate_alg2_values(&alg2_values_8_8, 8, false);
}

void RNG::generate_alg2_values_32_8()
{
	_generate_alg2_values((uint8**)&alg2_values_32_8, 32, false);
}

void RNG::generate_alg2_values_32_16()
{
	_generate_alg2_values((uint8**)&alg2_values_32_16, 32, true);
}

void RNG::generate_alg2_values_64_8()
{
	_generate_alg2_values((uint8**)&alg2_values_64_8, 64, false);
}

void RNG::generate_alg2_values_64_16()
{
	_generate_alg2_values((uint8**)&alg2_values_64_16, 64, true);
}

void RNG::generate_alg2_values_128_8()
{
	_generate_alg2_values(&alg2_values_128_8, 128, false);
}

void RNG::generate_alg2_values_128_16()
{
	_generate_alg2_values(&alg2_values_128_16, 128, true);
}

void RNG::generate_alg2_values_256_8()
{
	_generate_alg2_values(&alg2_values_256_8, 256, false);
}

void RNG::generate_alg2_values_256_16()
{
	_generate_alg2_values(&alg2_values_256_16, 256, true);
}

void RNG::generate_alg2_values_512_8()
{
	_generate_alg2_values(&alg2_values_512_8, 512, false);
}

void RNG::_generate_alg4_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int stride = 128 * (packing_16 ? 2 : 1);
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		for (int i = 0; i < 0x10000; i++)
		{
			uint16 rng_seed = (uint16)i;
			uint8* seed_out = *rng_values + i * stride;
			for (int j = 0; j < 128; j++)
			{
				uint8 val = (run_rng(&rng_seed) ^ 0xFF) + 1;
				packing_store(seed_out, 127 - j, val, packing_16);
			}

			if (shuffle)
			{
				uint8 temp[128];
				for (int k = 0; k < 128; k++)
					temp[k] = packing_load(seed_out, k, packing_16);
				for (int k = 0; k < 128; k++)
					packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
			}
		}
	}
}

void RNG::_generate_alg4_values_8_split(uint8** rng_values, bool shuffle, int bits, bool hi)
{
	if (*rng_values == nullptr)
	{
		_generate_alg4_values(rng_values, shuffle, bits, false);
		_split(rng_values, hi);
	}
}

void RNG::generate_alg4_values_8()
{
	_generate_alg4_values(&alg4_values_8, false, -1, false);
}

void RNG::generate_alg4_values_8_hi()
{
	_generate_alg4_values_8_split(&alg4_values_8_hi, false, -1, true);
}

void RNG::generate_alg4_values_128_8_shuffled()
{
	_generate_alg4_values(&alg4_values_128_8_shuffled, true, 128, false);
}

void RNG::generate_alg4_values_256_8_shuffled()
{
	_generate_alg4_values(&alg4_values_256_8_shuffled, true, 256, false);
}

void RNG::generate_alg4_values_512_8_shuffled()
{
	_generate_alg4_values(&alg4_values_512_8_shuffled, true, 512, false);
}

void RNG::generate_alg4_values_256_8_shuffled_hi()
{
	_generate_alg4_values_8_split(&alg4_values_256_8_shuffled_hi, true, 256, true);
}

void RNG::generate_alg4_values_8_lo()
{
	_generate_alg4_values_8_split(&alg4_values_8_lo, false, -1, false);
}

void RNG::generate_alg4_values_256_8_shuffled_lo()
{
	_generate_alg4_values_8_split(&alg4_values_256_8_shuffled_lo, true, 256, false);
}

void RNG::generate_alg4_values_16()
{
	_generate_alg4_values((uint8**)&alg4_values_16, false, -1, true);
}

void RNG::_generate_alg5_values_for_seed(uint8* out, uint16 seed, int entries, int bits, bool packing_16)
{
	int bytes = bits / 8;
	uint16 rng_seed = seed;
	for (int j = 0; j < entries; j++)
	{
		uint8* entry = out + (entries - 1 - j) * bytes;
		for (int k = 0; k < bytes; k++)
			entry[k] = 0;
		entry[bytes - 1 + (packing_16 ? -1 : 0)] = run_rng(&rng_seed) & 0x80;
	}
}

void RNG::generate_alg5_values_for_seeds(uint8** out, uint16* seeds, int seed_count, int bits, bool packing_16)
{
	const int entries = 2048;
	int bytes = bits / 8;
	int stride = entries * bytes;
	aligned_free(*out);
	*out = (uint8*)aligned_malloc(seed_count * stride, stride);
	for (int i = 0; i < seed_count; i++)
	{
		_generate_alg5_values_for_seed(*out + i * stride, seeds[i], entries, bits, packing_16);
	}
}

void RNG::generate_alg5_values_for_seeds_128_8(uint8_t** out, uint16* seeds, int seed_count)
{
	generate_alg5_values_for_seeds(out, seeds, seed_count, 128, false);
}

void RNG::_generate_alg5_values(uint8** rng_values, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int bytes = bits / 8;
		*rng_values = (uint8*)aligned_malloc(0x10000 * bytes, 64);
		for (int i = 0; i < 0x10000; i++)
		{
			_generate_alg5_values_for_seed(*rng_values + i * bytes, (uint16)i, 1, bits, packing_16);
		}
	}
}

void RNG::generate_alg5_values_8_8()
{
	_generate_alg5_values(&alg5_values_8_8, 8, false);
}

void RNG::generate_alg5_values_32_8()
{
	_generate_alg5_values((uint8**)&alg5_values_32_8, 32, false);
}

void RNG::generate_alg5_values_32_16()
{
	_generate_alg5_values((uint8**)&alg5_values_32_16, 32, true);
}

void RNG::generate_alg5_values_64_8()
{
	_generate_alg5_values((uint8**)&alg5_values_64_8, 64, false);
}

void RNG::generate_alg5_values_64_16()
{
	_generate_alg5_values((uint8**)&alg5_values_64_16, 64, true);
}

void RNG::generate_alg5_values_128_8()
{
	_generate_alg5_values(&alg5_values_128_8, 128, false);
}

void RNG::generate_alg5_values_128_16()
{
	_generate_alg5_values(&alg5_values_128_16, 128, true);
}

void RNG::generate_alg5_values_256_8()
{
	_generate_alg5_values(&alg5_values_256_8, 256, false);
}

void RNG::generate_alg5_values_256_16()
{
	_generate_alg5_values(&alg5_values_256_16, 256, true);
}

void RNG::generate_alg5_values_512_8()
{
	_generate_alg5_values(&alg5_values_512_8, 512, false);
}

void RNG::_generate_alg6_values_for_seed(uint8* out, uint16 seed, int entries, bool packing_16)
{
	uint16 rng_seed = seed;
	for (int j = 0; j < entries; j++)
	{
		uint8 val = run_rng(&rng_seed) & 0x80;
		packing_store(out, j, val, packing_16);
	}
}

void RNG::generate_alg6_values_for_seeds(uint8** out, uint16* seeds, int seed_count, bool packing_16)
{
	const int entries = 2048;
	int stride = entries * (packing_16 ? 2 : 1);
	aligned_free(*out);
	*out = packing_alloc(seed_count * entries, packing_16, stride);
	for (int i = 0; i < seed_count; i++)
	{
		_generate_alg6_values_for_seed(*out + i * stride, seeds[i], entries, packing_16);
	}
}

void RNG::generate_alg6_values_for_seeds_8(uint8_t** out, uint16* seeds, int seed_count)
{
	generate_alg6_values_for_seeds(out, seeds, seed_count, false);
}

void RNG::_generate_alg6_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int stride = 128 * (packing_16 ? 2 : 1);
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		for (int i = 0; i < 0x10000; i++)
		{
			uint8* seed_out = *rng_values + i * stride;
			_generate_alg6_values_for_seed(seed_out, (uint16)i, 128, packing_16);

			if (shuffle)
			{
				uint8 temp[128];
				for (int k = 0; k < 128; k++)
					temp[k] = packing_load(seed_out, k, packing_16);
				for (int k = 0; k < 128; k++)
					packing_store(seed_out, shuffle_8(k, bits), temp[k], packing_16);
			}
		}
	}
}

void RNG::generate_alg6_values_128_8_shuffled()
{
	_generate_alg6_values(&alg6_values_128_8_shuffled, true, 128, false);
}

void RNG::generate_alg6_values_256_8_shuffled()
{
	_generate_alg6_values(&alg6_values_256_8_shuffled, true, 256, false);
}

void RNG::generate_alg6_values_512_8_shuffled()
{
	_generate_alg6_values(&alg6_values_512_8_shuffled, true, 512, false);
}

void RNG::generate_alg6_values_8()
{
	_generate_alg6_values(&alg6_values_8, false, -1, false);
}

void RNG::generate_alg6_values_16()
{
	_generate_alg6_values((uint8**)&alg6_values_16, false, -1, true);
}

void RNG::_generate_alg06_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			for (int j = 0; j < 128; j++)
			{
				packing_store(*rng_values, i * 128 + j, 0, packing_16);
			}
			rng_seed = i;
			for (int j = 0; j < 128; j++)
			{
				uint8 rng_res = run_rng(&rng_seed);

				uint8 alg0_val = (rng_res >> 7) & 0x01;
				uint8 alg6_val = rng_res & 0x80;

				int alg0_offset = (127 - j);
				int alg6_offset = j;
				if (shuffle)
				{
					alg0_offset = shuffle_8(alg0_offset, bits);
					alg6_offset = shuffle_8(alg6_offset, bits);
				}

				uint8 old_val = packing_load(*rng_values, i * 128 + alg0_offset, packing_16);
				packing_store(*rng_values, i * 128 + alg0_offset, old_val | alg0_val, packing_16);

				old_val = packing_load(*rng_values, i * 128 + alg6_offset, packing_16);
				packing_store(*rng_values, i * 128 + alg6_offset, old_val | alg6_val, packing_16);
			}
		}
	}
}

void RNG::generate_alg06_values_8()
{
	_generate_alg06_values((uint8**)&alg06_values_8, false, -1, false);
}

void RNG::generate_alg06_values_128_8_shuffled()
{
	_generate_alg06_values((uint8**)&alg06_values_128_8_shuffled, true, 128, false);
}

void RNG::generate_seed_forward_1()
{
	seed_forward_1 = new uint16[256 * 256];
	for (int i = 0; i < 0x10000; i++)
	{
		seed_forward_1[i] = rng_table[i];
	}
}

void RNG::generate_seed_forward_128()
{
	seed_forward_128 = new uint16[256 * 256];
	uint16 rng_seed;
	for (int i = 0; i < 0x10000; i++)
	{
		rng_seed = i;
		for (int j = 0; j < 128; j++)
		{
			rng_seed = rng_table[rng_seed];
		}
		seed_forward_128[i] = rng_seed;
	}
}

void RNG::generate_seed_forward()
{
	if (seed_forward == nullptr)
	{
		seed_forward = new uint16[256 * 256 * 2048];
		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			uint16* cur_seed_ptr = &(seed_forward[i * 2048]);
			rng_seed = i;
			for (int j = 0; j < 2048; j++)
			{
				cur_seed_ptr[j] = rng_seed;
				rng_seed = rng_table[rng_seed];
			}
		}
	}
}

void RNG::generate_rng_seq_tables()
{
	if (rng_seq_table != nullptr)
		return;

	// RNG structure (from reverse-engineering):
	//   X  cycle: 2067  seeds, starting at 0x000F (pure cycle)
	//   Y  cycle: 11429 seeds, starting at 0x0004 (pure cycle)
	//   Z0 cycle: 33153 seeds, starting at 0x0000, includes 0xAA6D (pure cycle)
	//   Z1 tail:  18887 seeds, starting at 0x2143, leads into Z0 at 0xAA6D (no predecessor)
	//   Total: 65536 seeds exactly
	//
	// Layout: [Z1 tail][Z0 cycle + 127 pad][X cycle + 127 pad][Y cycle + 127 pad]
	// Z1 tail is placed first so forward reads from tail seeds continue into Z0.
	// Only the 3 pure cycles need 127-entry wrap-around padding.
	// Max size: 65536 + 3*127 = 65917 entries.

	const uint32 MAX_SEQ_SIZE = 65536 + 3 * 127;
	rng_seq_table = (uint16*)aligned_malloc(MAX_SEQ_SIZE * sizeof(uint16), 64);
	rng_pos_table = (uint32*)aligned_malloc(0x10000 * sizeof(uint32), 64);

	bool visited[0x10000] = {};
	uint32 write_pos = 0;

	// 1. Z1 tail: walk forward from 0x2143 until we reach 0xAA6D (Z0's entry point).
	//    No padding needed — Z0 follows immediately, so reads naturally continue into it.
	{
		uint16 cur = 0x2143;
		while (cur != 0xAA6D)
		{
			rng_pos_table[cur] = write_pos;
			rng_seq_table[write_pos++] = rng_table[cur];
			visited[cur] = true;
			cur = rng_table[cur];
		}
	}

	// 2. Z0 cycle: start from 0xAA6D so it's contiguous with the Z1 tail above.
	{
		uint32 z0_base = write_pos;
		uint16 cur = 0xAA6D;
		do {
			rng_pos_table[cur] = write_pos;
			rng_seq_table[write_pos++] = rng_table[cur];
			visited[cur] = true;
			cur = rng_table[cur];
		} while (cur != 0xAA6D);
		uint32 z0_len = write_pos - z0_base;
		for (uint32 i = 0; i < 127; i++)
			rng_seq_table[write_pos++] = rng_seq_table[z0_base + (i % z0_len)];
	}

	// 3. X cycle: start from 0x000F.
	{
		uint32 x_base = write_pos;
		uint16 cur = 0x000F;
		do {
			rng_pos_table[cur] = write_pos;
			rng_seq_table[write_pos++] = rng_table[cur];
			visited[cur] = true;
			cur = rng_table[cur];
		} while (cur != 0x000F);
		uint32 x_len = write_pos - x_base;
		for (uint32 i = 0; i < 127; i++)
			rng_seq_table[write_pos++] = rng_seq_table[x_base + (i % x_len)];
	}

	// 4. Y cycle: start from 0x0004.
	{
		uint32 y_base = write_pos;
		uint16 cur = 0x0004;
		do {
			rng_pos_table[cur] = write_pos;
			rng_seq_table[write_pos++] = rng_table[cur];
			visited[cur] = true;
			cur = rng_table[cur];
		} while (cur != 0x0004);
		uint32 y_len = write_pos - y_base;
		for (uint32 i = 0; i < 127; i++)
			rng_seq_table[write_pos++] = rng_seq_table[y_base + (i % y_len)];
	}

	rng_seq_table_size = write_pos;

	// Verify all 65536 seeds were covered.
	uint32 missed = 0;
	for (uint32 s = 0; s < 0x10000; s++)
	{
		if (!visited[s])
		{
			fprintf(stderr, "generate_rng_seq_tables: unvisited seed 0x%04X\n", s);
			missed++;
		}
	}
	if (missed == 0)
		fprintf(stderr, "generate_rng_seq_tables: all 65536 seeds OK, table size=%u\n", write_pos);
	else
		fprintf(stderr, "generate_rng_seq_tables: ERROR: %u seeds not visited!\n", missed);
}

void RNG::cleanup()
{
	delete[] rng_table;          rng_table = nullptr;
	delete[] seed_forward;       seed_forward = nullptr;
	delete[] seed_forward_1;     seed_forward_1 = nullptr;
	delete[] seed_forward_128;   seed_forward_128 = nullptr;

	aligned_free(regular_rng_values_8);                 regular_rng_values_8 = nullptr;
	aligned_free(regular_rng_values_128_8_shuffled);    regular_rng_values_128_8_shuffled = nullptr;
	aligned_free(regular_rng_values_256_8_shuffled);    regular_rng_values_256_8_shuffled = nullptr;
	aligned_free(regular_rng_values_512_8_shuffled);    regular_rng_values_512_8_shuffled = nullptr;
	aligned_free(regular_rng_values_8_hi);              regular_rng_values_8_hi = nullptr;
	aligned_free(regular_rng_values_256_8_shuffled_hi); regular_rng_values_256_8_shuffled_hi = nullptr;
	aligned_free(regular_rng_values_8_lo);              regular_rng_values_8_lo = nullptr;
	aligned_free(regular_rng_values_256_8_shuffled_lo); regular_rng_values_256_8_shuffled_lo = nullptr;
	aligned_free(regular_rng_values_16);                regular_rng_values_16 = nullptr;

	aligned_free(expansion_values_8);               expansion_values_8 = nullptr;
	aligned_free(expansion_values_128_8_shuffled);  expansion_values_128_8_shuffled = nullptr;
	aligned_free(expansion_values_256_8_shuffled);  expansion_values_256_8_shuffled = nullptr;

	aligned_free(alg0_values_8);                alg0_values_8 = nullptr;
	aligned_free(alg0_values_128_8_shuffled);   alg0_values_128_8_shuffled = nullptr;
	aligned_free(alg0_values_256_8_shuffled);   alg0_values_256_8_shuffled = nullptr;
	aligned_free(alg0_values_512_8_shuffled);   alg0_values_512_8_shuffled = nullptr;
	aligned_free(alg0_values_16);               alg0_values_16 = nullptr;

	aligned_free(alg2_values_8_8);    alg2_values_8_8 = nullptr;
	aligned_free(alg2_values_32_8);   alg2_values_32_8 = nullptr;
	aligned_free(alg2_values_32_16);  alg2_values_32_16 = nullptr;
	aligned_free(alg2_values_64_8);   alg2_values_64_8 = nullptr;
	aligned_free(alg2_values_64_16);  alg2_values_64_16 = nullptr;
	aligned_free(alg2_values_128_8);  alg2_values_128_8 = nullptr;
	aligned_free(alg2_values_128_16); alg2_values_128_16 = nullptr;
	aligned_free(alg2_values_256_8);  alg2_values_256_8 = nullptr;
	aligned_free(alg2_values_256_16); alg2_values_256_16 = nullptr;
	aligned_free(alg2_values_512_8);  alg2_values_512_8 = nullptr;

	aligned_free(alg4_values_8);                alg4_values_8 = nullptr;
	aligned_free(alg4_values_8_hi);             alg4_values_8_hi = nullptr;
	aligned_free(alg4_values_128_8_shuffled);   alg4_values_128_8_shuffled = nullptr;
	aligned_free(alg4_values_256_8_shuffled);   alg4_values_256_8_shuffled = nullptr;
	aligned_free(alg4_values_512_8_shuffled);   alg4_values_512_8_shuffled = nullptr;
	aligned_free(alg4_values_256_8_shuffled_hi);alg4_values_256_8_shuffled_hi = nullptr;
	aligned_free(alg4_values_8_lo);             alg4_values_8_lo = nullptr;
	aligned_free(alg4_values_256_8_shuffled_lo);alg4_values_256_8_shuffled_lo = nullptr;
	aligned_free(alg4_values_16);               alg4_values_16 = nullptr;

	aligned_free(alg5_values_8_8);    alg5_values_8_8 = nullptr;
	aligned_free(alg5_values_32_8);   alg5_values_32_8 = nullptr;
	aligned_free(alg5_values_32_16);  alg5_values_32_16 = nullptr;
	aligned_free(alg5_values_64_8);   alg5_values_64_8 = nullptr;
	aligned_free(alg5_values_64_16);  alg5_values_64_16 = nullptr;
	aligned_free(alg5_values_128_8);  alg5_values_128_8 = nullptr;
	aligned_free(alg5_values_128_16); alg5_values_128_16 = nullptr;
	aligned_free(alg5_values_256_8);  alg5_values_256_8 = nullptr;
	aligned_free(alg5_values_256_16); alg5_values_256_16 = nullptr;
	aligned_free(alg5_values_512_8);  alg5_values_512_8 = nullptr;

	aligned_free(alg6_values_8);                alg6_values_8 = nullptr;
	aligned_free(alg6_values_128_8_shuffled);   alg6_values_128_8_shuffled = nullptr;
	aligned_free(alg6_values_256_8_shuffled);   alg6_values_256_8_shuffled = nullptr;
	aligned_free(alg6_values_512_8_shuffled);   alg6_values_512_8_shuffled = nullptr;
	aligned_free(alg6_values_16);               alg6_values_16 = nullptr;

	aligned_free(alg06_values_8);              alg06_values_8 = nullptr;
	aligned_free(alg06_values_128_8_shuffled); alg06_values_128_8_shuffled = nullptr;

	aligned_free(rng_seq_table); rng_seq_table = nullptr;
	aligned_free(rng_pos_table); rng_pos_table = nullptr;
}

uint16* RNG::rng_table = nullptr;
uint16* RNG::rng_seq_table      = nullptr;
uint32* RNG::rng_pos_table      = nullptr;
uint32  RNG::rng_seq_table_size = 0;

uint8* RNG::regular_rng_values_8 = nullptr;
uint8* RNG::regular_rng_values_128_8_shuffled = nullptr;
uint8* RNG::regular_rng_values_256_8_shuffled = nullptr;
uint8* RNG::regular_rng_values_512_8_shuffled = nullptr;
uint8* RNG::regular_rng_values_8_hi = nullptr;
uint8* RNG::regular_rng_values_256_8_shuffled_hi = nullptr;
uint8* RNG::regular_rng_values_8_lo = nullptr;
uint8* RNG::regular_rng_values_256_8_shuffled_lo = nullptr;
uint16* RNG:: regular_rng_values_16 = nullptr;

uint8* RNG::expansion_values_8 = nullptr;
uint8* RNG::expansion_values_128_8_shuffled = nullptr;
uint8* RNG::expansion_values_256_8_shuffled = nullptr;

uint16* RNG::seed_forward = nullptr;
uint16* RNG::seed_forward_1 = nullptr;
uint16* RNG::seed_forward_128 = nullptr;


uint8* RNG::alg0_values_8 = nullptr;
uint8* RNG::alg0_values_128_8_shuffled = nullptr;
uint8* RNG::alg0_values_256_8_shuffled = nullptr;
uint8* RNG::alg0_values_512_8_shuffled = nullptr;
uint16* RNG::alg0_values_16 = nullptr;

uint8* RNG::alg2_values_8_8 = nullptr;
uint32* RNG::alg2_values_32_8 = nullptr;
uint32* RNG::alg2_values_32_16 = nullptr;
uint64* RNG::alg2_values_64_8 = nullptr;
uint64* RNG::alg2_values_64_16 = nullptr;
uint8* RNG::alg2_values_128_8 = nullptr;
uint8* RNG::alg2_values_128_16 = nullptr;
uint8* RNG::alg2_values_256_8 = nullptr;
uint8* RNG::alg2_values_256_16 = nullptr;
uint8* RNG::alg2_values_512_8 = nullptr;

uint8* RNG::alg4_values_8 = nullptr;
uint8* RNG::alg4_values_8_hi = nullptr;
uint8* RNG::alg4_values_128_8_shuffled = nullptr;
uint8* RNG::alg4_values_256_8_shuffled = nullptr;
uint8* RNG::alg4_values_512_8_shuffled = nullptr;
uint8* RNG::alg4_values_256_8_shuffled_hi = nullptr;
uint8* RNG::alg4_values_8_lo = nullptr;
uint8* RNG::alg4_values_256_8_shuffled_lo = nullptr;
uint16* RNG::alg4_values_16 = nullptr;

uint8* RNG::alg5_values_8_8 = nullptr;
uint32* RNG::alg5_values_32_8 = nullptr;
uint32* RNG::alg5_values_32_16 = nullptr;
uint64* RNG::alg5_values_64_8 = nullptr;
uint64* RNG::alg5_values_64_16 = nullptr;
uint8* RNG::alg5_values_128_8 = nullptr;
uint8* RNG::alg5_values_128_16 = nullptr;
uint8* RNG::alg5_values_256_8 = nullptr;
uint8* RNG::alg5_values_256_16 = nullptr;
uint8* RNG::alg5_values_512_8 = nullptr;

uint8* RNG::alg6_values_8 = nullptr;
uint8* RNG::alg6_values_128_8_shuffled = nullptr;
uint8* RNG::alg6_values_256_8_shuffled = nullptr;
uint8* RNG::alg6_values_512_8_shuffled = nullptr;
uint16* RNG::alg6_values_16 = nullptr;

uint8* RNG::alg06_values_8 = nullptr;
uint8* RNG::alg06_values_128_8_shuffled = nullptr;