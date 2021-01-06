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
#include "tm_avx2_r256s_8.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx2_r256s_8::tm_avx2_r256s_8(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_avx2_r256s_8::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

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
	obj_name = "tm_avx2_r256s_8";
}

int tm_avx2_r256s_8::shuffle(int addr)
{
	return (addr / 64) * 64 + (addr % 2) * 32 + ((addr / 2) % 32);
}

void tm_avx2_r256s_8::expand(uint32 key, uint32 data)
{
	uint8* x = (uint8*)working_code_data;
	for (int i = 0; i < 128; i += 8)
	{
		x[shuffle(i)] = (key >> 24) & 0xFF;
		x[shuffle(i + 1)] = (key >> 16) & 0xFF;
		x[shuffle(i + 2)] = (key >> 8) & 0xFF;
		x[shuffle(i + 3)] = key & 0xFF;

		x[shuffle(i + 4)] = (data >> 24) & 0xFF;
		x[shuffle(i + 5)] = (data >> 16) & 0xFF;
		x[shuffle(i + 6)] = (data >> 8) & 0xFF;
		x[shuffle(i + 7)] = data & 0xFF;
	}

	uint16 rng_seed = (key >> 16) & 0xFFFF;
	for (int i = 0; i < 128; i++)
	{
		x[shuffle(i)] += rng->expansion_values_8[rng_seed * 128 + i];
		x[shuffle(i)] = x[shuffle(i)] & 0xFF;
	}
}

void tm_avx2_r256s_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[(i / 64) * 64 + (i % 2) * 32 + ((i / 2) % 32)] = new_data[i];
	}

}

void tm_avx2_r256s_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8*)working_code_data)[(i / 64) * 64 + (i % 2) * 32 + ((i / 2) % 32)];
	}
}

__forceinline void tm_avx2_r256s_8::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
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

__forceinline void tm_avx2_r256s_8::alg_0(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_FE)
{
	uint8* rng_start = rng->alg0_values_256_8_shuffled + ((*rng_seed) * 128);

	working_code0 = _mm256_slli_epi16(working_code0, 1);
	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FE)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	working_code1 = _mm256_slli_epi16(working_code1, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FE)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	working_code2 = _mm256_slli_epi16(working_code2, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FE)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	working_code3 = _mm256_slli_epi16(working_code3, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FE)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx2_r256s_8::alg_2_sub(__m256i& working_a, __m256i& working_b, __m256i& carry, __m256i& mask_top_01, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	// Shift bytes right 1 bit
	__m256i cur_val1_most = _mm256_and_si256(_mm256_srli_epi16(working_a, 1), mask_7F);
	// Shift bytes left 1 bit
	__m256i cur_val2_most = _mm256_and_si256(_mm256_slli_epi16(working_b, 1), mask_FE);
	
	// Mask off the top bits
	__m256i cur_val2_masked = _mm256_and_si256(working_b, mask_80);

	// Shift right 1 byte
	__m256i cur_val1_bit = _mm256_and_si256(working_a, mask_01);

	__m256i mask = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(3, 0, 0, 3));
	__m256i cur_val1_srl = _mm256_alignr_epi8(mask, cur_val1_bit, 1);
	__m256i cur_val1_srl_w_carry = _mm256_or_si256(cur_val1_srl, carry);

	// Save the next carry
	__m256i lo_to_hi = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(0, 0, 3, 0));
	__m256i next_carry = _mm256_bslli_epi128(lo_to_hi, 15);

	working_a = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_most), _mm256_castsi256_pd(cur_val2_masked)));
	working_b = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val2_most), _mm256_castsi256_pd(cur_val1_srl_w_carry)));

	carry = next_carry;
}

__forceinline void tm_avx2_r256s_8::alg_2(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_01, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg2_values_256_8 + ((*rng_seed) * 32)));

	alg_2_sub(working_code2, working_code3, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code0, working_code1, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_avx2_r256s_8::alg_3(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed)
{
	uint8* rng_start = rng->regular_rng_values_256_8_shuffled + ((*rng_seed) * 128);

	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx2_r256s_8::alg_5_sub(__m256i& working_a, __m256i& working_b, __m256i& carry, __m256i& mask_top_80, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	// Shift bytes right 1 bit
	__m256i cur_val1_most = _mm256_and_si256(_mm256_slli_epi16(working_a, 1), mask_FE);
	// Shift bytes left 1 bit
	__m256i cur_val2_most = _mm256_and_si256(_mm256_srli_epi16(working_b, 1), mask_7F);

	// Mask off the top bits
	__m256i cur_val2_masked = _mm256_and_si256(working_b, mask_01);

	// Shift right 1 byte
	__m256i cur_val1_bit = _mm256_and_si256(working_a, mask_80);

	__m256i mask = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(3, 0, 0, 3));
	__m256i cur_val1_srl = _mm256_alignr_epi8(mask, cur_val1_bit, 1);
	__m256i cur_val1_srl_w_carry = _mm256_or_si256(cur_val1_srl, carry);

	// Save the next carry
	__m256i lo_to_hi = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(0, 0, 3, 0));
	__m256i next_carry = _mm256_bslli_epi128(lo_to_hi, 15);

	working_a = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_most), _mm256_castsi256_pd(cur_val2_masked)));
	working_b = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val2_most), _mm256_castsi256_pd(cur_val1_srl_w_carry)));

	carry = next_carry;
}

__forceinline void tm_avx2_r256s_8::alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg5_values_256_8 + ((*rng_seed) * 32)));

	alg_5_sub(working_code2, working_code3, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code0, working_code1, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_avx2_r256s_8::alg_6(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_7F)
{
	uint8* rng_start = rng->alg6_values_256_8_shuffled + ((*rng_seed) * 128);

	working_code0 = _mm256_srli_epi16(working_code0, 1);
	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	working_code0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_7F)));
	working_code0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

	working_code1 = _mm256_srli_epi16(working_code1, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	working_code1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_7F)));
	working_code1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

	working_code2 = _mm256_srli_epi16(working_code2, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	working_code2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_7F)));
	working_code2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

	working_code3 = _mm256_srli_epi16(working_code3, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	working_code3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_7F)));
	working_code3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx2_r256s_8::alg_7(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& mask_FF)
{
	working_code0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));
	working_code1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));
	working_code2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));
	working_code3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));
}

__forceinline void tm_avx2_r256s_8::add_alg(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, uint8* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);

	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	working_code0 = _mm256_add_epi8(working_code0, rng_val);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	working_code1 = _mm256_add_epi8(working_code1, rng_val);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	working_code2 = _mm256_add_epi8(working_code2, rng_val);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	working_code3 = _mm256_add_epi8(working_code3, rng_val);
}


void tm_avx2_r256s_8::run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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

void tm_avx2_r256s_8::run_all_maps(const key_schedule& schedule_entries)
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
			unsigned char current_byte = (uint8)((uint8*)working_code_data)[shuffle(i)];

			if (nibble == 1)
			{
				current_byte = current_byte >> 4;
			}

			// Mask off only 3 bits
			unsigned char algorithm_id = (current_byte >> 1) & 0x07;
			/*
			printf("%i ", algorithm_id);
			printf("%04X ", rng_seed);

			// store back to memory
			_mm256_store_si256((__m256i*)(working_code_data), working_code0);
			_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
			_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
			_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);

			print_working_code();
			*/

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
		/*
		printf("\n");
		if (schedule_counter == 6)
		{
			// store back to memory
			_mm256_store_si256((__m256i*)(working_code_data), working_code0);
			_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
			_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
			_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);

			print_working_code();
		}
		*/
	}

	// store back to memory
	_mm256_store_si256((__m256i*)(working_code_data), working_code0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);
}

bool tm_avx2_r256s_8::initialized = false;