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
#include "tm_avx_8_in_cpu.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx_8_in_cpu::tm_avx_8_in_cpu(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_avx_8_in_cpu::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();
		rng->generate_regular_rng_values_8_lo();
		rng->generate_regular_rng_values_8_hi();

		rng->generate_alg0_values_8();
		rng->generate_alg2_values_256_8();
		rng->generate_alg4_values_8_lo();
		rng->generate_alg4_values_8_hi();
		rng->generate_alg5_values_256_8();
		rng->generate_alg6_values_8();

		initialized = true;
	}
	obj_name = "tm_avx_8_in_cpu";
}

void tm_avx_8_in_cpu::expand(uint32 key, uint32 data)
{
	uint8* x = (uint8*)working_code_data;
	for (int i = 0; i < 128; i += 8)
	{
		x[i] = (key >> 24) & 0xFF;
		x[i + 1] = (key >> 16) & 0xFF;
		x[i + 2] = (key >> 8) & 0xFF;
		x[i + 3] = key & 0xFF;

		x[i + 4] = (data >> 24) & 0xFF;
		x[i + 5] = (data >> 16) & 0xFF;
		x[i + 6] = (data >> 8) & 0xFF;
		x[i + 7] = data & 0xFF;
	}

	uint16 rng_seed = (key >> 16) & 0xFFFF;
	for (int i = 0; i < 128; i++)
	{
		x[i] += rng->expansion_values_8[rng_seed * 128 + i];
		x[i] = x[i] & 0xFF;
	}
}


void tm_avx_8_in_cpu::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[i] = new_data[i];
	}
}

void tm_avx_8_in_cpu::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8*)working_code_data)[i];
	}
}

void tm_avx_8_in_cpu::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	__m256i mask_FF = _mm256_set1_epi16(0xFFFF);
	__m256i mask_FE = _mm256_set1_epi16(0xFEFE);
	__m256i mask_7F = _mm256_set1_epi16(0x7F7F);

	__m128i mask_lo = _mm_set_epi8(0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0);
	__m128i mask_hi = _mm_set_epi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);

	__m256i mask_01 = _mm256_set1_epi16(0x0001);
	__m256i mask_top_01 = _mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg2 = _mm256_set_epi16(0, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100);
	__m256i mask_007F = _mm256_set_epi16(0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F);
	__m256i mask_FE00 = _mm256_set_epi16(0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00);
	__m256i mask_0080 = _mm256_set_epi16(0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080);

	__m256i mask_top_80 = _mm256_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg5 = _mm256_set_epi16(0, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000);
	__m256i mask_7F00 = _mm256_set_epi16(0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00);
	__m256i mask_00FE = _mm256_set_epi16(0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE);
	__m256i mask_0001 = _mm256_set_epi16(0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001);
	if (algorithm_id == 0)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_0(working_code0, working_code1, working_code2, working_code3, rng_seed, mask_FE);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8* rng_lo_start = rng->regular_rng_values_8_lo;
			uint8* rng_hi_start = rng->regular_rng_values_8_hi;

			if (algorithm_id == 4)
			{
				rng_lo_start = rng->alg4_values_8_lo;
				rng_hi_start = rng->alg4_values_8_hi;
			}

			add_alg(working_code0, working_code1, working_code2, working_code3, rng_seed, rng_lo_start, rng_hi_start, mask_lo, mask_hi);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_2(working_code0, working_code1, working_code2, working_code3, rng_seed, mask_top_01, mask_alg2, mask_007F, mask_FE00, mask_0080);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 3)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_3(working_code0, working_code1, working_code2, working_code3, rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 5)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_5(working_code0, working_code1, working_code2, working_code3, rng_seed, mask_top_80, mask_alg5, mask_7F00, mask_00FE, mask_0001);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_6(working_code0, working_code1, working_code2, working_code3, rng_seed, mask_7F);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 7)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_7(working_code0, working_code1, working_code2, working_code3, mask_FF);
		}
	}

	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
}

__forceinline void tm_avx_8_in_cpu::alg_0(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed,__m256i& mask_FE)
{
	uint8* rng_start = rng->alg0_values_8 + ((*rng_seed) * 128);

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code0);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code0, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code0 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FE)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	cur_val_lo = _mm256_castsi256_si128(working_code1);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code1, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code1 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FE)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code2 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FE)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	cur_val_lo = _mm256_castsi256_si128(working_code3);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code3, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code3 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FE)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_8_in_cpu::alg_2(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_01, __m256i& mask_alg2, __m256i& mask_007F, __m256i& mask_FE00, __m256i& mask_0080)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg2_values_256_8 + ((*rng_seed) * 32)));

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code3);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code3, 1);

	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));

	__m128i temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 1);
	__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

	__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));

	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));

	__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));

	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(part3)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(carry)));

	carry = next_carry;

	
	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	temp_hi = _mm_srli_si128(cur_val_hi, 1);
	cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
	temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
	part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(part3)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code1);
	cur_val_hi = _mm256_extractf128_si256(working_code1, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	temp_hi = _mm_srli_si128(cur_val_hi, 1);
	cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
	temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
	part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(part3)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code0);
	cur_val_hi = _mm256_extractf128_si256(working_code0, 1);
	temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	temp_hi = _mm_srli_si128(cur_val_hi, 1);
	cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
	temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
	part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(part3)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(carry)));
	
}

__forceinline void tm_avx_8_in_cpu::alg_3(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16 * rng_seed) 
{
	uint8 * rng_start = rng->regular_rng_values_8 + ((*rng_seed) * 128);

	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_8_in_cpu::alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_alg5, __m256i& mask_7F00, __m256i& mask_00FE, __m256i& mask_0001)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg5_values_256_8 + ((*rng_seed) * 32)));

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code3);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code3, 1);

	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));

	__m128i temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 1);
	__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

	__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));

	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));

	__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));

	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(part3)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(carry)));

	carry = next_carry;



	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	temp_hi = _mm_srli_si128(cur_val_hi, 1);
	cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
	temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
	part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(part3)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code1);
	cur_val_hi = _mm256_extractf128_si256(working_code1, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	temp_hi = _mm_srli_si128(cur_val_hi, 1);
	cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
	temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
	part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(part3)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code0);
	cur_val_hi = _mm256_extractf128_si256(working_code0, 1);
	temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	temp_hi = _mm_srli_si128(cur_val_hi, 1);
	cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
	temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
	carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));
	temp_lo = _mm_srli_epi16(cur_val_lo, 1);
	temp_hi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
	temp_lo = _mm_slli_epi16(cur_val_lo, 1);
	temp_hi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(temp_hi, temp_lo);
	part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
	part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(part3)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(carry)));
}

__forceinline void tm_avx_8_in_cpu::alg_6(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed,__m256i& mask_7F)
{
	uint8* rng_start = rng->alg6_values_8 + ((*rng_seed) * 128);

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code0);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code0, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code0 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_7F)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	cur_val_lo = _mm256_castsi256_si128(working_code1);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code1, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code1 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_7F)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code2 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_7F)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	cur_val_lo = _mm256_castsi256_si128(working_code3);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code3, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code3 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_7F)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_8_in_cpu::alg_7(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& mask_FF)
{
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));
}

__forceinline void tm_avx_8_in_cpu::add_alg(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, uint8* rng_lo_start, uint8* rng_hi_start, __m128i& mask_lo, __m128i& mask_hi)
{
	rng_lo_start = rng_lo_start + ((*rng_seed) * 128);
	rng_hi_start = rng_hi_start + ((*rng_seed) * 128);

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code0);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code0, 1);
	__m128i rng_val_lo_lo = _mm_loadu_si128((__m128i*)(rng_lo_start));
	__m128i rng_val_lo_hi = _mm_loadu_si128((__m128i*)(rng_lo_start + 16));
	__m128i rng_val_hi_lo = _mm_loadu_si128((__m128i*)(rng_hi_start));
	__m128i rng_val_hi_hi = _mm_loadu_si128((__m128i*)(rng_hi_start + 16));

	__m128i sum_lo_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_lo), rng_val_lo_lo), mask_lo);
	__m128i sum_lo_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_hi), rng_val_hi_lo), mask_hi);

	__m128i sum_hi_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_lo), rng_val_lo_hi), mask_lo);
	__m128i sum_hi_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_hi), rng_val_hi_hi), mask_hi);

	__m256i sum_lo = _mm256_set_m128i(sum_hi_lo, sum_lo_lo);
	__m256i sum_hi = _mm256_set_m128i(sum_hi_hi, sum_lo_hi);

	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(sum_lo), _mm256_castsi256_pd(sum_hi)));


	cur_val_lo = _mm256_castsi256_si128(working_code1);
	cur_val_hi = _mm256_extractf128_si256(working_code1, 1);
	rng_val_lo_lo = _mm_loadu_si128((__m128i*)(rng_lo_start + 32));
	rng_val_lo_hi = _mm_loadu_si128((__m128i*)(rng_lo_start + 48));
	rng_val_hi_lo = _mm_loadu_si128((__m128i*)(rng_hi_start + 32));
	rng_val_hi_hi = _mm_loadu_si128((__m128i*)(rng_hi_start + 48));
	sum_lo_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_lo), rng_val_lo_lo), mask_lo);
	sum_lo_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_hi), rng_val_hi_lo), mask_hi);
	sum_hi_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_lo), rng_val_lo_hi), mask_lo);
	sum_hi_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_hi), rng_val_hi_hi), mask_hi);
	sum_lo = _mm256_set_m128i(sum_hi_lo, sum_lo_lo);
	sum_hi = _mm256_set_m128i(sum_hi_hi, sum_lo_hi);
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(sum_lo), _mm256_castsi256_pd(sum_hi)));

	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	rng_val_lo_lo = _mm_loadu_si128((__m128i*)(rng_lo_start + 64));
	rng_val_lo_hi = _mm_loadu_si128((__m128i*)(rng_lo_start + 80));
	rng_val_hi_lo = _mm_loadu_si128((__m128i*)(rng_hi_start + 64));
	rng_val_hi_hi = _mm_loadu_si128((__m128i*)(rng_hi_start + 80));
	sum_lo_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_lo), rng_val_lo_lo), mask_lo);
	sum_lo_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_hi), rng_val_hi_lo), mask_hi);
	sum_hi_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_lo), rng_val_lo_hi), mask_lo);
	sum_hi_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_hi), rng_val_hi_hi), mask_hi);
	sum_lo = _mm256_set_m128i(sum_hi_lo, sum_lo_lo);
	sum_hi = _mm256_set_m128i(sum_hi_hi, sum_lo_hi);
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(sum_lo), _mm256_castsi256_pd(sum_hi)));

	cur_val_lo = _mm256_castsi256_si128(working_code3);
	cur_val_hi = _mm256_extractf128_si256(working_code3, 1);
	rng_val_lo_lo = _mm_loadu_si128((__m128i*)(rng_lo_start + 96));
	rng_val_lo_hi = _mm_loadu_si128((__m128i*)(rng_lo_start + 112));
	rng_val_hi_lo = _mm_loadu_si128((__m128i*)(rng_hi_start + 96));
	rng_val_hi_hi = _mm_loadu_si128((__m128i*)(rng_hi_start + 112));
	sum_lo_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_lo), rng_val_lo_lo), mask_lo);
	sum_lo_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_lo, mask_hi), rng_val_hi_lo), mask_hi);
	sum_hi_lo = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_lo), rng_val_lo_hi), mask_lo);
	sum_hi_hi = _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val_hi, mask_hi), rng_val_hi_hi), mask_hi);
	sum_lo = _mm256_set_m128i(sum_hi_lo, sum_lo_lo);
	sum_hi = _mm256_set_m128i(sum_hi_hi, sum_lo_hi);
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(sum_lo), _mm256_castsi256_pd(sum_hi)));

}

void tm_avx_8_in_cpu::run_one_map(key_schedule_entry schedule_entry)
{
	uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
	uint16 nibble_selector = schedule_entry.nibble_selector;

	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		// Get the highest bit of the nibble selector to use as a flag
		unsigned char nibble = (nibble_selector >> 15) & 0x01;
		// Shift the nibble selector up one bit
		nibble_selector = nibble_selector << 1;

		// If the flag is a 1, get the high nibble of the current byte
		// Otherwise use the low nibble
		unsigned char current_byte = (uint8)working_code_data[i];

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char alg_id = (current_byte >> 1) & 0x07;

		run_alg(alg_id, &rng_seed, 1);
	}
}



void tm_avx_8_in_cpu::run_all_maps(key_schedule_entry* schedule_entries)
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	__m256i mask_FF = _mm256_set1_epi16(0xFFFF);
	__m256i mask_FE = _mm256_set1_epi16(0xFEFE);
	__m256i mask_7F = _mm256_set1_epi16(0x7F7F);

	__m128i mask_lo = _mm_set_epi8(0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0);
	__m128i mask_hi = _mm_set_epi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);

	__m256i mask_01 = _mm256_set1_epi16(0x0001);
	__m256i mask_top_01 = _mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg2 = _mm256_set_epi16(0, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100);
	__m256i mask_007F = _mm256_set_epi16(0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F);
	__m256i mask_FE00 = _mm256_set_epi16(0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00, 0xFE00);
	__m256i mask_0080 = _mm256_set_epi16(0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080);

	__m256i mask_top_80 = _mm256_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg5 = _mm256_set_epi16(0, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000);
	__m256i mask_7F00 = _mm256_set_epi16(0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00);
	__m256i mask_00FE = _mm256_set_epi16(0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE, 0x00FE);
	__m256i mask_0001 = _mm256_set_epi16(0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001);

	for (int schedule_counter = 0; schedule_counter < 27; schedule_counter++)
	{
		key_schedule_entry schedule_entry = schedule_entries[schedule_counter];

		uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
		uint16 nibble_selector = schedule_entry.nibble_selector;

		// Next, the working code is processed with the same steps 16 times:
		for (int i = 0; i < 16; i++)
		{
			_mm256_store_si256((__m256i*)(((uint8*)working_code_data)), working_code0);

			// Get the highest bit of the nibble selector to use as a flag
			unsigned char nibble = (nibble_selector >> 15) & 0x01;
			// Shift the nibble selector up one bit
			nibble_selector = nibble_selector << 1;

			// If the flag is a 1, get the high nibble of the current byte
			// Otherwise use the low nibble
			unsigned char current_byte = (uint8)((uint8*)working_code_data)[i];

			if (nibble == 1)
			{
				current_byte = current_byte >> 4;
			}

			// Mask off only 3 bits
			unsigned char algorithm_id = (current_byte >> 1) & 0x07;

			if (algorithm_id == 0)
			{
				alg_0(working_code0, working_code1, working_code2, working_code3, &rng_seed, mask_FE);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 1 || algorithm_id == 4)
			{
				uint8* rng_lo_start = rng->regular_rng_values_8_lo;
				uint8* rng_hi_start = rng->regular_rng_values_8_hi;

				if (algorithm_id == 4)
				{
					rng_lo_start = rng->alg4_values_8_lo;
					rng_hi_start = rng->alg4_values_8_hi;
				}

				add_alg(working_code0, working_code1, working_code2, working_code3, &rng_seed, rng_lo_start, rng_hi_start, mask_lo, mask_hi);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 2)
			{
				alg_2(working_code0, working_code1, working_code2, working_code3, &rng_seed, mask_top_01, mask_alg2, mask_007F, mask_FE00, mask_0080);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 3)
			{
				alg_3(working_code0, working_code1, working_code2, working_code3, &rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 5)
			{
				alg_5(working_code0, working_code1, working_code2, working_code3, &rng_seed, mask_top_80, mask_alg5, mask_7F00, mask_00FE, mask_0001);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 6)
			{
				alg_6(working_code0, working_code1, working_code2, working_code3, &rng_seed, mask_7F);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 7)
			{
				alg_7(working_code0, working_code1, working_code2, working_code3, mask_FF);
			}
		}
	}

	// store back to memory
	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
}

bool tm_avx_8_in_cpu::initialized = false;