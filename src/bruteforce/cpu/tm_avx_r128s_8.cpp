#include <stdio.h>
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
#include "tm_avx_r128s_8.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx_r128s_8::tm_avx_r128s_8(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_avx_r128s_8::initialize()
{
	if (!initialized)
	{
		shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 128, false);
		shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 128, false);

		shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 128, false);
		shuffle_mem(other_world_data, other_world_data_shuffled, 128, false);

		rng->generate_expansion_values_128_8_shuffled();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();
		rng->generate_regular_rng_values_128_8_shuffled();

		rng->generate_alg0_values_128_8_shuffled();
		rng->generate_alg2_values_128_8();
		rng->generate_alg4_values_128_8_shuffled();
		rng->generate_alg5_values_128_8();
		rng->generate_alg6_values_128_8_shuffled();

		initialized = true;
	}
	obj_name = "tm_avx_r128s_8";
}

__forceinline void tm_avx_r128s_8::_load_from_mem(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	working_code0 = _mm_load_si128((__m128i*)(working_code_data));
	working_code1 = _mm_load_si128((__m128i*)(working_code_data + 16));
	working_code2 = _mm_load_si128((__m128i*)(working_code_data + 32));
	working_code3 = _mm_load_si128((__m128i*)(working_code_data + 48));
	working_code4 = _mm_load_si128((__m128i*)(working_code_data + 64));
	working_code5 = _mm_load_si128((__m128i*)(working_code_data + 80));
	working_code6 = _mm_load_si128((__m128i*)(working_code_data + 96));
	working_code7 = _mm_load_si128((__m128i*)(working_code_data + 112));
}

__forceinline void tm_avx_r128s_8::_store_to_mem(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	_mm_store_si128((__m128i*)(working_code_data), working_code0);
	_mm_store_si128((__m128i*)(working_code_data + 16), working_code1);
	_mm_store_si128((__m128i*)(working_code_data + 32), working_code2);
	_mm_store_si128((__m128i*)(working_code_data + 48), working_code3);
	_mm_store_si128((__m128i*)(working_code_data + 64), working_code4);
	_mm_store_si128((__m128i*)(working_code_data + 80), working_code5);
	_mm_store_si128((__m128i*)(working_code_data + 96), working_code6);
	_mm_store_si128((__m128i*)(working_code_data + 112), working_code7);
}

void tm_avx_r128s_8::expand(uint32 key, uint32 data)
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;

	_expand_code(key, data, working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	_store_to_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);
}

__forceinline void tm_avx_r128s_8::_expand_code(uint32 key, uint32 data, __m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	uint64 x = ((uint64)key << 32) | data;

	__m128i a = _mm_cvtsi64_si128(x);
	__m128i lo_mask = _mm_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m128i hi_mask = _mm_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);

	__m128i lo = _mm_shuffle_epi8(a, lo_mask);
	__m128i hi = _mm_shuffle_epi8(a, hi_mask);

	working_code0 = lo;
	working_code1 = hi;
	working_code2 = lo;
	working_code3 = hi;
	working_code4 = lo;
	working_code5 = hi; 
	working_code6 = lo;
	working_code7 = hi;

	uint8* rng_start = rng->expansion_values_128_8_shuffled;
	uint16 rng_seed = (key >> 16) & 0xFFFF;

	add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, rng_start);
}

void tm_avx_r128s_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[(i / 64) * 64 + (i % 2) * 32 + ((i / 2) % 32)] = new_data[i];
	}
}

void tm_avx_r128s_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8*)working_code_data)[shuffle_8(i, 128)];
	}
}

__forceinline void tm_avx_r128s_8::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;
	_load_from_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	__m128i mask_FF = _mm_set1_epi16(0xFFFF);
	__m128i mask_FE = _mm_set1_epi16(0xFEFE);
	__m128i mask_7F = _mm_set1_epi16(0x7F7F);
	__m128i mask_80 = _mm_set1_epi16(0x8080);
	__m128i mask_01 = _mm_set1_epi16(0x0101);

	__m128i mask_top_01 = _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0);

	__m128i mask_top_80 = _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0);
	if (algorithm_id == 0)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_0(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_FE);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8* rng_start = rng->regular_rng_values_128_8_shuffled;

			if (algorithm_id == 4)
			{
				rng_start = rng->alg4_values_128_8_shuffled;
			}

			add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, rng_start);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_2(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
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
	else if (algorithm_id == 5)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_5(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_6(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_7F);
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

	_store_to_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);
}

__forceinline void tm_avx_r128s_8::alg_0(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed,__m128i& mask_FE)
{
	uint8* rng_start = rng->alg0_values_128_8_shuffled + ((*rng_seed) * 128);

	alg_0_sub(working_code0, rng_start, mask_FE);
	alg_0_sub(working_code1, rng_start + 16, mask_FE);
	alg_0_sub(working_code2, rng_start + 32, mask_FE);
	alg_0_sub(working_code3, rng_start + 48, mask_FE);
	alg_0_sub(working_code4, rng_start + 64, mask_FE);
	alg_0_sub(working_code5, rng_start + 80, mask_FE);
	alg_0_sub(working_code6, rng_start + 96, mask_FE);
	alg_0_sub(working_code7, rng_start + 112, mask_FE);
}

__forceinline void tm_avx_r128s_8::alg_0_sub(__m128i& working_code, uint8* rng_start, __m128i& mask_FE)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_slli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_FE);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
{
	// bitwise right shift
	__m128i temp1 = _mm_srli_epi16(working_a, 1);
	// Mask off top bits
	__m128i cur_val1_most = _mm_and_si128(temp1, mask_7F);

	// Mask off the top bits
	__m128i cur_val2_masked = _mm_and_si128(working_b, mask_80);

	// bytewise right shift
	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	// mask off only the relevant low bit
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, mask_01);
	// add the carry to the top
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	// bitwise right shift
	__m128i temp2 = _mm_slli_epi16(working_b, 1);
	// mask off lowest bit
	__m128i cur_val2_most = _mm_and_si128(temp2, mask_FE);

	// Save the next carry
	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), mask_top_01);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);

	carry = next_carry;
}

__forceinline void tm_avx_r128s_8::alg_2(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg2_values_128_8 + (*rng_seed * 16)));

	alg_2_sub(working_code6, working_code7, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code4, working_code5, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code2, working_code3, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code0, working_code1, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_avx_r128s_8::alg_3(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16 * rng_seed)
{
	uint8 * rng_start = rng->regular_rng_values_128_8_shuffled + ((*rng_seed) * 128);

	xor_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_start);
}

__forceinline void tm_avx_r128s_8::xor_alg(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint8* values)
{
	working_code0 = _mm_xor_si128(working_code0, _mm_load_si128((__m128i*)values));
	working_code1 = _mm_xor_si128(working_code1, _mm_load_si128((__m128i*)(values + 16)));
	working_code2 = _mm_xor_si128(working_code2, _mm_load_si128((__m128i*)(values + 32)));
	working_code3 = _mm_xor_si128(working_code3, _mm_load_si128((__m128i*)(values + 48)));
	working_code4 = _mm_xor_si128(working_code4, _mm_load_si128((__m128i*)(values + 64)));
	working_code5 = _mm_xor_si128(working_code5, _mm_load_si128((__m128i*)(values + 80)));
	working_code6 = _mm_xor_si128(working_code6, _mm_load_si128((__m128i*)(values + 96)));
	working_code7 = _mm_xor_si128(working_code7, _mm_load_si128((__m128i*)(values + 112)));
}

__forceinline void tm_avx_r128s_8::alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
{
	// bitwise left shift
	__m128i temp1 = _mm_slli_epi16(working_a, 1);
	// Mask off low bits
	__m128i cur_val1_most = _mm_and_si128(temp1, mask_FE);

	// Mask off the low bits
	__m128i cur_val2_masked = _mm_and_si128(working_b, mask_01);

	// bytewise right shift
	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	// mask off only the relevant high bit
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, mask_80);
	// add the carry to the top
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	// bitwise right shift
	__m128i temp2 = _mm_srli_epi16(working_b, 1);
	// mask off high bit
	__m128i cur_val2_most = _mm_and_si128(temp2, mask_7F);

	// Save the next carry
	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), mask_top_80);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);

	carry = next_carry;
}

__forceinline void tm_avx_r128s_8::alg_5(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg5_values_128_8 + (*rng_seed * 16)));

	alg_5_sub(working_code6, working_code7, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code4, working_code5, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code2, working_code3, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code0, working_code1, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_avx_r128s_8::alg_6(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_7F)
{
	uint8* rng_start = rng->alg0_values_128_8_shuffled + ((*rng_seed) * 128);

	alg_6_sub(working_code0, rng_start, mask_7F);
	alg_6_sub(working_code1, rng_start + 16, mask_7F);
	alg_6_sub(working_code2, rng_start + 32, mask_7F);
	alg_6_sub(working_code3, rng_start + 48, mask_7F);
	alg_6_sub(working_code4, rng_start + 64, mask_7F);
	alg_6_sub(working_code5, rng_start + 80, mask_7F);
	alg_6_sub(working_code6, rng_start + 96, mask_7F);
	alg_6_sub(working_code7, rng_start + 112, mask_7F);
}

__forceinline void tm_avx_r128s_8::alg_6_sub(__m128i& working_code, uint8* rng_start, __m128i& mask_7F)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_srli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_7F);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_7(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& mask_FF)
{
	working_code0 = _mm_xor_si128(working_code0, mask_FF);
	working_code1 = _mm_xor_si128(working_code1, mask_FF);
	working_code2 = _mm_xor_si128(working_code2, mask_FF);
	working_code3 = _mm_xor_si128(working_code3, mask_FF);
	working_code4 = _mm_xor_si128(working_code4, mask_FF);
	working_code5 = _mm_xor_si128(working_code5, mask_FF);
	working_code6 = _mm_xor_si128(working_code6, mask_FF);
	working_code7 = _mm_xor_si128(working_code7, mask_FF);
}

__forceinline void tm_avx_r128s_8::add_alg(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, uint8* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);
	working_code0 = _mm_add_epi8(working_code0, _mm_load_si128((__m128i*)rng_start));
	working_code1 = _mm_add_epi8(working_code1, _mm_load_si128((__m128i*)(rng_start + 16)));
	working_code2 = _mm_add_epi8(working_code2, _mm_load_si128((__m128i*)(rng_start + 32)));
	working_code3 = _mm_add_epi8(working_code3, _mm_load_si128((__m128i*)(rng_start + 48)));
	working_code4 = _mm_add_epi8(working_code4, _mm_load_si128((__m128i*)(rng_start + 64)));
	working_code5 = _mm_add_epi8(working_code5, _mm_load_si128((__m128i*)(rng_start + 80)));
	working_code6 = _mm_add_epi8(working_code6, _mm_load_si128((__m128i*)(rng_start + 96)));
	working_code7 = _mm_add_epi8(working_code7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline bool tm_avx_r128s_8::check_carnival_world_checksum(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	return _calculate_carnival_world_checksum(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7) == _fetch_carnival_world_checksum_value(working_code0, working_code1);
}

__forceinline bool tm_avx_r128s_8::check_other_world_checksum(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	return _calculate_other_world_checksum(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7) == _fetch_other_world_checksum_value(working_code0, working_code1);
}

__forceinline void tm_avx_r128s_8::mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask)
{
	__m128i temp_masked = _mm_and_si128(working_code, sum_mask);

	__m128i temp1_lo_lo = _mm_and_si128(temp_masked, lo_mask);
	__m128i temp1_lo_hi = _mm_srli_epi16(temp_masked, 8);

	sum = _mm_add_epi16(sum, temp1_lo_lo);
	sum = _mm_add_epi16(sum, temp1_lo_hi);
}

void tm_avx_r128s_8::run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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

void tm_avx_r128s_8::run_all_maps(const key_schedule& schedule_entries)
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;
	_load_from_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	__m128i mask_FF = _mm_set1_epi16(0xFFFF);
	__m128i mask_FE = _mm_set1_epi16(0xFEFE);
	__m128i mask_7F = _mm_set1_epi16(0x7F7F);
	__m128i mask_80 = _mm_set1_epi16(0x8080);
	__m128i mask_01 = _mm_set1_epi16(0x0101);

	__m128i mask_top_01 = _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0);

	__m128i mask_top_80 = _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0);

	_run_all_maps(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, schedule_entries, mask_FF, mask_FE, mask_7F, mask_80, mask_01, mask_top_01, mask_top_80);

	_store_to_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);
}

__forceinline void tm_avx_r128s_8::_run_all_maps(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, const key_schedule& schedule_entries, __m128i& mask_FF, __m128i& mask_FE, __m128i& mask_7F, __m128i& mask_80, __m128i& mask_01, __m128i& mask_top_01, __m128i& mask_top_80)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries.entries.begin(); it != schedule_entries.entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

		uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
		uint16 nibble_selector = schedule_entry.nibble_selector;

		// Next, the working code is processed with the same steps 16 times:
		for (int i = 0; i < 16; i++)
		{
			_mm_store_si128((__m128i*)(working_code_data), working_code0);
			_mm_store_si128((__m128i*)(working_code_data + 16), working_code1);

			// Get the highest bit of the nibble selector to use as a flag
			unsigned char nibble = (nibble_selector >> 15) & 0x01;
			// Shift the nibble selector up one bit
			nibble_selector = nibble_selector << 1;

			// If the flag is a 1, get the high nibble of the current byte
			// Otherwise use the low nibble
			unsigned char current_byte = (uint8)((uint8*)working_code_data)[shuffle_8(i, 128)];

			if (nibble == 1)
			{
				current_byte = current_byte >> 4;
			}

			// Mask off only 3 bits
			unsigned char algorithm_id = (current_byte >> 1) & 0x07;

			if (algorithm_id == 0)
			{
				alg_0(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_FE);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 1 || algorithm_id == 4)
			{
				uint8* rng_start = rng->regular_rng_values_128_8_shuffled;

				if (algorithm_id == 4)
				{
					rng_start = rng->alg4_values_128_8_shuffled;
				}

				add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, rng_start);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 2)
			{
				alg_2(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 3)
			{
				alg_3(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 5)
			{
				alg_5(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 6)
			{
				alg_6(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, &rng_seed, mask_7F);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 7)
			{
				alg_7(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, mask_FF);
			}
		}
	}
}

void tm_avx_r128s_8::decrypt_carnival_world()
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;
	_load_from_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	_decrypt_carnival_world(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	_store_to_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);
}

void tm_avx_r128s_8::decrypt_other_world()
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;
	_load_from_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	_decrypt_other_world(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	_store_to_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);
}

void tm_avx_r128s_8::_decrypt_carnival_world(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	xor_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, carnival_world_data_shuffled);
}

void tm_avx_r128s_8::_decrypt_other_world(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	xor_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, other_world_data_shuffled);
}

__forceinline uint16 tm_avx_r128s_8::masked_checksum(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint8* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m128i sum_mask = _mm_load_si128((__m128i*)(mask));
	mid_sum(sum, working_code0, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 16));
	mid_sum(sum, working_code1, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 32));
	mid_sum(sum, working_code2, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 48));
	mid_sum(sum, working_code3, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 64));
	mid_sum(sum, working_code4, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 80));
	mid_sum(sum, working_code5, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 96));
	mid_sum(sum, working_code6, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 12));
	mid_sum(sum, working_code7, sum_mask, lo_mask);

	uint16 code_sum = _mm_extract_epi16(sum, 0) +
		_mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) +
		_mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) +
		_mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) +
		_mm_extract_epi16(sum, 7);

	return code_sum;
}


uint16 tm_avx_r128s_8::calculate_carnival_world_checksum()
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;
	_load_from_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	return _calculate_carnival_world_checksum(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);
}

uint16 tm_avx_r128s_8::calculate_other_world_checksum()
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;
	_load_from_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

	return _calculate_other_world_checksum(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);
}

uint16 tm_avx_r128s_8::_calculate_carnival_world_checksum(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	return masked_checksum(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, carnival_world_checksum_mask_shuffled);
}

uint16 tm_avx_r128s_8::_calculate_other_world_checksum(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7)
{
	return masked_checksum(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, other_world_checksum_mask_shuffled);
}

__forceinline uint16 tm_avx_r128s_8::fetch_checksum_value(__m128i& working_code0, __m128i& working_code1, uint8 code_length)
{
	_mm_store_si128((__m128i*)(working_code_data), working_code0);
	_mm_store_si128((__m128i*)(working_code_data + 32), working_code1);

	unsigned char checksum_low = (uint8)((uint8*)working_code_data)[shuffle_8((127 - code_length), 128)];
	unsigned char checksum_hi = (uint8)((uint8*)working_code_data)[shuffle_8((127 - (code_length + 1)), 128)];
	uint16 checksum = (checksum_hi << 8) | checksum_low;

	return checksum;
}

uint16 tm_avx_r128s_8::fetch_carnival_world_checksum_value()
{
	__m128i working_code0 = _mm_load_si128((__m128i*)(working_code_data));
	__m128i working_code1 = _mm_load_si128((__m128i*)(working_code_data + 16));

	return _fetch_carnival_world_checksum_value(working_code0, working_code1);
}

uint16 tm_avx_r128s_8::fetch_other_world_checksum_value()
{
	__m128i working_code0 = _mm_load_si128((__m128i*)(working_code_data));
	__m128i working_code1 = _mm_load_si128((__m128i*)(working_code_data + 16));

	return _fetch_other_world_checksum_value(working_code0, working_code1);
}

__forceinline uint16 tm_avx_r128s_8::_fetch_carnival_world_checksum_value(__m128i& working_code0, __m128i& working_code1)
{
	return fetch_checksum_value(working_code0, working_code1, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16 tm_avx_r128s_8::_fetch_other_world_checksum_value(__m128i& working_code0, __m128i& working_code1)
{
	return fetch_checksum_value(working_code0, working_code1, OTHER_WORLD_CODE_LENGTH - 2);
}

void tm_avx_r128s_8::run_bruteforce_data(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;

	__m128i mask_FF = _mm_set1_epi16(0xFFFF);
	__m128i mask_FE = _mm_set1_epi16(0xFEFE);
	__m128i mask_7F = _mm_set1_epi16(0x7F7F);
	__m128i mask_80 = _mm_set1_epi16(0x8080);
	__m128i mask_01 = _mm_set1_epi16(0x0101);

	__m128i mask_top_01 = _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0);
	__m128i mask_top_80 = _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0);

	uint32 output_pos = 0;
	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - output_pos) < 5)
		{
			*result_size = result_max_size;
			return;
		}
		uint32 data = start_data + i;

		_expand_code(key, data, working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7);

		_run_all_maps(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, schedule_entries, mask_FF, mask_FE, mask_7F, mask_80, mask_01, mask_top_01, mask_top_80);

		__m128i working_code0_xor = working_code0;
		__m128i working_code1_xor = working_code1;
		__m128i working_code2_xor = working_code2;
		__m128i working_code3_xor = working_code3;
		__m128i working_code4_xor = working_code4;
		__m128i working_code5_xor = working_code5;
		__m128i working_code6_xor = working_code6;
		__m128i working_code7_xor = working_code7;

		_decrypt_carnival_world(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor, working_code4_xor, working_code5_xor, working_code6_xor, working_code7_xor);

		if (check_carnival_world_checksum(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor, working_code4_xor, working_code5_xor, working_code6_xor, working_code7_xor))
		{
			_store_to_mem(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor, working_code4_xor, working_code5_xor, working_code6_xor, working_code7_xor);

			*((uint32*)(&result_data[output_pos])) = i;

			uint8 unshuffled_data[128];
			unshuffle_mem(working_code_data, unshuffled_data, 128, false);

 			result_data[output_pos + 4] = check_machine_code(unshuffled_data, CARNIVAL_WORLD);
			output_pos += 5;
		}
		else
		{
			working_code0_xor = working_code0;
			working_code1_xor = working_code1;
			working_code2_xor = working_code2;
			working_code3_xor = working_code3;
			working_code4_xor = working_code4;
			working_code5_xor = working_code5;
			working_code6_xor = working_code6;
			working_code7_xor = working_code7;

			_decrypt_other_world(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor, working_code4_xor, working_code5_xor, working_code6_xor, working_code7_xor);

			if (check_other_world_checksum(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor, working_code4_xor, working_code5_xor, working_code6_xor, working_code7_xor))
			{
				_store_to_mem(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor, working_code4_xor, working_code5_xor, working_code6_xor, working_code7_xor);
				*((uint32*)(&result_data[output_pos])) = i;

				uint8 unshuffled_data[128];
				unshuffle_mem(working_code_data, unshuffled_data, 128, false);

				result_data[output_pos + 4] = check_machine_code(unshuffled_data, OTHER_WORLD);
				output_pos += 5;
			}
		}
		
		report_progress((float)(i + 1) / amount_to_run);
	}

	*result_size = output_pos;
}

bool tm_avx_r128s_8::initialized = false;
uint8 tm_avx_r128s_8::carnival_world_checksum_mask_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8 tm_avx_r128s_8::carnival_world_data_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8 tm_avx_r128s_8::other_world_checksum_mask_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8 tm_avx_r128s_8::other_world_data_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };