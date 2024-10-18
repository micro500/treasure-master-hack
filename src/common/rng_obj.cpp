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

void RNG::_generate_regular_rng_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		*rng_values = packing_alloc(0x10000 * 128, packing_16);
		
		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			rng_seed = i;
			for (int j = 0; j < 128; j++)
			{
				int offset = (127 - j);
				if (shuffle)
				{
					offset = shuffle_8(offset, bits);
				}

				uint8 val = run_rng(&rng_seed);
				packing_store(*rng_values, i * 128 + offset, val, packing_16);
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

void RNG::_generate_expansion_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		uint16 rng_seed;
		uint8 temp_values[8];

		for (int i = 0; i < 0x10000; i++)
		{
			rng_seed = i;
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
					packing_store(*rng_values, i * 128 + offset, temp_values[k], packing_16);
				}
			}
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


void RNG::_generate_alg0_values(uint8**rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			rng_seed = i;
			for (int j = 0; j < 128; j++)
			{
				int offset = (127 - j);
				if (shuffle)
				{
					offset = shuffle_8(offset, bits);
				}

				uint8 val = (run_rng(&rng_seed) >> 7) & 0x01;
				packing_store(*rng_values, i * 128 + offset, val, packing_16);
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

void RNG::_generate_alg2_values(uint8** rng_values, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int bytes = bits / 8;
		*rng_values = (uint8*)aligned_malloc(0x10000 * bytes, 64);
		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			for (int j = 0; j < bytes; j++)
			{
				(*rng_values)[i * bytes + j] = 0;
			}
			rng_seed = i;
			(*rng_values)[i * bytes + (bytes - 1 + (packing_16 ? -1 : 0))] = (run_rng(&rng_seed) & 0x80) >> 7;
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
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			rng_seed = i;
			for (int j = 0; j < 128; j++)
			{
				int offset = (127 - j);
				if (shuffle)
				{
					offset = shuffle_8(offset, bits);
				}

				uint8 val = (run_rng(&rng_seed) ^ 0xFF) + 1;
				packing_store(*rng_values, i * 128 + offset, val, packing_16);
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

void RNG::_generate_alg5_values(uint8** rng_values, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		int bytes = bits / 8;
		*rng_values = (uint8*)aligned_malloc(0x10000 * bytes, 64);
		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			for (int j = 0; j < bytes; j++)
			{
				(*rng_values)[i * bytes + j] = 0;
			}
			rng_seed = i;
			(*rng_values)[i * bytes + (bytes - 1 + (packing_16 ? -1 : 0))] = run_rng(&rng_seed) & 0x80;
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

void RNG::_generate_alg6_values(uint8** rng_values, bool shuffle, int bits, bool packing_16)
{
	if (*rng_values == nullptr)
	{
		*rng_values = packing_alloc(0x10000 * 128, packing_16);

		uint16 rng_seed;
		for (int i = 0; i < 0x10000; i++)
		{
			rng_seed = i;
			for (int j = 0; j < 128; j++)
			{
				int offset = j;
				if (shuffle)
				{
					offset = shuffle_8(offset, bits);
				}

				uint8 val = run_rng(&rng_seed) & 0x80;
				packing_store(*rng_values, i * 128 + offset, val, packing_16);
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
			rng_seed = i;
			for (int j = 0; j < 128; j++)
			{
				int offset = (127 - j);
				if (shuffle)
				{
					offset = shuffle_8(offset, bits);
				}

				uint8 rng_res = run_rng(&rng_seed);

				uint8 alg0_val = (rng_res >> 7) & 0x01;
				uint8 alg6_val = rng_res & 0x80;
				packing_store(*rng_values, i * 128 + offset, alg0_val | alg6_val, packing_16);
			}
		}
	}
}

void RNG::generate_alg06_values_8()
{
	_generate_alg06_values((uint8**)&alg06_values_8, false, -1, false);
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
		for (int i = 0x6f0f; i < 0x10000; i++)
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

uint16* RNG::rng_table = nullptr;

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