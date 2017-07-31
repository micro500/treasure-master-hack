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
	__m128i mask_FE = _mm_set1_epi16(0xFEFE);
    for (int i = 0; i < 8; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(alg0_values + ((*rng_seed) * 128) + (i * 16)));
		cur_val = _mm_slli_epi16(cur_val,1);
		cur_val = _mm_and_si128 (cur_val, mask_FE);
		cur_val = _mm_or_si128(cur_val, rng_val);
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = ((working_code[i] << 1) | alg0_values[(*rng_seed * 128) / 4 + i]) & 0x00FF00FF00FF00FFull;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg1(uint8 * working_code, uint8 * regular_rng_values_lo, uint8 * regular_rng_values_hi, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m128i mask_lo = _mm_set1_epi16(0x00FF);
	__m128i mask_hi = _mm_set1_epi16(0xFF00);

    for (int i = 0; i < 8; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val_lo = _mm_loadu_si128((__m128i *)(regular_rng_values_lo + ((*rng_seed) * 128) + (i * 16)));
		__m128i rng_val_hi = _mm_loadu_si128((__m128i *)(regular_rng_values_hi + ((*rng_seed) * 128) + (i * 16)));
		cur_val = _mm_or_si128(_mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val, mask_lo),rng_val_lo),mask_lo), _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val, mask_hi),rng_val_hi),mask_hi));
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg2(uint8 * working_code, uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m128i carry = _mm_loadu_si128((__m128i *)(alg2_values + ((*rng_seed) * 16)));
    for (int i = 7; i >= 0; i--)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val,_mm_set_epi16(0,0,0,0,0,0,0,1)),15);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val,_mm_set_epi8(0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0)),1));

		//_mm_and_si128(_mm_slli_si128(cur_val,15),_mm_set_epi16(0,0x80,0,0x80,0,0x80,0,0x80));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val,1), 
						_mm_set_epi8(0,0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val,1), 
						_mm_set_epi8(0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0))),
				_mm_and_si128(
					_mm_srli_si128(cur_val,1),
					_mm_set_epi8(0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80))),
			carry);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

		carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg3(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
    for (int i = 0; i < 8; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(regular_rng_values + ((*rng_seed) * 128) + (i * 16)));
		cur_val = _mm_xor_si128 (cur_val, rng_val);
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

        //working_code[i] = working_code[i] ^ regular_rng_values[(*rng_seed * 128) / 4 + i];
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg5(uint8* working_code, uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m128i carry = _mm_loadu_si128((__m128i *)(alg5_values + ((*rng_seed) * 16)));
    for (int i = 7; i >= 0; i--)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val,_mm_set_epi16(0,0,0,0,0,0,0,0x80)),15);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val,_mm_set_epi8(0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0)),1));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val,1), 
						_mm_set_epi8(0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f,0,0x7f,0)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val,1), 
						_mm_set_epi8(0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE))),
				_mm_and_si128(
					_mm_srli_si128(cur_val,1),
					_mm_set_epi8(0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1))),
			carry);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);

		carry = next_carry;

    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg6(uint8 * working_code, uint8 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m128i mask_7F = _mm_set1_epi16(0x7F7F);
    for (int i = 0; i < 8; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(alg6_values + ((*rng_seed) * 128) + (i * 16)));
		cur_val = _mm_srli_epi16(cur_val,1);
		cur_val = _mm_and_si128(cur_val, mask_7F);
		cur_val = _mm_or_si128(cur_val, rng_val);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg7(uint8 * working_code)
{
	//return;
	__m128i mask = _mm_set1_epi16(0xFFFF);
    for (int i = 0; i < 8; i++)
    {
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		
		cur_val = _mm_xor_si128 (cur_val, mask);
		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
    }
}
