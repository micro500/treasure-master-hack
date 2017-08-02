#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
#include <immintrin.h> //AVX

#include "data_sizes.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

void alg0(uint8 * working_code, const uint8 * alg0_values, const uint16 rng_seed)
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

		__m256i rng_val = _mm256_load_si256((__m256i *)(alg0_values + (rng_seed * 128) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));
		
		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}

void alg1(uint8 * working_code, const uint8 * regular_rng_values_lo, const uint8 * regular_rng_values_hi, const uint16 rng_seed)
{
	__m128i mask_lo = _mm_set1_epi16(0x00FF);
	__m128i mask_hi = _mm_set1_epi16(0xFF00);

    for (int i = 0; i < 4; i++)
    {
		__m128i cur_val_lo = _mm_loadu_si128((__m128i *)(working_code + i*32));
		__m128i cur_val_hi = _mm_loadu_si128((__m128i *)(working_code + i*32 + 16));
		__m128i rng_val_lo_lo = _mm_loadu_si128((__m128i *)(regular_rng_values_lo + (rng_seed * 128) + (i * 32)));
		__m128i rng_val_lo_hi = _mm_loadu_si128((__m128i *)(regular_rng_values_lo + (rng_seed * 128) + (i * 32) + 16));
		__m128i rng_val_hi_lo = _mm_loadu_si128((__m128i *)(regular_rng_values_hi + (rng_seed * 128) + (i * 32)));
		__m128i rng_val_hi_hi = _mm_loadu_si128((__m128i *)(regular_rng_values_hi + (rng_seed * 128) + (i * 32) + 16));

		__m128i sum_lo_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_lo), rng_val_lo_lo), mask_lo);
		__m128i sum_lo_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_hi), rng_val_hi_lo), mask_hi);

		__m128i sum_hi_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_lo), rng_val_lo_hi), mask_lo);
		__m128i sum_hi_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_hi), rng_val_hi_hi), mask_hi);

		__m256i sum_lo = _mm256_set_m128i(sum_hi_lo, sum_lo_lo);
		__m256i sum_hi = _mm256_set_m128i(sum_hi_hi, sum_lo_hi);

		__m256i cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(sum_lo), _mm256_castsi256_pd(sum_hi)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}

void alg2(uint8 * working_code, const uint8 * alg2_values, const uint16 rng_seed)
{
	__m256i carry = _mm256_load_si256((__m256i *)(alg2_values + (rng_seed * 32)));
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
}

void alg3(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed)
{
	const uint8 * rng_start = regular_rng_values + (rng_seed * 128);
    for (int i = 0; i < 4; i++)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start + (i * 32)));
		cur_val = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}

void alg5(uint8* working_code, const uint8 * alg5_values, const uint16 rng_seed)
{
	__m256i carry = _mm256_load_si256((__m256i *)(alg5_values + (rng_seed * 32)));
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
}

void alg6(uint8 * working_code, const uint8 * alg6_values, const uint16 rng_seed)
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

		__m256i rng_val = _mm256_load_si256((__m256i *)(alg6_values + (rng_seed * 128) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
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
