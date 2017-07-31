#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A
#include <immintrin.h> //AVX
//#include <zmmintrin.h> //AVX512

#include "data_sizes.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

void alg0(uint8 * working_code, uint8 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m256i mask_FE = _mm256_set1_epi16(0xFEFE);

    for (int i = 0; i < 4; i++)
    {
		__m128i cur_val_lo = _mm_loadu_si128((__m128i *)(working_code + i*32));
		cur_val_lo = _mm_slli_epi16(cur_val_lo,1);

		__m128i cur_val_hi = _mm_loadu_si128((__m128i *)(working_code + i*32 + 16));
		cur_val_hi = _mm_slli_epi16(cur_val_hi,1);

		__m256i cur_val = _mm256_set_m128i(cur_val_hi, cur_val_lo);
		cur_val = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask_FE)));

		__m256i rng_val = _mm256_load_si256((__m256i *)(alg0_values + ((*rng_seed) * 128) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));
		
		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg1(uint8 * working_code, uint8 * regular_rng_values_lo, uint8 * regular_rng_values_hi, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m128i mask_lo = _mm_set1_epi16(0x00FF);
	__m128i mask_hi = _mm_set1_epi16(0xFF00);

    for (int i = 0; i < 4; i++)
    {
		__m128i cur_val_lo = _mm_loadu_si128((__m128i *)(working_code + i*32));
		__m128i cur_val_hi = _mm_loadu_si128((__m128i *)(working_code + i*32 + 16));
		__m128i rng_val_lo_lo = _mm_loadu_si128((__m128i *)(regular_rng_values_lo + ((*rng_seed) * 128) + (i * 32)));
		__m128i rng_val_lo_hi = _mm_loadu_si128((__m128i *)(regular_rng_values_lo + ((*rng_seed) * 128) + (i * 32) + 16));
		__m128i rng_val_hi_lo = _mm_loadu_si128((__m128i *)(regular_rng_values_hi + ((*rng_seed) * 128) + (i * 32)));
		__m128i rng_val_hi_hi = _mm_loadu_si128((__m128i *)(regular_rng_values_hi + ((*rng_seed) * 128) + (i * 32) + 16));

		__m128i sum_lo_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_lo), rng_val_lo_lo), mask_lo);
		__m128i sum_lo_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_hi), rng_val_hi_lo), mask_hi);

		__m128i sum_hi_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_lo), rng_val_lo_hi), mask_lo);
		__m128i sum_hi_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_hi), rng_val_hi_hi), mask_hi);

		__m256i sum_lo = _mm256_set_m128i(sum_hi_lo, sum_lo_lo);
		__m256i sum_hi = _mm256_set_m128i(sum_hi_hi, sum_lo_hi);

		__m256i cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(sum_lo), _mm256_castsi256_pd(sum_hi)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg2(uint8 * working_code, uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	//return;
	// Load the 256 carry

	// Fetch current working value
	// Set the next carry by getting lowest of cur_val, shifting to top of 256
		// do 128bit shift to top of 128 lane, then 256 permute

	// Get extra carry:
		// do shift to cur val lo, store to temp
		// permute, do shift to hi, store to temp2
		// combine into 256
		// 256 mask it
		// or with carry

	// final combine
		// shift cur val lo, store to temp 1
		// shift cur val hi, store to temp 2
		// combine into temp large
		// mask it

		// shift cur val lo, store to temp 1
		// shift cur val hi, store to temp 2
		// combine into temp large
		// mask it

		// Shift across 128 lanes...
		// shift cur_val hi down 1 cell
		// shift cur_val hi up 7 cells, keep top 0xFF (probably don't need to mask it) (carry across lanes)
		// shift cur_val lo down 1 cell, or it with lane carry
		// combine into a thing
		// 256 mask it


		// 256-or the 4 parts together


	__m256i carry = _mm256_load_si256((__m256i *)(alg2_values + ((*rng_seed) * 32)));
    for (int i = 3; i >= 0; i--)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		__m128i cur_val_hi = _mm256_castsi256_si128(cur_val);

		__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,15), cur_val_lo)), _mm256_castsi256_pd(_mm256_set_epi16(0x0100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))));

		// Fetch extra carry and apply it to carry
		__m128i temp_lo = _mm_srli_si128(cur_val_lo,1);
		temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,15));
		__m128i temp_hi = _mm_srli_si128(cur_val_hi,1);
		__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100,0x0100))));
		carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));


		temp_lo = _mm_srli_epi16(cur_val_lo,1);
		temp_hi = _mm_srli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F,0x007F))));

		temp_lo = _mm_slli_epi16(cur_val_lo,1);
		temp_hi = _mm_slli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00,0xFE00))));

		__m256i part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080))));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(part3)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(carry)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);

		carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg3(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	uint8 * rng_start = regular_rng_values + ((*rng_seed) * 128);
	//return;
    for (int i = 0; i < 4; i++)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start + (i * 32)));
		cur_val = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));
		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
	*rng_seed = rng_forward[*rng_seed];
}

void alg5(uint8* working_code, uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m256i carry = _mm256_load_si256((__m256i *)(alg5_values + ((*rng_seed) * 32)));
    for (int i = 3; i >= 0; i--)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		__m128i cur_val_hi = _mm256_castsi256_si128(cur_val);

		// Maybe 128-bit mask it, then reinterpret-cast it to 256 and permute it?
		__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,15), cur_val_lo)), _mm256_castsi256_pd(_mm256_set_epi16(0x8000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))));

		// Fetch extra carry and apply it to carry
		__m128i temp_lo = _mm_srli_si128(cur_val_lo,1);
		temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,15));
		__m128i temp_hi = _mm_srli_si128(cur_val_hi,1);
		__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000))));
		carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));


		temp_lo = _mm_srli_epi16(cur_val_lo,1);
		temp_hi = _mm_srli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00,0x7F00))));

		temp_lo = _mm_slli_epi16(cur_val_lo,1);
		temp_hi = _mm_slli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE,0x00FE))));

		__m256i part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001))));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(part3)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(carry)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);

		carry = next_carry;
    }
	*rng_seed = rng_forward[*rng_seed];







	return;
	/*
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
	*/
}


void alg6(uint8 * working_code, uint8 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward)
{
	__m256i mask_7F = _mm256_set1_epi16(0x7F7F);
    for (int i = 0; i < 4; i++)
    {
		__m128i cur_val_lo = _mm_loadu_si128((__m128i *)(working_code + i*32));
		cur_val_lo = _mm_srli_epi16(cur_val_lo,1);

		__m128i cur_val_hi = _mm_loadu_si128((__m128i *)(working_code + i*32 + 16));
		cur_val_hi = _mm_srli_epi16(cur_val_hi,1);

		__m256i cur_val = _mm256_set_m128i(cur_val_hi, cur_val_lo);
		cur_val = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask_7F)));

		__m256i rng_val = _mm256_load_si256((__m256i *)(alg6_values + ((*rng_seed) * 128) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);

    }
	*rng_seed = rng_forward[*rng_seed];
}


void alg7(uint8 * working_code)
{
	__m256i mask = _mm256_set1_epi16(0xFFFF);
    for (int i = 0; i < 4; i++)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		cur_val = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));
		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}
