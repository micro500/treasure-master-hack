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
#include "tm_avx_in_cpu.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx_in_cpu::tm_avx_in_cpu(RNG *rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_avx_in_cpu::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_16();

		rng->generate_alg0_values_16();
		rng->generate_alg2_values_256_16();
		rng->generate_alg5_values_256_16();
		rng->generate_alg6_values_16();

		initialized = true;
	}
	obj_name = "tm_avx_in_cpu";
}

void tm_avx_in_cpu::expand(uint32 key, uint32 data)
{
	uint16* x = (uint16*)working_code_data;
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


void tm_avx_in_cpu::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint16*)working_code_data)[i] = new_data[i];
	}
}

void tm_avx_in_cpu::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint16*)working_code_data)[i];
	}
}


void tm_avx_in_cpu::run_alg(int algorithm_id, uint16 * rng_seed, int iterations)
{
	// get working code from memory
	__m256i working_code0 = _mm256_load_si256((__m256i *)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i *)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i *)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i *)(working_code_data + 96));
	__m256i working_code4 = _mm256_load_si256((__m256i *)(working_code_data + 128));
	__m256i working_code5 = _mm256_load_si256((__m256i *)(working_code_data + 160));
	__m256i working_code6 = _mm256_load_si256((__m256i *)(working_code_data + 192));
	__m256i working_code7 = _mm256_load_si256((__m256i *)(working_code_data + 224));

	// init
	__m256i mask_FF = _mm256_set1_epi16(0x00FF);
	__m256i mask_01 = _mm256_set1_epi16(0x0001);
	__m256i mask_top_01 = _mm256_set_epi16(0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg2 = _mm256_set_epi16(0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
	__m256i mask_007F = _mm256_set_epi16(0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F);
	__m256i mask_FE00 = _mm256_set_epi16(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0);
	__m256i mask_0080 = _mm256_set_epi16(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80);

	__m256i mask_top_80 = _mm256_set_epi16(0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg5 = _mm256_set_epi16(0, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0);
	__m256i mask_7F00 = _mm256_set_epi16(0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0);
	__m256i mask_00FE = _mm256_set_epi16(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE);
	__m256i mask_0001 = _mm256_set_epi16(0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01);

	if (algorithm_id == 0)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_0(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_FF);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 1)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_1(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_FF);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_2(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_top_01, mask_alg2, mask_007F, mask_FE00, mask_0080);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 3)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_3(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 4)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_4(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_FF);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 5)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_5(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_top_80, mask_alg5, mask_7F00, mask_00FE, mask_0001);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_6(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_FF);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 7)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_7(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, mask_FF);
		}
	}

	// store back to memory
	_mm256_store_si256((__m256i *)(working_code_data), working_code0);
	_mm256_store_si256((__m256i *)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i *)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i *)(working_code_data + 96), working_code3);
	_mm256_store_si256((__m256i *)(working_code_data + 128), working_code4);
	_mm256_store_si256((__m256i *)(working_code_data + 160), working_code5);
	_mm256_store_si256((__m256i *)(working_code_data + 192), working_code6);
	_mm256_store_si256((__m256i *)(working_code_data + 224), working_code7);
}

__forceinline void tm_avx_in_cpu::alg_0(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF)
{
	uint8 * rng_start = ((uint8*)rng->alg0_values_16) + ((*rng_seed) * 128 * 2);

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code0);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code0, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code0 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code1);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code1, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code1 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code2 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code3);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code3, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code3 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code4);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code4, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code4 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(rng_val)));
	working_code4 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code5);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code5, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code5 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(rng_val)));
	working_code5 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code6);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code6, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code6 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(rng_val)));
	working_code6 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code7);
	cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code7, 1);
	cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);
	working_code7 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(rng_val)));
	working_code7 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));

}

__forceinline void tm_avx_in_cpu::alg_1(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF)
{
	uint8 * rng_start = ((uint8*)rng->regular_rng_values_16) + ((*rng_seed) * 128 * 2);

	__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
	__m128i sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code0), _mm256_castsi256_si128(rng_val));
	__m128i sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code0, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code0 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
	sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code1), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code1, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code1 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
	sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code2), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code2, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code2 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
	sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code3), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code3, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code3 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
	sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code4), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code4, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code4 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code4 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
	sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code5), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code5, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code5 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code5 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
	sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code6), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code6, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code6 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code6 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
	sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code7), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code7, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code7 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code7 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));
}

__forceinline void tm_avx_in_cpu::alg_2(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_top_01, __m256i& mask_alg2, __m256i& mask_007F, __m256i& mask_FE00, __m256i& mask_0080)
{
	__m256i carry = _mm256_load_si256((__m256i *)(rng->alg2_values_256_16 + ((*rng_seed) * 32)));

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code7);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code7, 1);

	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));

	__m128i temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 2);
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

	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(part3)));
	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(carry)));

	carry = next_carry;



	cur_val_lo = _mm256_castsi256_si128(working_code6);
	cur_val_hi = _mm256_extractf128_si256(working_code6, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(part3)));
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code5);
	cur_val_hi = _mm256_extractf128_si256(working_code5, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(part3)));
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code4);
	cur_val_hi = _mm256_extractf128_si256(working_code4, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(part3)));
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code3);
	cur_val_hi = _mm256_extractf128_si256(working_code3, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(part3)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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

__forceinline void tm_avx_in_cpu::alg_3(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed)
{
	uint8 * rng_start = ((uint8*)rng->regular_rng_values_16) + ((*rng_seed) * 128 * 2);

	__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
	working_code4 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
	working_code5 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
	working_code6 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
	working_code7 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_in_cpu::alg_4(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF)
{
	uint8 * rng_start = ((uint8*)rng->regular_rng_values_16) + ((*rng_seed) * 128 * 2);

	__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
	__m128i sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code0), _mm256_castsi256_si128(rng_val));
	__m128i sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code0, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code0 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
	sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code1), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code1, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code1 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
	sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code2), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code2, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code2 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
	sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code3), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code3, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code3 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
	sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code4), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code4, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code4 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code4 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
	sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code5), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code5, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code5 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code5 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
	sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code6), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code6, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code6 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code6 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

	rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
	sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code7), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code7, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code7 = _mm256_set_m128i(sum_hi, sum_lo);
	working_code7 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));
}

__forceinline void tm_avx_in_cpu::alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_alg5, __m256i& mask_7F00, __m256i& mask_00FE, __m256i& mask_0001)
{
	__m256i carry = _mm256_load_si256((__m256i *)(rng->alg5_values_256_16 + ((*rng_seed) * 32)));

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code7);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code7, 1);

	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));

	__m128i temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 2);
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

	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(part3)));
	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(carry)));

	carry = next_carry;



	cur_val_lo = _mm256_castsi256_si128(working_code6);
	cur_val_hi = _mm256_extractf128_si256(working_code6, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(part3)));
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code5);
	cur_val_hi = _mm256_extractf128_si256(working_code5, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(part3)));
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code4);
	cur_val_hi = _mm256_extractf128_si256(working_code4, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(part3)));
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code3);
	cur_val_hi = _mm256_extractf128_si256(working_code3, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(part3)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(carry)));
	carry = next_carry;

	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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
	temp_lo = _mm_srli_si128(cur_val_lo, 2);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
	temp_hi = _mm_srli_si128(cur_val_hi, 2);
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

__forceinline void tm_avx_in_cpu::alg_6(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF)
{
	uint8 * rng_start = ((uint8*)rng->alg6_values_16) + ((*rng_seed) * 128 * 2);

	__m128i cur_val_lo = _mm256_castsi256_si128(working_code0);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	__m128i cur_val_hi = _mm256_extractf128_si256(working_code0, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code0 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code1);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code1, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code1 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code2);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code2, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code2 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code3);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code3, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code3 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code4);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code4, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code4 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
	working_code4 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(rng_val)));
	working_code4 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code5);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code5, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code5 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
	working_code5 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(rng_val)));
	working_code5 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code6);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code6, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code6 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
	working_code6 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(rng_val)));
	working_code6 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

	cur_val_lo = _mm256_castsi256_si128(working_code7);
	cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);
	cur_val_hi = _mm256_extractf128_si256(working_code7, 1);
	cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);
	working_code7 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
	rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
	working_code7 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(rng_val)));
	working_code7 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));
}

__forceinline void tm_avx_in_cpu::alg_7(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, __m256i& mask_FF)
{
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));
	working_code4 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));
	working_code5 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));
	working_code6 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));
	working_code7 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));
}

void tm_avx_in_cpu::run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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



void tm_avx_in_cpu::run_all_maps(const key_schedule& schedule_entries)
{
	// get working code from memory
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));
	__m256i working_code4 = _mm256_load_si256((__m256i*)(working_code_data + 128));
	__m256i working_code5 = _mm256_load_si256((__m256i*)(working_code_data + 160));
	__m256i working_code6 = _mm256_load_si256((__m256i*)(working_code_data + 192));
	__m256i working_code7 = _mm256_load_si256((__m256i*)(working_code_data + 224));

	// init
	__m256i mask_FF = _mm256_set1_epi16(0x00FF);
	__m256i mask_01 = _mm256_set1_epi16(0x0001);
	__m256i mask_top_01 = _mm256_set_epi16(0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg2 = _mm256_set_epi16(0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
	__m256i mask_007F = _mm256_set_epi16(0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F);
	__m256i mask_FE00 = _mm256_set_epi16(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0);
	__m256i mask_0080 = _mm256_set_epi16(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80);

	__m256i mask_top_80 = _mm256_set_epi16(0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_alg5 = _mm256_set_epi16(0, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0);
	__m256i mask_7F00 = _mm256_set_epi16(0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0);
	__m256i mask_00FE = _mm256_set_epi16(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE);
	__m256i mask_0001 = _mm256_set_epi16(0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01);

	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries.entries.begin(); it != schedule_entries.entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

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
			unsigned char current_byte = (uint8)((uint16*)working_code_data)[i];

			if (nibble == 1)
			{
				current_byte = current_byte >> 4;
			}

			// Mask off only 3 bits
			unsigned char algorithm_id = (current_byte >> 1) & 0x07;

			if (algorithm_id == 0)
			{
					alg_0(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_FF);
					rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 1)
			{
					alg_1(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_FF);
					rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 2)
			{
					alg_2(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_top_01, mask_alg2, mask_007F, mask_FE00, mask_0080);
					rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 3)
			{
					alg_3(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed);
					rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 4)
			{
					alg_4(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_FF);
					rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 5)
			{
					alg_5(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_top_80, mask_alg5, mask_7F00, mask_00FE, mask_0001);
					rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 6)
			{
					alg_6(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_FF);
					rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 7)
			{
					alg_7(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, mask_FF);
			}

		}
	}

	// store back to memory
	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
	_mm256_store_si256((__m256i*)(working_code_data + 128), working_code4);
	_mm256_store_si256((__m256i*)(working_code_data + 160), working_code5);
	_mm256_store_si256((__m256i*)(working_code_data + 192), working_code6);
	_mm256_store_si256((__m256i*)(working_code_data + 224), working_code7);
}

bool tm_avx_in_cpu::initialized = false;