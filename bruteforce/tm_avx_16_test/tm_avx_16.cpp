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
	//return;
    for (int i = 0; i < 8; i++)
    {
		__m128i cur_val_lo = _mm_loadu_si128((__m128i *)(working_code + i*32));
		cur_val_lo = _mm_slli_epi16(cur_val_lo,1);

		__m128i cur_val_hi = _mm_loadu_si128((__m128i *)(working_code + i*32 + 16));
		cur_val_hi = _mm_slli_epi16(cur_val_hi,1);

		__m256i cur_val = _mm256_set_m128i(cur_val_hi, cur_val_lo);

		__m256i rng_val = _mm256_load_si256((__m256i *)(alg0_values + (rng_seed * 128 * 2) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}

void alg1(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed)
{
    for (int i = 0; i < 8; i++)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m256i rng_val = _mm256_load_si256((__m256i *)(regular_rng_values + (rng_seed * 128 * 2) + (i * 32)));

		__m128i sum_lo = _mm_add_epi16(_mm256_castsi256_si128(cur_val), _mm256_castsi256_si128(rng_val));

		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		rng_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(rng_val), _mm256_castsi256_pd(rng_val), 1));

		__m128i sum_hi = _mm_add_epi16(_mm256_castsi256_si128(cur_val), _mm256_castsi256_si128(rng_val));

		cur_val = _mm256_set_m128i(sum_hi, sum_lo);

		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}

void alg2(uint8 * working_code, const uint8 * alg2_values, const uint16 rng_seed)
{
	__m256i carry = _mm256_load_si256((__m256i *)(alg2_values + (rng_seed * 32)));
    for (int i = 7; i >= 0; i--)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		__m128i cur_val_hi = _mm256_castsi256_si128(cur_val);

		// Maybe 128-bit mask it, then reinterpret-cast it to 256 and permute it?
		__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(_mm256_set_epi16(0x01,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))));

		// Fetch extra carry and apply it to carry
		__m128i temp_lo = _mm_srli_si128(cur_val_lo,2);
		temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
		__m128i temp_hi = _mm_srli_si128(cur_val_hi,2);
		__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0))));
		carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));


		temp_lo = _mm_srli_epi16(cur_val_lo,1);
		temp_hi = _mm_srli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F))));

		temp_lo = _mm_slli_epi16(cur_val_lo,1);
		temp_hi = _mm_slli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0))));

		__m256i part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80))));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(part3)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(carry)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);

		carry = next_carry;
    }
}

void alg3(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed)
{
	const uint8 * rng_start = regular_rng_values + (rng_seed * 128 * 2);
    for (int i = 0; i < 8; i++)
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
    for (int i = 7; i >= 0; i--)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		__m128i cur_val_hi = _mm256_castsi256_si128(cur_val);

		// Maybe 128-bit mask it, then reinterpret-cast it to 256 and permute it?
		__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(_mm256_set_epi16(0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))));

		// Fetch extra carry and apply it to carry
		__m128i temp_lo = _mm_srli_si128(cur_val_lo,2);
		temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
		__m128i temp_hi = _mm_srli_si128(cur_val_hi,2);
		__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0))));
		carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));


		temp_lo = _mm_srli_epi16(cur_val_lo,1);
		temp_hi = _mm_srli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0))));

		temp_lo = _mm_slli_epi16(cur_val_lo,1);
		temp_hi = _mm_slli_epi16(cur_val_hi,1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE))));

		__m256i part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1))));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(part3)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val),_mm256_castsi256_pd(carry)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);

		carry = next_carry;
    }
}


void alg6(uint8 * working_code, const uint8 * alg6_values, const uint16 rng_seed)
{
	//return;
    for (int i = 0; i < 8; i++)
    {
		__m128i cur_val_lo = _mm_loadu_si128((__m128i *)(working_code + i*32));
		cur_val_lo = _mm_srli_epi16(cur_val_lo,1);

		__m128i cur_val_hi = _mm_loadu_si128((__m128i *)(working_code + i*32 + 16));
		cur_val_hi = _mm_srli_epi16(cur_val_hi,1);

		__m256i cur_val = _mm256_set_m128i(cur_val_hi, cur_val_lo);

		__m256i rng_val = _mm256_load_si256((__m256i *)(alg6_values + (rng_seed * 128 * 2) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));

		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}


void alg7(uint8 * working_code)
{
	//return;
    for (int i = 0; i < 8; i++)
    {
		__m256i cur_val = _mm256_load_si256((__m256i *)(working_code + i*32));
		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));
		_mm256_store_si256 ((__m256i *)(working_code + i*32), cur_val);
    }
}
