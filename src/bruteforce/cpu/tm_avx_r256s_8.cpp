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
#include "tm_avx_r256s_8.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx_r256s_8::tm_avx_r256s_8(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_avx_r256s_8::initialize()
{
	if (!initialized)
	{
		shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 256, false);
		shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 256, false);

		shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 256, false);
		shuffle_mem(other_world_data, other_world_data_shuffled, 256, false);

		rng->generate_expansion_values_256_8_shuffled();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();
		rng->generate_regular_rng_values_256_8_shuffled();

		rng->generate_alg0_values_256_8_shuffled();
		rng->generate_alg2_values_256_8();
		rng->generate_alg4_values_256_8_shuffled();
		rng->generate_alg5_values_256_8();
		rng->generate_alg6_values_256_8_shuffled();

		initialized = true;
	}
	obj_name = "tm_avx_r256s_8";
}

void tm_avx_r256s_8::expand(uint32 key, uint32 data)
{
	__m256i working_code0;
	__m256i working_code1;
	__m256i working_code2;
	__m256i working_code3;

	_expand_code(key, data, working_code0, working_code1, working_code2, working_code3);

	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
}

__forceinline void tm_avx_r256s_8::_expand_code(uint32 key, uint32 data, __m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3)
{
	__m256i lo = _mm256_set_epi8((data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF,
		(data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF,
		(data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF,
		(data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF,
		(data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF,
		(data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF,
		(data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF,
		(data >> 8) & 0xFF, (data >> 24) & 0xFF, (key >> 8) & 0xFF, (key >> 24) & 0xFF);

	__m256i hi = _mm256_set_epi8(data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF,
		data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF,
		data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF,
		data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF,
		data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF,
		data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF,
		data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF,
		data & 0xFF, (data >> 16) & 0xFF, key & 0xFF, (key >> 16) & 0xFF);

	working_code0 = lo;
	working_code1 = hi;
	working_code2 = lo;
	working_code3 = hi;

	uint8* rng_start = rng->expansion_values_256_8_shuffled;
	uint16 rng_seed = (key >> 16) & 0xFFFF;

	add_alg(working_code0, working_code1, working_code2, working_code3, &rng_seed, rng_start);
}

void tm_avx_r256s_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[(i / 64) * 64 + (i % 2) * 32 + ((i / 2) % 32)] = new_data[i];
	}
}

void tm_avx_r256s_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8*)working_code_data)[(i / 64) * 64 + (i % 2) * 32 + ((i / 2) % 32)];
	}
}

__forceinline void tm_avx_r256s_8::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	__m256i mask_FF = _mm256_set1_epi16(0xFFFF);
	__m256i mask_FE = _mm256_set1_epi16(0xFEFE);
	__m256i mask_7F = _mm256_set1_epi16(0x7F7F);
	__m256i mask_80 = _mm256_set1_epi16(0x8080);
	__m256i mask_01 = _mm256_set1_epi16(0x0101);

	__m256i mask_top_01 = _mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	__m256i mask_top_80 = _mm256_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
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
			uint8* rng_start = rng->regular_rng_values_256_8_shuffled;

			if (algorithm_id == 4)
			{
				rng_start = rng->alg4_values_256_8_shuffled;
			}

			add_alg(working_code0, working_code1, working_code2, working_code3, rng_seed, rng_start);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_2(working_code0, working_code1, working_code2, working_code3, rng_seed, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
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
			alg_5(working_code0, working_code1, working_code2, working_code3, rng_seed, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
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

__forceinline void tm_avx_r256s_8::alg_0(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed,__m256i& mask_FE)
{
	uint8* rng_start = rng->alg0_values_256_8_shuffled + ((*rng_seed) * 128);

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

__forceinline void tm_avx_r256s_8::alg_2_sub(__m256i& working_a, __m256i& working_b, __m256i& carry, __m256i& mask_top_01, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m128i cur_val1_lo = _mm256_castsi256_si128(working_a);
	__m128i cur_val1_hi = _mm256_extractf128_si256(working_a, 1);
	__m128i cur_val2_lo = _mm256_castsi256_si128(working_b);
	__m128i cur_val2_hi = _mm256_extractf128_si256(working_b, 1);

	// bitwise right shift
	__m128i temp1_lo = _mm_srli_epi16(cur_val1_lo, 1);
	__m128i temp1_hi = _mm_srli_epi16(cur_val1_hi, 1);
	// Mask off top bits
	__m256i cur_val1_most = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(temp1_hi, temp1_lo)), _mm256_castsi256_pd(mask_7F)));

	// Mask off the top bits
	__m256i cur_val2_masked = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_b), _mm256_castsi256_pd(mask_80)));

	// bytewise right shift
	temp1_lo = _mm_srli_si128(cur_val1_lo, 1);
	// carry the lowest byte from the high half into the lowest half
	temp1_lo = _mm_or_si128(temp1_lo, _mm_slli_si128(cur_val1_hi, 15));
	temp1_hi = _mm_srli_si128(cur_val1_hi, 1);
	__m256i cur_val1_srl = _mm256_set_m128i(temp1_hi, temp1_lo);
	// mask off only the relevant low bit
	__m256i cur_val1_bit = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val1_srl), _mm256_castsi256_pd(mask_01)));
	// add the carry to the top
	cur_val1_bit = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_bit), _mm256_castsi256_pd(carry)));

	// bitwise right shift
	__m128i temp2_lo = _mm_slli_epi16(cur_val2_lo, 1);
	__m128i temp2_hi = _mm_slli_epi16(cur_val2_hi, 1);
	// mask off lowest bit
	__m256i cur_val2_most = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(temp2_hi, temp2_lo)), _mm256_castsi256_pd(mask_FE)));

	// Save the next carry
	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val1_lo, 15), cur_val1_lo)), _mm256_castsi256_pd(mask_top_01)));

	working_a = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_most), _mm256_castsi256_pd(cur_val2_masked)));
	working_b = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val2_most), _mm256_castsi256_pd(cur_val1_bit)));

	carry = next_carry;
}

__forceinline void tm_avx_r256s_8::alg_2(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_01, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg2_values_256_8 + ((*rng_seed) * 32)));

	alg_2_sub(working_code2, working_code3, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code0, working_code1, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_avx_r256s_8::alg_3(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16 * rng_seed)
{
	uint8 * rng_start = rng->regular_rng_values_256_8_shuffled + ((*rng_seed) * 128);

	xor_alg(working_code0, working_code1, working_code2, working_code3, rng_start);
}

__forceinline void tm_avx_r256s_8::xor_alg(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint8* values)
{
	__m256i rng_val = _mm256_load_si256((__m256i*)(values));
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(values + 32));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(values + 64));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(values + 96));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_r256s_8::alg_5_sub(__m256i& working_a, __m256i& working_b, __m256i& carry, __m256i& mask_top_80, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m128i cur_val1_lo = _mm256_castsi256_si128(working_a);
	__m128i cur_val1_hi = _mm256_extractf128_si256(working_a, 1);
	__m128i cur_val2_lo = _mm256_castsi256_si128(working_b);
	__m128i cur_val2_hi = _mm256_extractf128_si256(working_b, 1);

	// bitwise left shift
	__m128i temp1_lo = _mm_slli_epi16(cur_val1_lo, 1);
	__m128i temp1_hi = _mm_slli_epi16(cur_val1_hi, 1);
	// Mask off low bits
	__m256i cur_val1_most = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(temp1_hi, temp1_lo)), _mm256_castsi256_pd(mask_FE)));

	// Mask off the low bits
	__m256i cur_val2_masked = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_b), _mm256_castsi256_pd(mask_01)));

	// bytewise right shift
	temp1_lo = _mm_srli_si128(cur_val1_lo, 1);
	// carry the lowest byte from the high half into the lowest half
	temp1_lo = _mm_or_si128(temp1_lo, _mm_slli_si128(cur_val1_hi, 15));
	temp1_hi = _mm_srli_si128(cur_val1_hi, 1);
	__m256i cur_val1_srl = _mm256_set_m128i(temp1_hi, temp1_lo);
	// mask off only the relevant high bit
	__m256i cur_val1_bit = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val1_srl), _mm256_castsi256_pd(mask_80)));
	// add the carry to the top
	cur_val1_bit = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_bit), _mm256_castsi256_pd(carry)));

	// bitwise right shift
	__m128i temp2_lo = _mm_srli_epi16(cur_val2_lo, 1);
	__m128i temp2_hi = _mm_srli_epi16(cur_val2_hi, 1);
	// mask off high bit
	__m256i cur_val2_most = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(temp2_hi, temp2_lo)), _mm256_castsi256_pd(mask_7F)));

	// Save the next carry
	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val1_lo, 15), cur_val1_lo)), _mm256_castsi256_pd(mask_top_80)));

	working_a = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_most), _mm256_castsi256_pd(cur_val2_masked)));
	working_b = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val2_most), _mm256_castsi256_pd(cur_val1_bit)));

	carry = next_carry;
}

__forceinline void tm_avx_r256s_8::alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg5_values_256_8 + ((*rng_seed) * 32)));

	alg_5_sub(working_code2, working_code3, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code0, working_code1, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_avx_r256s_8::alg_6(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed,__m256i& mask_7F)
{
	uint8* rng_start = rng->alg6_values_256_8_shuffled + ((*rng_seed) * 128);

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

__forceinline void tm_avx_r256s_8::alg_7(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& mask_FF)
{
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));
}

__forceinline void tm_avx_r256s_8::add_alg(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, uint8* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);

	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	__m128i sum_lo = _mm_add_epi8(_mm256_castsi256_si128(working_code0), _mm256_castsi256_si128(rng_val));
	__m128i sum_hi = _mm_add_epi8(_mm256_extractf128_si256(working_code0, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code0 = _mm256_set_m128i(sum_hi, sum_lo);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	sum_lo = _mm_add_epi8(_mm256_castsi256_si128(working_code1), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi8(_mm256_extractf128_si256(working_code1, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code1 = _mm256_set_m128i(sum_hi, sum_lo);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	sum_lo = _mm_add_epi8(_mm256_castsi256_si128(working_code2), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi8(_mm256_extractf128_si256(working_code2, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code2 = _mm256_set_m128i(sum_hi, sum_lo);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	sum_lo = _mm_add_epi8(_mm256_castsi256_si128(working_code3), _mm256_castsi256_si128(rng_val));
	sum_hi = _mm_add_epi8(_mm256_extractf128_si256(working_code3, 1), _mm256_extractf128_si256(rng_val, 1));
	working_code3 = _mm256_set_m128i(sum_hi, sum_lo);
}

__forceinline bool tm_avx_r256s_8::check_carnival_world_checksum(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3)
{
	return _calculate_carnival_world_checksum(working_code0, working_code1, working_code2, working_code3) == _fetch_carnival_world_checksum_value(working_code0, working_code1);
}

__forceinline bool tm_avx_r256s_8::check_other_world_checksum(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3)
{
	return _calculate_other_world_checksum(working_code0, working_code1, working_code2, working_code3) == _fetch_other_world_checksum_value(working_code0, working_code1);
}

__forceinline void tm_avx_r256s_8::mid_sum(__m128i& sum, __m256i& working_code, __m256i& sum_mask, __m128i& lo_mask)
{
	__m256i temp_masked = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code), _mm256_castsi256_pd(sum_mask)));

	__m128i temp1_lo = _mm256_castsi256_si128(temp_masked);
	__m128i temp1_hi = _mm256_extractf128_si256(temp_masked, 1);

	__m128i temp1_lo_lo = _mm_and_si128(temp1_lo, lo_mask);
	__m128i temp1_lo_hi = _mm_srli_epi16(temp1_lo, 8);
	__m128i temp1_hi_lo = _mm_and_si128(temp1_hi, lo_mask);
	__m128i temp1_hi_hi = _mm_srli_epi16(temp1_hi, 8);

	sum = _mm_add_epi16(sum, temp1_lo_lo);
	sum = _mm_add_epi16(sum, temp1_lo_hi);
	sum = _mm_add_epi16(sum, temp1_hi_lo);
	sum = _mm_add_epi16(sum, temp1_hi_hi);
}

void tm_avx_r256s_8::run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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

void tm_avx_r256s_8::run_all_maps(const key_schedule& schedule_entries)
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	__m256i mask_FF = _mm256_set1_epi16(0xFFFF);
	__m256i mask_FE = _mm256_set1_epi16(0xFEFE);
	__m256i mask_7F = _mm256_set1_epi16(0x7F7F);
	__m256i mask_80 = _mm256_set1_epi16(0x8080);
	__m256i mask_01 = _mm256_set1_epi16(0x0101);

	__m256i mask_top_01 = _mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_top_80 = _mm256_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	_run_all_maps(working_code0, working_code1, working_code2, working_code3, schedule_entries, mask_FF, mask_FE, mask_7F, mask_80, mask_01, mask_top_01, mask_top_80);

	// store back to memory
	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
}

void tm_avx_r256s_8::_run_all_maps(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, const key_schedule& schedule_entries, __m256i& mask_FF, __m256i& mask_FE, __m256i& mask_7F, __m256i& mask_80, __m256i& mask_01, __m256i& mask_top_01, __m256i& mask_top_80)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries.entries.begin(); it != schedule_entries.entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

		uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
		uint16 nibble_selector = schedule_entry.nibble_selector;

		// Next, the working code is processed with the same steps 16 times:
		for (int i = 0; i < 16; i++)
		{
			_mm256_store_si256((__m256i*)(working_code_data), working_code0);
			_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);

			// Get the highest bit of the nibble selector to use as a flag
			unsigned char nibble = (nibble_selector >> 15) & 0x01;
			// Shift the nibble selector up one bit
			nibble_selector = nibble_selector << 1;

			// If the flag is a 1, get the high nibble of the current byte
			// Otherwise use the low nibble
			unsigned char current_byte = (uint8)((uint8*)working_code_data)[shuffle_8(i, 256)];

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
				uint8* rng_start = rng->regular_rng_values_256_8_shuffled;

				if (algorithm_id == 4)
				{
					rng_start = rng->alg4_values_256_8_shuffled;
				}

				add_alg(working_code0, working_code1, working_code2, working_code3, &rng_seed, rng_start);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 2)
			{
				alg_2(working_code0, working_code1, working_code2, working_code3, &rng_seed, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 3)
			{
				alg_3(working_code0, working_code1, working_code2, working_code3, &rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 5)
			{
				alg_5(working_code0, working_code1, working_code2, working_code3, &rng_seed, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
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
}

void tm_avx_r256s_8::decrypt_carnival_world()
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	_decrypt_carnival_world(working_code0, working_code1, working_code2, working_code3);

	// store back to memory
	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
}

void tm_avx_r256s_8::decrypt_other_world()
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	_decrypt_other_world(working_code0, working_code1, working_code2, working_code3);

	// store back to memory
	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
}

void tm_avx_r256s_8::_decrypt_carnival_world(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3)
{
	xor_alg(working_code0, working_code1, working_code2, working_code3, carnival_world_data_shuffled);
}

void tm_avx_r256s_8::_decrypt_other_world(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3)
{
	xor_alg(working_code0, working_code1, working_code2, working_code3, other_world_data_shuffled);
}

__forceinline uint16 tm_avx_r256s_8::masked_checksum(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint8* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m256i sum_mask = _mm256_load_si256((__m256i*)(mask));
	mid_sum(sum, working_code0, sum_mask, lo_mask);

	sum_mask = _mm256_load_si256((__m256i*)(mask + 32));
	mid_sum(sum, working_code1, sum_mask, lo_mask);

	sum_mask = _mm256_load_si256((__m256i*)(mask + 64));
	mid_sum(sum, working_code2, sum_mask, lo_mask);

	sum_mask = _mm256_load_si256((__m256i*)(mask + 96));
	mid_sum(sum, working_code3, sum_mask, lo_mask);

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


uint16 tm_avx_r256s_8::calculate_carnival_world_checksum()
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	return _calculate_carnival_world_checksum(working_code0, working_code1, working_code2, working_code3);
}

uint16 tm_avx_r256s_8::calculate_other_world_checksum()
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i*)(working_code_data + 96));

	return _calculate_other_world_checksum(working_code0, working_code1, working_code2, working_code3);
}

uint16 tm_avx_r256s_8::_calculate_carnival_world_checksum(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3)
{
	return masked_checksum(working_code0, working_code1, working_code2, working_code3, carnival_world_checksum_mask_shuffled);
}

uint16 tm_avx_r256s_8::_calculate_other_world_checksum(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3)
{
	return masked_checksum(working_code0, working_code1, working_code2, working_code3, other_world_checksum_mask_shuffled);
}

__forceinline uint16 tm_avx_r256s_8::fetch_checksum_value(__m256i& working_code0, __m256i& working_code1, uint8 code_length)
{
	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);

	unsigned char checksum_low = (uint8)((uint8*)working_code_data)[shuffle_8((127 - code_length), 256)];
	unsigned char checksum_hi = (uint8)((uint8*)working_code_data)[shuffle_8((127 - (code_length + 1)), 256)];
	uint16 checksum = (checksum_hi << 8) | checksum_low;

	return checksum;
}

uint16 tm_avx_r256s_8::fetch_carnival_world_checksum_value()
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));

	return _fetch_carnival_world_checksum_value(working_code0, working_code1);
}

uint16 tm_avx_r256s_8::fetch_other_world_checksum_value()
{
	__m256i working_code0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i working_code1 = _mm256_load_si256((__m256i*)(working_code_data + 32));

	return _fetch_other_world_checksum_value(working_code0, working_code1);
}

__forceinline uint16 tm_avx_r256s_8::_fetch_carnival_world_checksum_value(__m256i& working_code0, __m256i& working_code1)
{
	return fetch_checksum_value(working_code0, working_code1, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16 tm_avx_r256s_8::_fetch_other_world_checksum_value(__m256i& working_code0, __m256i& working_code1)
{
	return fetch_checksum_value(working_code0, working_code1, OTHER_WORLD_CODE_LENGTH - 2);
}

void tm_avx_r256s_8::run_bruteforce_data(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	__m256i working_code0;
	__m256i working_code1;
	__m256i working_code2;
	__m256i working_code3;

	__m256i mask_FF = _mm256_set1_epi16(0xFFFF);
	__m256i mask_FE = _mm256_set1_epi16(0xFEFE);
	__m256i mask_7F = _mm256_set1_epi16(0x7F7F);
	__m256i mask_80 = _mm256_set1_epi16(0x8080);
	__m256i mask_01 = _mm256_set1_epi16(0x0101);

	__m256i mask_top_01 = _mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i mask_top_80 = _mm256_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	uint32 output_pos = 0;
	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - output_pos) < 5)
		{
			*result_size = result_max_size;
			return;
		}
		uint32 data = start_data + i;

		_expand_code(key, data, working_code0, working_code1, working_code2, working_code3);

		_run_all_maps(working_code0, working_code1, working_code2, working_code3, schedule_entries, mask_FF, mask_FE, mask_7F, mask_80, mask_01, mask_top_01, mask_top_80);

		__m256i working_code0_xor = working_code0;
		__m256i working_code1_xor = working_code1;
		__m256i working_code2_xor = working_code2;
		__m256i working_code3_xor = working_code3;

		_decrypt_carnival_world(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor);

		if (check_carnival_world_checksum(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor))
		{
			_mm256_store_si256((__m256i*)(working_code_data), working_code0_xor);
			_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1_xor);
			_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2_xor);
			_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3_xor);
			*((uint32*)(&result_data[output_pos])) = i;

			uint8 unshuffled_data[128];
			unshuffle_mem(working_code_data, unshuffled_data, 256, false);

 			result_data[output_pos + 4] = check_machine_code(unshuffled_data, CARNIVAL_WORLD);
			output_pos += 5;
		}
		else
		{
			working_code0_xor = working_code0;
			working_code1_xor = working_code1;
			working_code2_xor = working_code2;
			working_code3_xor = working_code3;

			_decrypt_other_world(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor);

			if (check_other_world_checksum(working_code0_xor, working_code1_xor, working_code2_xor, working_code3_xor))
			{
				_mm256_store_si256((__m256i*)(working_code_data), working_code0_xor);
				_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1_xor);
				_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2_xor);
				_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3_xor);
				*((uint32*)(&result_data[output_pos])) = i;

				uint8 unshuffled_data[128];
				unshuffle_mem(working_code_data, unshuffled_data, 256, false);

				result_data[output_pos + 4] = check_machine_code(unshuffled_data, OTHER_WORLD);
				output_pos += 5;
			}
		}
		
		report_progress((float)(i + 1) / amount_to_run);
	}

	*result_size = output_pos;
}

bool tm_avx_r256s_8::initialized = false;
uint8 tm_avx_r256s_8::carnival_world_checksum_mask_shuffled[128] =
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

uint8 tm_avx_r256s_8::carnival_world_data_shuffled[128] =
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

uint8 tm_avx_r256s_8::other_world_checksum_mask_shuffled[128] =
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

uint8 tm_avx_r256s_8::other_world_data_shuffled[128] =
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