#include "data_sizes.h"
#include "tm_32_8.h"

void alg0(uint32 * working_code, uint32 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = ((working_code[i] << 1) & 0xFEFEFEFE) | alg0_values[(*rng_seed * 128) / 4 + i];
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg1(uint32 * working_code, uint32 * regular_rng_values_lo, uint32 * regular_rng_values_hi, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = (((working_code[i] & 0x00FF00FF) + regular_rng_values_lo[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF) | (((working_code[i] & 0xFF00FF00) + regular_rng_values_hi[(*rng_seed * 128) / 4 + i]) & 0xFF00FF00);
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg2(uint32 * working_code, uint32 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    uint32 carry = alg2_values[*rng_seed];
    for (int i = 31; i >= 0; i--)
    {
        uint32 next_carry = (working_code[i] & 0x00000001) << 24;
        
        working_code[i] = ((working_code[i] & 0x00010000) >> 8) | ((working_code[i] >> 1) & 0x007F007F) | ((working_code[i] << 1) & 0xFE00FE00) | ((working_code[i] >> 8) & 0x00800080) | carry;
        
        carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg3(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = working_code[i] ^ regular_rng_values[(*rng_seed * 128) / 4 + i];
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg5(uint32 * working_code, uint32 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    uint32 carry = alg5_values[*rng_seed];
    for (int i = 31; i >= 0; i--)
    {
        uint32 next_carry = (working_code[i] & 0x00000080) << 24;
        
        working_code[i] = ((working_code[i] & 0x00800000) >> 8) | ((working_code[i] << 1) & 0x00FE00FE) | ((working_code[i] >> 1) & 0x7F007F00) | ((working_code[i] >> 8) & 0x00010001) | carry;
        
        carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg6(uint32 * working_code, uint32 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = ((working_code[i] >> 1) &0x7F7F7F7F) | alg6_values[(*rng_seed * 128) / 4 + i];
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg7(uint32 * working_code)
{
    for (int i = 0; i < 32; i++)
    {
        working_code[i] = working_code[i] ^ 0xFFFFFFFF;
    }
}
