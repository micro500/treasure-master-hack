#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2

#include "data_sizes.h"

void alg0(uint8 * working_code, const uint8 * alg0_values, const uint16 rng_seed)
{
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(alg0_values + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_slli_epi16(cur_val,1);
		cur_val = _mm_or_si128 (cur_val, rng_val);

		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128  (cur_val, mask);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
}

void alg1(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed)
{
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(regular_rng_values + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_add_epi16 (cur_val, rng_val);
		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128  (cur_val, mask);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
}

void alg2(uint8 * working_code, const uint8 * alg2_values, const uint16 rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i *)(alg2_values + (rng_seed * 16)));
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
    }
}

void alg3(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed)
{
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(regular_rng_values + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_xor_si128 (cur_val, rng_val);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
}

void alg5(uint8* working_code, const uint8 * alg5_values, const uint16 rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i *)(alg5_values + (rng_seed * 16)));
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
}


void alg6(uint8 * working_code, const uint8 * alg6_values, const uint16 rng_seed)
{
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(alg6_values + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_srli_epi16(cur_val,1);
		cur_val = _mm_or_si128 (cur_val, rng_val);

		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128  (cur_val, mask);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
}


void alg7(uint8 * working_code)
{
	__m128i mask = _mm_set1_epi16(0x00FF);
    for (int i = 0; i < 16; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		cur_val = _mm_xor_si128 (cur_val, mask);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
}
