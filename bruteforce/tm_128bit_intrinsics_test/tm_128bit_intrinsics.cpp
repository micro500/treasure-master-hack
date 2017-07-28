#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A

#include "data_sizes.h"

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
