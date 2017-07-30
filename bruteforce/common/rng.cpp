#include "rng.h"
#include "data_sizes.h"

void generate_rng_table(uint16 * rng_table)
{
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

uint8 run_rng(uint16 * rng_seed, uint16 * rng_table)
{
	uint16 result = rng_table[*rng_seed];
	*rng_seed = result;

	return ((result >> 8) ^ (result)) & 0xFF;
}


void generate_regular_rng_values_8(uint8 * regular_rng_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
		rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            regular_rng_values[i*128 + (127 - j)] = run_rng(&rng_seed, rng_table);
        }
    }
}

void generate_regular_rng_values_16(uint16 * regular_rng_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
		rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            regular_rng_values[i*128 + (127 - j)] = run_rng(&rng_seed, rng_table);
        }
    }
}

void generate_regular_rng_values_8_hi(uint8 * regular_rng_values_hi, uint16 * rng_table)
{
	generate_regular_rng_values_8(regular_rng_values_hi, rng_table);

    for (int i = 0; i < 0x10000; i++)
    {
        for (int j = 0; j < 128; j+=2)
        {
            regular_rng_values_hi[i*128 + (127 - j) + 1] = 0;
        }
    }
}

void generate_regular_rng_values_8_lo(uint8 * regular_rng_values_lo, uint16 * rng_table)
{
	generate_regular_rng_values_8(regular_rng_values_lo, rng_table);

    for (int i = 0; i < 0x10000; i++)
    {
        for (int j = 0; j < 128; j+=2)
        {
            regular_rng_values_lo[i*128 + (127 - j)] = 0;
        }
    }
}

void generate_alg0_values_8(uint8 * alg0_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg0_values[i*128 + (127 - j)] = (run_rng(&rng_seed, rng_table) >> 7) & 0x01;
        }
    }
}

void generate_alg0_values_16(uint16 * alg0_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg0_values[i*128 + (127 - j)] = (run_rng(&rng_seed, rng_table) >> 7) & 0x01;
        }
    }
}

void generate_alg4_values_8_hi(uint8 * alg4_values_hi, uint16 * rng_table)
{
	generate_regular_rng_values_8(alg4_values_hi, rng_table);

    for (int i = 0; i < 0x10000; i++)
    {
        for (int j = 0; j < 128; j+=2)
        {
			alg4_values_hi[i*128 + (127 - j)] = (alg4_values_hi[i*128 + (127 - j)] ^ 0xFF) + 1;
            alg4_values_hi[i*128 + (127 - j) + 1] = 0;
        }
    }
}

void generate_alg4_values_8_lo(uint8 * alg4_values_lo, uint16 * rng_table)
{
	generate_regular_rng_values_8(alg4_values_lo, rng_table);

    for (int i = 0; i < 0x10000; i++)
    {
        for (int j = 0; j < 128; j+=2)
        {
            alg4_values_lo[i*128 + (127 - j)] = 0;
			alg4_values_lo[i*128 + (127 - j) + 1] = (alg4_values_lo[i*128 + (127 - j) + 1] ^ 0xFF) + 1;
        }
    }
}

void generate_alg4_values_16(uint16 * alg4_values, uint16 * rng_table)
{
	generate_regular_rng_values_16(alg4_values, rng_table);

    for (int i = 0; i < 0x10000; i++)
    {
        for (int j = 0; j < 128; j++)
        {
			alg4_values[i*128 + j] = (alg4_values[i*128 + j] ^ 0x00FF) + 1;
        }
    }
}

void generate_alg6_values_8(uint8 * alg6_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg6_values[i*128 + j] = run_rng(&rng_seed, rng_table) & 0x80;
        }
    }
}

void generate_alg6_values_16(uint16 * alg6_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg6_values[i*128 + j] = run_rng(&rng_seed, rng_table) & 0x80;
        }
    }
}

void generate_alg2_values_32_8(uint32 * alg2_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg2_values[i] = (run_rng(&rng_seed, rng_table) & 0x80) << 17;
    }
}

void generate_alg2_values_32_16(uint32 * alg2_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg2_values[i] = (run_rng(&rng_seed, rng_table) & 0x80) << 9;
    }
}

void generate_alg2_values_64_8(uint64 * alg2_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg2_values[i] = ((uint64)run_rng(&rng_seed, rng_table) & 0x80) << 49;
    }
}

void generate_alg2_values_64_16(uint64 * alg2_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg2_values[i] = ((uint64)run_rng(&rng_seed, rng_table) & 0x80) << 41;
    }
}

void generate_alg2_values_128_16(uint8 * alg2_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 16; j++)
		{
			alg2_values[i*16 + j] = 0;
		}
        rng_seed = i;
        alg2_values[i*16+14] = (run_rng(&rng_seed, rng_table) & 0x80) >> 7;
    }
}

void generate_alg2_values_256_16(uint8 * alg2_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 132; j++)
		{
			alg2_values[i*32 + j] = 0;
		}
        rng_seed = i;
        alg2_values[i*32+30] = (run_rng(&rng_seed, rng_table) & 0x80) >> 7;
    }
}

void generate_alg5_values_32_8(uint32 * alg5_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg5_values[i] = (run_rng(&rng_seed, rng_table) & 0x80) << 24;
    }
}

void generate_alg5_values_32_16(uint32 * alg5_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg5_values[i] = (run_rng(&rng_seed, rng_table) & 0x80) << 16;
    }
}

void generate_alg5_values_64_8(uint64 * alg5_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg5_values[i] = ((uint64)run_rng(&rng_seed, rng_table) & 0x80) << 56;
    }
}

void generate_alg5_values_64_16(uint64 * alg5_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
        rng_seed = i;
        alg5_values[i] = ((uint64)run_rng(&rng_seed, rng_table) & 0x80) << 48;
    }
}

void generate_alg5_values_128_16(uint8 * alg5_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 16; j++)
		{
			alg5_values[i*16 + j] = 0;
		}
        rng_seed = i;
		alg5_values[i*16+14] = run_rng(&rng_seed, rng_table) & 0x80;
    }
}

void generate_alg5_values_256_16(uint8 * alg5_values, uint16 * rng_table)
{
	uint16 rng_seed;
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 32; j++)
		{
			alg5_values[i*32 + j] = 0;
		}
        rng_seed = i;
		alg5_values[i*32+30] = run_rng(&rng_seed, rng_table) & 0x80;
    }
}

void generate_seed_forward_1(uint16 * values, uint16 * rng_table)
{
	for (int i = 0; i < 0x10000; i++)
    {
		values[i] = rng_table[i];
    }
}

void generate_seed_forward_128(uint16 * values, uint16 * rng_table)
{
	uint16 rng_seed;
	for (int i = 0; i < 0x10000; i++)
    {
		rng_seed = i;
		for (int j = 0; j < 128; j++)
		{
			rng_seed = rng_table[rng_seed];
		}
		values[i] = rng_seed;
    }
}
