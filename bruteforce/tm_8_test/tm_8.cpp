#include "data_sizes.h"
#include "tm_8.h"

void working_code_alg_0 (uint8 * working_code, uint8 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	for (int i = 0; i < 128; i++)
	{
		working_code[i] = (working_code[i] << 1) | alg0_values[*rng_seed * 128 + i];
	}
	*rng_seed = rng_forward[*rng_seed];
}

void working_code_alg_1(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	for (int i = 0; i < 128; i++)
	{
		working_code[i] = working_code[i] + regular_rng_values[*rng_seed * 128 + i];
	}
	*rng_seed = rng_forward[*rng_seed];
}

void working_code_alg_2(uint8 * working_code, uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	uint8 carry = alg2_values[*rng_seed];
	for (int i = 127; i >= 0; i-=2)
	{
		uint8 next_carry = working_code[i-1] & 0x01;
    
		working_code[i-1] = (working_code[i-1] >> 1) | (working_code[i] & 0x80);
		working_code[i] = (working_code[i] << 1) | (carry & 0x01);

		carry = next_carry;
	}
	*rng_seed = rng_forward[*rng_seed];
}


void working_code_alg_3(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	for (int i = 0; i < 128; i++)
	{
		working_code[i] = working_code[i] ^ regular_rng_values[*rng_seed * 128 + i];
	}

	*rng_seed = rng_forward[*rng_seed];
}

void working_code_alg_5(uint8 * working_code, uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	uint8 carry = alg5_values[*rng_seed];

	for (int i = 127; i >= 0; i-=2)
	{
		uint8 next_carry = working_code[i-1] & 0x80;
    
		working_code[i-1] = (working_code[i-1] << 1) | (working_code[i] & 0x01);
		working_code[i] = (working_code[i] >> 1) | carry;

		carry = next_carry;
	}
	*rng_seed = rng_forward[*rng_seed];
}


void working_code_alg_6(uint8 * working_code, uint8 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	for (int i = 0; i < 128; i++)
	{
		working_code[i] = (working_code[i] >> 1) |  alg6_values[*rng_seed * 128 + i];
	}
	*rng_seed = rng_forward[*rng_seed];
}

void working_code_alg_7 (uint8 * working_code)
{
	for (int i = 0; i < 128; i++)
	{
		working_code[i] = working_code[i] ^ 0xFF;
	}
}
