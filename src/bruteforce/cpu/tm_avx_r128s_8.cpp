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

__forceinline void tm_avx_r128s_8::_load_from_mem(wc_r128& wc)
{
	wc.wc0 = _mm_load_si128((__m128i*)(working_code_data));
	wc.wc1 = _mm_load_si128((__m128i*)(working_code_data + 16));
	wc.wc2 = _mm_load_si128((__m128i*)(working_code_data + 32));
	wc.wc3 = _mm_load_si128((__m128i*)(working_code_data + 48));
	wc.wc4 = _mm_load_si128((__m128i*)(working_code_data + 64));
	wc.wc5 = _mm_load_si128((__m128i*)(working_code_data + 80));
	wc.wc6 = _mm_load_si128((__m128i*)(working_code_data + 96));
	wc.wc7 = _mm_load_si128((__m128i*)(working_code_data + 112));
}

__forceinline void tm_avx_r128s_8::_store_to_mem(wc_r128& wc)
{
	_mm_store_si128((__m128i*)(working_code_data), wc.wc0);
	_mm_store_si128((__m128i*)(working_code_data + 16), wc.wc1);
	_mm_store_si128((__m128i*)(working_code_data + 32), wc.wc2);
	_mm_store_si128((__m128i*)(working_code_data + 48), wc.wc3);
	_mm_store_si128((__m128i*)(working_code_data + 64), wc.wc4);
	_mm_store_si128((__m128i*)(working_code_data + 80), wc.wc5);
	_mm_store_si128((__m128i*)(working_code_data + 96), wc.wc6);
	_mm_store_si128((__m128i*)(working_code_data + 112), wc.wc7);
}

void tm_avx_r128s_8::expand(uint32 key, uint32 data)
{
	wc_r128 wc;

	_expand_code(key, data, wc);

	_store_to_mem(wc);
}

__forceinline void tm_avx_r128s_8::_expand_code(uint32 key, uint32 data, wc_r128& wc)
{
	uint64 x = ((uint64)key << 32) | data;

	__m128i a = _mm_cvtsi64_si128(x);
	__m128i lo_mask = _mm_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m128i hi_mask = _mm_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);

	__m128i lo = _mm_shuffle_epi8(a, lo_mask);
	__m128i hi = _mm_shuffle_epi8(a, hi_mask);

	wc.wc0 = lo;
	wc.wc1 = hi;
	wc.wc2 = lo;
	wc.wc3 = hi;
	wc.wc4 = lo;
	wc.wc5 = hi; 
	wc.wc6 = lo;
	wc.wc7 = hi;

	uint8* rng_start = rng->expansion_values_128_8_shuffled;
	uint16 rng_seed = (key >> 16) & 0xFFFF;

	add_alg(wc, &rng_seed, rng_start);
}

void tm_avx_r128s_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[shuffle_8(i, 128)] = new_data[i];
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
	wc_r128 wc;
	_load_from_mem(wc);

	reg_masks masks;
	if (algorithm_id == 0)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_0(wc, rng_seed, masks);
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

			add_alg(wc, rng_seed, rng_start);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_2(wc, rng_seed, masks);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 3)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_3(wc, rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 5)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_5(wc, rng_seed, masks);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_6(wc, rng_seed, masks);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 7)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_7(wc, masks);
		}
	}

	_store_to_mem(wc);
}

__forceinline void tm_avx_r128s_8::alg_0(wc_r128& wc, uint16* rng_seed, reg_masks& masks)
{
	uint8* rng_start = rng->alg0_values_128_8_shuffled + ((*rng_seed) * 128);

	alg_0_sub(wc.wc0, rng_start, masks);
	alg_0_sub(wc.wc1, rng_start + 16, masks);
	alg_0_sub(wc.wc2, rng_start + 32, masks);
	alg_0_sub(wc.wc3, rng_start + 48, masks);
	alg_0_sub(wc.wc4, rng_start + 64, masks);
	alg_0_sub(wc.wc5, rng_start + 80, masks);
	alg_0_sub(wc.wc6, rng_start + 96, masks);
	alg_0_sub(wc.wc7, rng_start + 112, masks);
}

__forceinline void tm_avx_r128s_8::alg_0_sub(__m128i& working_code, uint8* rng_start, reg_masks& masks)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_slli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, masks.vFE);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, reg_masks& masks)
{
	// bitwise right shift
	__m128i temp1 = _mm_srli_epi16(working_a, 1);
	// Mask off top bits
	__m128i cur_val1_most = _mm_and_si128(temp1, masks.v7F);

	// Mask off the top bits
	__m128i cur_val2_masked = _mm_and_si128(working_b, masks.v80);

	// bytewise right shift
	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	// mask off only the relevant low bit
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, masks.v01);
	// add the carry to the top
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	// bitwise right shift
	__m128i temp2 = _mm_slli_epi16(working_b, 1);
	// mask off lowest bit
	__m128i cur_val2_most = _mm_and_si128(temp2, masks.vFE);

	// Save the next carry
	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), masks.top01);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);

	carry = next_carry;
}

__forceinline void tm_avx_r128s_8::alg_2(wc_r128& wc, uint16* rng_seed, reg_masks& masks)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg2_values_128_8 + (*rng_seed * 16)));

	alg_2_sub(wc.wc6, wc.wc7, carry, masks);
	alg_2_sub(wc.wc4, wc.wc5, carry, masks);
	alg_2_sub(wc.wc2, wc.wc3, carry, masks);
	alg_2_sub(wc.wc0, wc.wc1, carry, masks);
}

__forceinline void tm_avx_r128s_8::alg_3(wc_r128& wc, uint16 * rng_seed)
{
	uint8 * rng_start = rng->regular_rng_values_128_8_shuffled + ((*rng_seed) * 128);

	xor_alg(wc, rng_start);
}

__forceinline void tm_avx_r128s_8::xor_alg(wc_r128& wc, uint8* values)
{
	wc.wc0 = _mm_xor_si128(wc.wc0, _mm_load_si128((__m128i*)values));
	wc.wc1 = _mm_xor_si128(wc.wc1, _mm_load_si128((__m128i*)(values + 16)));
	wc.wc2 = _mm_xor_si128(wc.wc2, _mm_load_si128((__m128i*)(values + 32)));
	wc.wc3 = _mm_xor_si128(wc.wc3, _mm_load_si128((__m128i*)(values + 48)));
	wc.wc4 = _mm_xor_si128(wc.wc4, _mm_load_si128((__m128i*)(values + 64)));
	wc.wc5 = _mm_xor_si128(wc.wc5, _mm_load_si128((__m128i*)(values + 80)));
	wc.wc6 = _mm_xor_si128(wc.wc6, _mm_load_si128((__m128i*)(values + 96)));
	wc.wc7 = _mm_xor_si128(wc.wc7, _mm_load_si128((__m128i*)(values + 112)));
}

__forceinline void tm_avx_r128s_8::alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, reg_masks& masks)
{
	// bitwise left shift
	__m128i temp1 = _mm_slli_epi16(working_a, 1);
	// Mask off low bits
	__m128i cur_val1_most = _mm_and_si128(temp1, masks.vFE);

	// Mask off the low bits
	__m128i cur_val2_masked = _mm_and_si128(working_b, masks.v01);

	// bytewise right shift
	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	// mask off only the relevant high bit
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, masks.v80);
	// add the carry to the top
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	// bitwise right shift
	__m128i temp2 = _mm_srli_epi16(working_b, 1);
	// mask off high bit
	__m128i cur_val2_most = _mm_and_si128(temp2, masks.v7F);

	// Save the next carry
	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), masks.top80);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);

	carry = next_carry;
}

__forceinline void tm_avx_r128s_8::alg_5(wc_r128& wc, uint16* rng_seed, reg_masks& masks)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg5_values_128_8 + (*rng_seed * 16)));

	alg_5_sub(wc.wc6, wc.wc7, carry, masks);
	alg_5_sub(wc.wc4, wc.wc5, carry, masks);
	alg_5_sub(wc.wc2, wc.wc3, carry, masks);
	alg_5_sub(wc.wc0, wc.wc1, carry, masks);
}

__forceinline void tm_avx_r128s_8::alg_6(wc_r128& wc, uint16* rng_seed, reg_masks& masks)
{
	uint8* rng_start = rng->alg6_values_128_8_shuffled + ((*rng_seed) * 128);

	alg_6_sub(wc.wc0, rng_start, masks);
	alg_6_sub(wc.wc1, rng_start + 16, masks);
	alg_6_sub(wc.wc2, rng_start + 32, masks);
	alg_6_sub(wc.wc3, rng_start + 48, masks);
	alg_6_sub(wc.wc4, rng_start + 64, masks);
	alg_6_sub(wc.wc5, rng_start + 80, masks);
	alg_6_sub(wc.wc6, rng_start + 96, masks);
	alg_6_sub(wc.wc7, rng_start + 112, masks);
}

__forceinline void tm_avx_r128s_8::alg_6_sub(__m128i& working_code, uint8* rng_start, reg_masks& masks)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_srli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, masks.v7F);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_7(wc_r128& wc, reg_masks& masks)
{
	wc.wc0 = _mm_xor_si128(wc.wc0, masks.vFF);
	wc.wc1 = _mm_xor_si128(wc.wc1, masks.vFF);
	wc.wc2 = _mm_xor_si128(wc.wc2, masks.vFF);
	wc.wc3 = _mm_xor_si128(wc.wc3, masks.vFF);
	wc.wc4 = _mm_xor_si128(wc.wc4, masks.vFF);
	wc.wc5 = _mm_xor_si128(wc.wc5, masks.vFF);
	wc.wc6 = _mm_xor_si128(wc.wc6, masks.vFF);
	wc.wc7 = _mm_xor_si128(wc.wc7, masks.vFF);
}

__forceinline void tm_avx_r128s_8::add_alg(wc_r128& wc, uint16* rng_seed, uint8* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);
	wc.wc0 = _mm_add_epi8(wc.wc0, _mm_load_si128((__m128i*)rng_start));
	wc.wc1 = _mm_add_epi8(wc.wc1, _mm_load_si128((__m128i*)(rng_start + 16)));
	wc.wc2 = _mm_add_epi8(wc.wc2, _mm_load_si128((__m128i*)(rng_start + 32)));
	wc.wc3 = _mm_add_epi8(wc.wc3, _mm_load_si128((__m128i*)(rng_start + 48)));
	wc.wc4 = _mm_add_epi8(wc.wc4, _mm_load_si128((__m128i*)(rng_start + 64)));
	wc.wc5 = _mm_add_epi8(wc.wc5, _mm_load_si128((__m128i*)(rng_start + 80)));
	wc.wc6 = _mm_add_epi8(wc.wc6, _mm_load_si128((__m128i*)(rng_start + 96)));
	wc.wc7 = _mm_add_epi8(wc.wc7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline bool tm_avx_r128s_8::check_carnival_world_checksum(wc_r128& wc)
{
	return _calculate_carnival_world_checksum(wc) == _fetch_carnival_world_checksum_value(wc.wc0, wc.wc1);
}

__forceinline bool tm_avx_r128s_8::check_other_world_checksum(wc_r128& wc)
{
	return _calculate_other_world_checksum(wc) == _fetch_other_world_checksum_value(wc.wc2, wc.wc3);
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
		unsigned char current_byte = (uint8)((uint8*)working_code_data)[shuffle_8(i, 128)];

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
	wc_r128 wc;
	_load_from_mem(wc);

	reg_masks m;
	_run_all_maps(wc, schedule_entries, m);

	_store_to_mem(wc);
}

__forceinline void tm_avx_r128s_8::_run_all_maps(wc_r128& wc, const key_schedule& schedule_entries, reg_masks& masks)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries.entries.begin(); it != schedule_entries.entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

		uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
		uint16 nibble_selector = schedule_entry.nibble_selector;

		// Next, the working code is processed with the same steps 16 times:
		for (int i = 0; i < 16; i++)
		{
			_mm_store_si128((__m128i*)(working_code_data), wc.wc0);
			_mm_store_si128((__m128i*)(working_code_data + 16), wc.wc1);

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
				alg_0(wc, &rng_seed, masks);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 1 || algorithm_id == 4)
			{
				uint8* rng_start = rng->regular_rng_values_128_8_shuffled;

				if (algorithm_id == 4)
				{
					rng_start = rng->alg4_values_128_8_shuffled;
				}

				add_alg(wc, &rng_seed, rng_start);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 2)
			{
				alg_2(wc, &rng_seed, masks);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 3)
			{
				alg_3(wc, &rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 5)
			{
				alg_5(wc, &rng_seed, masks);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 6)
			{
				alg_6(wc, &rng_seed, masks);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 7)
			{
				alg_7(wc, masks);
			}
		}
	}
}

void tm_avx_r128s_8::decrypt_carnival_world()
{
	wc_r128 wc;
	_load_from_mem(wc);

	_decrypt_carnival_world(wc);

	_store_to_mem(wc);
}

void tm_avx_r128s_8::decrypt_other_world()
{
	wc_r128 wc;
	_load_from_mem(wc);

	_decrypt_other_world(wc);

	_store_to_mem(wc);
}

void tm_avx_r128s_8::_decrypt_carnival_world(wc_r128& wc)
{
	xor_alg(wc, carnival_world_data_shuffled);
}

void tm_avx_r128s_8::_decrypt_other_world(wc_r128& wc)
{
	xor_alg(wc, other_world_data_shuffled);
}

__forceinline uint16 tm_avx_r128s_8::masked_checksum(wc_r128& wc, uint8* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m128i sum_mask = _mm_load_si128((__m128i*)(mask));
	mid_sum(sum, wc.wc0, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 16));
	mid_sum(sum, wc.wc1, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 32));
	mid_sum(sum, wc.wc2, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 48));
	mid_sum(sum, wc.wc3, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 64));
	mid_sum(sum, wc.wc4, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 80));
	mid_sum(sum, wc.wc5, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 96));
	mid_sum(sum, wc.wc6, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 112));
	mid_sum(sum, wc.wc7, sum_mask, lo_mask);

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
	wc_r128 wc;
	_load_from_mem(wc);

	return _calculate_carnival_world_checksum(wc);
}

uint16 tm_avx_r128s_8::calculate_other_world_checksum()
{
	wc_r128 wc;
	_load_from_mem(wc);

	return _calculate_other_world_checksum(wc);
}

uint16 tm_avx_r128s_8::_calculate_carnival_world_checksum(wc_r128& wc)
{
	return masked_checksum(wc, carnival_world_checksum_mask_shuffled);
}

uint16 tm_avx_r128s_8::_calculate_other_world_checksum(wc_r128& wc)
{
	return masked_checksum(wc, other_world_checksum_mask_shuffled);
}

__forceinline uint16 tm_avx_r128s_8::fetch_checksum_value(__m128i& working_code0, __m128i& working_code1, uint8 code_length)
{
	_mm_store_si128((__m128i*)(working_code_data), working_code0);
	_mm_store_si128((__m128i*)(working_code_data + 16), working_code1);

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
	unsigned char checksum_low = _mm_extract_epi8(working_code1, 7);  // working_code_data[23]
	unsigned char checksum_hi = _mm_extract_epi8(working_code0, 7);  // working_code_data[7]
	return (uint16)((checksum_hi << 8) | checksum_low);

	//return fetch_checksum_value(working_code0, working_code1, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16 tm_avx_r128s_8::_fetch_other_world_checksum_value(__m128i& working_code2, __m128i& working_code3)
{
	unsigned char checksum_low = _mm_extract_epi8(working_code2, 7);  // working_code_data[39]
	unsigned char checksum_hi = _mm_extract_epi8(working_code3, 6);  // working_code_data[54]
	return (uint16)((checksum_hi << 8) | checksum_low);

	//return fetch_checksum_value(working_code2, working_code3, OTHER_WORLD_CODE_LENGTH - 2);
}

void tm_avx_r128s_8::run_bruteforce_data(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	wc_r128 wc;
	reg_masks masks;

	uint32 output_pos = 0;
	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - output_pos) < 5)
		{
			*result_size = result_max_size;
			return;
		}
		uint32 data = start_data + i;

		_expand_code(key, data, wc);

		_run_all_maps(wc, schedule_entries, masks);

		_store_to_mem(wc);

		wc_r128 wc_xor = wc;

		_decrypt_carnival_world(wc_xor);

		if (check_carnival_world_checksum(wc_xor))
		{
			_store_to_mem(wc_xor);

			*((uint32*)(&result_data[output_pos])) = data;

			uint8 unshuffled_data[128];
			unshuffle_mem(working_code_data, unshuffled_data, 128, false);

 			result_data[output_pos + 4] = check_machine_code(unshuffled_data, CARNIVAL_WORLD);
			output_pos += 5;
		}
		else
		{
			wc_xor = wc;

			_decrypt_other_world(wc_xor);

			if (check_other_world_checksum(wc_xor))
			{
				_store_to_mem(wc_xor);
				*((uint32*)(&result_data[output_pos])) = data;

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

void tm_avx_r128s_8::run_bruteforce_boinc(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	wc_r128 wc;
	reg_masks masks;

	uint32 output_pos = 0;
	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - output_pos) < 5)
		{
			*result_size = result_max_size;
			return;
		}
		uint32 data = start_data + i;

		_expand_code(key, data, wc);

		_run_all_maps(wc, schedule_entries, masks);

		wc_r128 wc_xor = wc;

		_decrypt_carnival_world(wc_xor);

		if (check_carnival_world_checksum(wc_xor))
		{
			_store_to_mem(wc_xor);

			*((uint32*)(&result_data[output_pos])) = data;

			uint8 unshuffled_data[128];
			unshuffle_mem(working_code_data, unshuffled_data, 128, false);

			result_data[output_pos + 4] = check_machine_code(unshuffled_data, CARNIVAL_WORLD);
			output_pos += 5;
		}
		else
		{
			wc_xor = wc;

			_decrypt_other_world(wc_xor);

			if (check_other_world_checksum(wc_xor))
			{
				_store_to_mem(wc_xor);

				*((uint32*)(&result_data[output_pos])) = data;

				uint8 unshuffled_data[128];
				unshuffle_mem(working_code_data, unshuffled_data, 128, false);

				result_data[output_pos + 4] = check_machine_code(unshuffled_data, OTHER_WORLD) | OTHER_WORLD;
				output_pos += 5;
			}
		}

		report_progress((float)(i + 1) / amount_to_run);
	}

	*result_size = output_pos;
}

void tm_avx_r128s_8::compute_challenge_flags(uint32 key, uint32 data, const key_schedule& schedule_entries, uint8& carnival_flags_out, uint8& other_flags_out)
{
	wc_r128 wc;
	reg_masks masks;

	_expand_code(key, data, wc);

	_run_all_maps(wc, schedule_entries, masks);

	// Carnival world — no checksum check
	{
		wc_r128 wc_copy = wc;

		_decrypt_carnival_world(wc_copy);
		_store_to_mem(wc_copy);

		uint8 unshuffled_data[128];
		unshuffle_mem(working_code_data, unshuffled_data, 128, false);

		carnival_flags_out = check_machine_code(unshuffled_data, CARNIVAL_WORLD);
	}

	// Other world — no checksum check
	{
		wc_r128 wc_copy = wc;

		_decrypt_other_world(wc_copy);
		_store_to_mem(wc_copy);

		uint8 unshuffled_data[128];
		unshuffle_mem(working_code_data, unshuffled_data, 128, false);

		other_flags_out = check_machine_code(unshuffled_data, OTHER_WORLD) | OTHER_WORLD;
	}
}

void tm_avx_r128s_8::test_expand_and_map(uint32 key, uint32 data, const key_schedule& schedule, uint8* result_out)
{
	wc_r128 wc;
	reg_masks masks;
	_expand_code(key, data, wc);
	_run_all_maps(wc, schedule, masks);
	_store_to_mem(wc);
	fetch_data(result_out);
}

bool tm_avx_r128s_8::test_pipeline_validate(uint32 key, uint32 data, const key_schedule& schedule, int world)
{
	wc_r128 wc;
	reg_masks masks;
	_expand_code(key, data, wc);
	_run_all_maps(wc, schedule, masks);
	wc_r128 wc_xor = wc;
	if (world == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(wc_xor);
		return check_carnival_world_checksum(wc_xor);
	}
	else
	{
		_decrypt_other_world(wc_xor);
		return check_other_world_checksum(wc_xor);
	}
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






void tm_avx_r128s_8::run_first_map(uint32_t key, uint32_t data, const key_schedule& schedule_entries)
{
	wc_r128 wc;
	reg_masks masks;

	_expand_code(key, data, wc);

	key_schedule::key_schedule_entry schedule_entry = *schedule_entries.entries.begin();


	uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
	uint16 nibble_selector = schedule_entry.nibble_selector;

	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		_mm_store_si128((__m128i*)(working_code_data), wc.wc0);
		_mm_store_si128((__m128i*)(working_code_data + 16), wc.wc1);

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
			alg_0(wc, &rng_seed, masks);
			rng_seed = rng->seed_forward_128[rng_seed];
		}
		else if (algorithm_id == 1 || algorithm_id == 4)
		{
			uint8* rng_start = rng->regular_rng_values_128_8_shuffled;

			if (algorithm_id == 4)
			{
				rng_start = rng->alg4_values_128_8_shuffled;
			}

			add_alg(wc, &rng_seed, rng_start);
			rng_seed = rng->seed_forward_128[rng_seed];
		}
		else if (algorithm_id == 2)
		{
			alg_2(wc, &rng_seed, masks);
			rng_seed = rng->seed_forward_1[rng_seed];
		}
		else if (algorithm_id == 3)
		{
			alg_3(wc, &rng_seed);
			rng_seed = rng->seed_forward_128[rng_seed];
		}
		else if (algorithm_id == 5)
		{
			alg_5(wc, &rng_seed, masks);
			rng_seed = rng->seed_forward_1[rng_seed];
		}
		else if (algorithm_id == 6)
		{
			alg_6(wc, &rng_seed, masks);
			rng_seed = rng->seed_forward_128[rng_seed];
		}
		else if (algorithm_id == 7)
		{
			alg_7(wc, masks);
		}
	}

	_store_to_mem(wc);
}