#include "data_sizes.h"
#include "tm_8bit.h"
#include "rng.h"

void working_code_alg_0 (uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 127; i >= 0; i--)
	{
		working_code[i] = (working_code[i] << 1) | ((run_rng(rng_seed, rng_table) >> 7) & 0x01);
	}
}

void working_code_alg_1(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 127; i >= 0; i--)
	{
		working_code[i] = working_code[i] + run_rng(rng_seed, rng_table);
	}
}

void working_code_alg_2(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	uint8 carry = (run_rng(rng_seed, rng_table) >> 7) & 0x01;
	for (int i = 0x7F; i >= 0; i-=2)
	{
		uint8 next_carry = working_code[i-1] & 0x01;
    
		working_code[i-1] = (working_code[i-1] >> 1) | (working_code[i] & 0x80);
		working_code[i] = (working_code[i] << 1) | (carry & 0x01);

		carry = next_carry;
	}
}


void working_code_alg_3(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 127; i >= 0; i--)
	{
		working_code[i] = working_code[i] ^ run_rng(rng_seed, rng_table);
	}
}

void working_code_alg_4(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 127; i >= 0; i--)
	{
		//working_code[i] = working_code[i] + (run_rng(rng_seed, rng_table) ^ 0xFF) + 1;
		working_code[i] = working_code[i] - (run_rng(rng_seed, rng_table));
	}
}

void working_code_alg_5(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	uint8 carry = run_rng(rng_seed, rng_table) & 0x80;

	for (int i = 0x7F; i >= 0; i-=2)
	{
		uint8 next_carry = working_code[i-1] & 0x80;
    
		working_code[i-1] = (working_code[i-1] << 1) | (working_code[i] & 0x01);
		working_code[i] = (working_code[i] >> 1) | carry;

		carry = next_carry;
	}
}


void working_code_alg_6 (uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 0; i < 128; i++)
	{
		working_code[i] = (working_code[i] >> 1) | (run_rng(rng_seed, rng_table) & 0x80);
	}
}

void working_code_alg_7 (uint8 * working_code, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 127; i >= 0; i--)
	{
		working_code[i] = working_code[i] ^ 0xFF;
	}
}
