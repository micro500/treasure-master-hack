#include "../tm_8bit/data_sizes.h"
#include "tm_64bit.h"

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


void generate_regular_rng_values(uint16 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
		*rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            regular_rng_values[i*128 + (127 - j)] = run_rng(rng_seed, rng_table);
        }
    }
}

void generate_alg0_values(uint16 * alg0_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg0_values[i*128 + (127 - j)] = (run_rng(rng_seed, rng_table) >> 7) & 0x01;
        }
    }
}

void generate_alg6_values(uint16 * alg6_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg6_values[i*128 + j] = run_rng(rng_seed, rng_table) & 0x80;
        }
    }
}

void generate_alg2_values(uint64 * alg2_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        alg2_values[i] = ((uint64)(run_rng(rng_seed, rng_table) & 0x80)) << 41;
    }
}

void generate_alg5_values(uint64 * alg5_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        alg5_values[i] = ((uint64)(run_rng(rng_seed, rng_table) & 0x80)) << 48;
    }
}


void generate_seed_forward_1(uint16 * values, uint16 * rng_table)
{
	for (int i = 0; i < 0x10000; i++)
    {
		values[i] = rng_table[i];
    }
}

void generate_seed_forward_128(uint16 * values, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 0; i < 0x10000; i++)
    {
		*rng_seed = i;
		for (int j = 0; j < 128; j++)
		{
			*rng_seed = rng_table[*rng_seed];
		}
		values[i] = *rng_seed;
    }
}





void alg0(uint64 * working_code, uint64 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = ((working_code[i] << 1) | alg0_values[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg1(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = (working_code[i] + regular_rng_values[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg2(uint64 * working_code, uint64 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    uint64 carry = alg2_values[*rng_seed];
    for (int i = 31; i >= 0; i--)
    {
        uint64 next_carry = (working_code[i] & 0x0000000000000001ull) << 48;
        
		carry = carry | ((working_code[i] & 0x0000000100000000ull) >> 16);
        working_code[i] = ((working_code[i] >> 1) & 0x0000007F0000007Full) | ((working_code[i] << 1) & 0x00FE000000FE0000ull) | ((working_code[i] >> 16) & 0x0000008000000080ull) | carry;
        
        carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg3(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = working_code[i] ^ regular_rng_values[(*rng_seed * 128) / 4 + i];
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg4(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = (working_code[i] + (regular_rng_values[(*rng_seed * 128) / 4 + i] ^ 0x00FF00FF00FF00FFull) + 0x0001000100010001ull) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg5(uint64* working_code, uint64 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    uint64 carry = alg5_values[*rng_seed];
    for (int i = 31; i >= 0; i--)
    {
        uint64 next_carry = (working_code[i] & 0x0000000000000080ull) << 48;
        
		carry = carry | ((working_code[i] & 0x0000008000000000ull) >> 16);
        working_code[i] = ((working_code[i] << 1) & 0x000000FE000000FEull) | ((working_code[i] >> 1) & 0x007F0000007F0000ull) | ((working_code[i] >> 16) & 0x0000000100000001ull) | carry;
        
        carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg6(uint64 * working_code, uint64 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = ((working_code[i] >> 1) | alg6_values[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg7(uint64 * working_code)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = working_code[i] ^ 0x00FF00FF00FF00FFull;
    }
}