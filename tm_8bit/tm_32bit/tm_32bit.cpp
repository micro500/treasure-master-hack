#include "..\tm_8bit\data_sizes.h"
#include "tm_32bit.h"

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

void generate_alg2_values(uint32 * alg2_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        alg2_values[i] = (run_rng(rng_seed, rng_table) & 0x80) << 9;
    }
}

void generate_alg5_values(uint32 * alg5_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        alg5_values[i] = (run_rng(rng_seed, rng_table) & 0x80) << 16;
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


void alg0(uint32 * working_code, uint32 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 64; i++)
    {
        working_code[i] = ((working_code[i] << 1) | alg0_values[(*rng_seed * 128) / 2 + i]) & 0xFF00FF;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg1(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 64; i++)
    {
        working_code[i] = (working_code[i] + regular_rng_values[(*rng_seed * 128) / 2 + i]) & 0xFF00FF;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg2(uint32 * working_code, uint32 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    uint32 carry = alg2_values[*rng_seed];
    for (int i = 63; i >= 0; i--)
    {
        uint32 next_carry = (working_code[i] & 0x00000001) << 16;
        
        working_code[i] = (((working_code[i] & 0x000000FF) >> 1) & 0x7F) | (((working_code[i] & 0x00FF0000) << 1) & 0x00FE0000) | ((working_code[i] & 0x00800000) >> 16) | carry;
        
        carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg3(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 64; i++)
    {
        working_code[i] = working_code[i] ^ regular_rng_values[(*rng_seed * 128) / 2 + i];
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg4(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 64; i++)
    {
        working_code[i] = (working_code[i] + (regular_rng_values[(*rng_seed * 128) / 2 + i] ^ 0xFF00FF) + 0x010001) & 0xFF00FF;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg5(uint32 * working_code, uint32 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    uint32 carry = alg5_values[*rng_seed];
    for (int i = 63; i >= 0; i--)
    {
        uint32 next_carry = (working_code[i] & 0x00000080) << 16;
        
        working_code[i] = (((working_code[i] & 0x000000FF) << 1) & 0xFE) | (((working_code[i] & 0x00FF0000) >> 1) & 0x007F0000) | ((working_code[i] & 0x00010000) >> 16) | carry;
        
        carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg6(uint32 * working_code, uint32 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 64; i++)
    {
        working_code[i] = ((working_code[i] >> 1) | alg6_values[(*rng_seed * 128) / 2 + i]) & 0xFF00FF;
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg7(uint32 * working_code)
{
    for (int i = 0; i < 64; i++)
    {
        working_code[i] = working_code[i] ^ 0xFF00FF;
    }
}
