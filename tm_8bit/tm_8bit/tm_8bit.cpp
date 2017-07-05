#include "data_sizes.h"
#include "tm_8bit.h"

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
