#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A

#include "../tm_8bit/data_sizes.h"

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

void generate_alg2_values(uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 16; j++)
		{
			alg2_values[i*16 + j] = 0;
		}
        *rng_seed = i;
        alg2_values[i*16+14] = (run_rng(rng_seed, rng_table) & 0x80) >> 7;
    }
}

void generate_alg5_values(uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 16; j++)
		{
			alg5_values[i*16 + j] = 0;
		}
        *rng_seed = i;
		alg5_values[i*16+14] = run_rng(rng_seed, rng_table) & 0x80;
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




void alg0(uint8 * working_code, uint8 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	//return;
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(alg0_values + ((*rng_seed) * 128 * 2) + (i * 16)));
		cur_val = _mm_slli_epi16(cur_val,1);
		cur_val = _mm_or_si128 (cur_val, rng_val);

		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128  (cur_val, mask);
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = ((working_code[i] << 1) | alg0_values[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg1(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	//return;
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(regular_rng_values + ((*rng_seed) * 128 * 2) + (i * 16)));
		cur_val = _mm_add_epi16 (cur_val, rng_val);
		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128  (cur_val, mask);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = (working_code[i] + regular_rng_values[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg2(uint8 * working_code, uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	//return;
    //uint64 carry = alg2_values[*rng_seed];
	__m128i carry = _mm_loadu_si128((__m128i *)(alg2_values + ((*rng_seed) * 16)));
    for (int i = 15; i >= 0; i--)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val,_mm_set_epi16(0,0,0,0,0,0,0,1)),14);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val,_mm_set_epi16(0,1,0,1,0,1,0,0)),2));

		//_mm_and_si128(_mm_slli_si128(cur_val,15),_mm_set_epi16(0,0x80,0,0x80,0,0x80,0,0x80));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val,1), 
						_mm_set_epi16(0,0x7f,0,0x7f,0,0x7f,0,0x7f)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val,1), 
						_mm_set_epi16(0xFE,0,0xFE,0,0xFE,0,0xFE,0))),
				_mm_and_si128(
					_mm_srli_si128(cur_val,2),
					_mm_set_epi16(0,0x80,0,0x80,0,0x80,0,0x80))),
			carry);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

		carry = next_carry;

		/*
		working_code[i] = ((working_code[i] >> 1) & 0x0000007F0000007Full) | ((working_code[i] << 1) & 0x00FE000000FE0000ull) | ((working_code[i] >> 16) & 0x0000008000000080ull) | carry;



        uint64 next_carry = (working_code[i] & 0x0000000000000001ull) << 48;
        
		carry = carry | ((working_code[i] & 0x0000000100000000ull) >> 16);
        working_code[i] = ((working_code[i] >> 1) & 0x0000007F0000007Full) | ((working_code[i] << 1) & 0x00FE000000FE0000ull) | ((working_code[i] >> 16) & 0x0000008000000080ull) | carry;
        
        carry = next_carry;
		*/
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg3(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(regular_rng_values + ((*rng_seed) * 128 * 2) + (i * 16)));
		cur_val = _mm_xor_si128 (cur_val, rng_val);
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = working_code[i] ^ regular_rng_values[(*rng_seed * 128) / 4 + i];
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg4(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	//return;
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(regular_rng_values + ((*rng_seed) * 128 * 2) + (i * 16)));
		__m128i mask = _mm_set1_epi16(0x00FF);
		rng_val = _mm_xor_si128 (rng_val, mask);
		cur_val = _mm_add_epi16 (cur_val, rng_val);
		__m128i one = _mm_set1_epi16(0x0001);
		cur_val = _mm_add_epi16 (cur_val, one);
		
		cur_val = _mm_and_si128  (cur_val, mask);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = (working_code[i] + (regular_rng_values[(*rng_seed * 128) / 4 + i] ^ 0x00FF00FF00FF00FFull) + 0x0001000100010001ull) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg5(uint8* working_code, uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m128i carry = _mm_loadu_si128((__m128i *)(alg5_values + ((*rng_seed) * 16)));
    for (int i = 15; i >= 0; i--)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val,_mm_set_epi16(0,0,0,0,0,0,0,0x80)),14);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val,_mm_set_epi16(0,0x80,0,0x80,0,0x80,0,0)),2));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val,1), 
						_mm_set_epi16(0x7f,0,0x7f,0,0x7f,0,0x7f,0)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val,1), 
						_mm_set_epi16(0,0xFE,0,0xFE,0,0xFE,0,0xFE))),
				_mm_and_si128(
					_mm_srli_si128(cur_val,2),
					_mm_set_epi16(0,1,0,1,0,1,0,1))),
			carry);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

		carry = next_carry;

    }
	*rng_seed = rng_forward[*rng_seed];





	/*
    uint64 carry = alg5_values[*rng_seed];
    for (int i = 31; i >= 0; i--)
    {
        uint64 next_carry = (working_code[i] & 0x0000000000000080ull) << 48;
        
		carry = carry | ((working_code[i] & 0x0000008000000000ull) >> 16);
        working_code[i] = ((working_code[i] << 1) & 0x000000FE000000FEull) | ((working_code[i] >> 1) & 0x007F0000007F0000ull) | ((working_code[i] >> 16) & 0x0000000100000001ull) | carry;
        
        carry = next_carry;
    }
	
	*rng_seed = rng_forward[*rng_seed];
	*/
}


void alg6(uint8 * working_code, uint8 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	//return;
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(alg6_values + ((*rng_seed) * 128 * 2) + (i * 16)));
		cur_val = _mm_srli_epi16(cur_val,1);
		cur_val = _mm_or_si128 (cur_val, rng_val);

		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128  (cur_val, mask);
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = ((working_code[i] >> 1) | alg6_values[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg7(uint8 * working_code)
{
	//return;
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_xor_si128 (cur_val, mask);
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = working_code[i] ^ 0x00FF00FF00FF00FFull;
    }
}

void * aligned_malloc(int byte_count, int align_size)
{
	uint8 * new_array = new uint8[byte_count + align_size];
	int mod = ((uint64)new_array) % align_size;
	int remainder = align_size - mod;
	uint64 result = (uint64)new_array + (uint64)remainder;
	return (void*)(result);
}