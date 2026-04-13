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

tm_avx_r128s_8::tm_avx_r128s_8(RNG* rng_obj) : tm_avx_r128s_8(rng_obj, 0) {}

tm_avx_r128s_8::tm_avx_r128s_8(RNG* rng_obj, const uint32_t key) : tm_avx_r128s_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx_r128s_8::tm_avx_r128s_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj)
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;
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

__forceinline void tm_avx_r128s_8::_load_from_mem(WC_ARGS)
{
	wc0 = _mm_load_si128((__m128i*)(working_code_data));
	wc1 = _mm_load_si128((__m128i*)(working_code_data + 16));
	wc2 = _mm_load_si128((__m128i*)(working_code_data + 32));
	wc3 = _mm_load_si128((__m128i*)(working_code_data + 48));
	wc4 = _mm_load_si128((__m128i*)(working_code_data + 64));
	wc5 = _mm_load_si128((__m128i*)(working_code_data + 80));
	wc6 = _mm_load_si128((__m128i*)(working_code_data + 96));
	wc7 = _mm_load_si128((__m128i*)(working_code_data + 112));
}

__forceinline void tm_avx_r128s_8::_store_to_mem(WC_ARGS)
{
	_mm_store_si128((__m128i*)(working_code_data), wc0);
	_mm_store_si128((__m128i*)(working_code_data + 16), wc1);
	_mm_store_si128((__m128i*)(working_code_data + 32), wc2);
	_mm_store_si128((__m128i*)(working_code_data + 48), wc3);
	_mm_store_si128((__m128i*)(working_code_data + 64), wc4);
	_mm_store_si128((__m128i*)(working_code_data + 80), wc5);
	_mm_store_si128((__m128i*)(working_code_data + 96), wc6);
	_mm_store_si128((__m128i*)(working_code_data + 112), wc7);
}

__forceinline void tm_avx_r128s_8::_expand_code(uint32 data, WC_ARGS)
{
	uint64 x = ((uint64)key << 32) | data;

	__m128i a = _mm_cvtsi64_si128(x);
	__m128i lo_mask = _mm_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m128i hi_mask = _mm_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);

	__m128i lo = _mm_shuffle_epi8(a, lo_mask);
	__m128i hi = _mm_shuffle_epi8(a, hi_mask);

	wc0 = lo;
	wc1 = hi;
	wc2 = lo;
	wc3 = hi;
	wc4 = lo;
	wc5 = hi; 
	wc6 = lo;
	wc7 = hi;

	uint8* rng_start = rng->expansion_values_128_8_shuffled;
	uint16 rng_seed = (key >> 16) & 0xFFFF;

	add_alg(WC_PASS, &rng_seed, rng_start);
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

__forceinline void tm_avx_r128s_8::_run_alg(WC_ARGS, int algorithm_id, uint16* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS, rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		uint8* rng_start = rng->regular_rng_values_128_8_shuffled;

		if (algorithm_id == 4)
		{
			rng_start = rng->alg4_values_128_8_shuffled;
		}

		add_alg(WC_PASS, rng_seed, rng_start);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS, rng_seed);
		*rng_seed = rng->seed_forward_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS, rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS, rng_seed);
		*rng_seed = rng->seed_forward_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS, rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS);
	}
}


void tm_avx_r128s_8::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	WC_VARS;
	_load_from_mem(WC_PASS);

	for (int j = 0; j < iterations; j++)
	{
		_run_alg(WC_PASS, algorithm_id, rng_seed);
	}

	_store_to_mem(WC_PASS);
}

__forceinline void tm_avx_r128s_8::alg_0(WC_ARGS, uint16* rng_seed)
{
	uint8* rng_start = rng->alg0_values_128_8_shuffled + ((*rng_seed) * 128);

	alg_0_sub(wc0, rng_start);
	alg_0_sub(wc1, rng_start + 16);
	alg_0_sub(wc2, rng_start + 32);
	alg_0_sub(wc3, rng_start + 48);
	alg_0_sub(wc4, rng_start + 64);
	alg_0_sub(wc5, rng_start + 80);
	alg_0_sub(wc6, rng_start + 96);
	alg_0_sub(wc7, rng_start + 112);
}

__forceinline void tm_avx_r128s_8::alg_0_sub(__m128i& working_code, uint8* rng_start)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_slli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_FE);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry)
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

__forceinline void tm_avx_r128s_8::alg_2(WC_ARGS, uint16* rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg2_values_128_8 + (*rng_seed * 16)));

	alg_2_sub(wc6, wc7, carry);
	alg_2_sub(wc4, wc5, carry);
	alg_2_sub(wc2, wc3, carry);
	alg_2_sub(wc0, wc1, carry);
}

__forceinline void tm_avx_r128s_8::alg_3(WC_ARGS, uint16* rng_seed)
{
	uint8 * rng_start = rng->regular_rng_values_128_8_shuffled + ((*rng_seed) * 128);

	xor_alg(WC_PASS, rng_start);
}

__forceinline void tm_avx_r128s_8::xor_alg(WC_ARGS, uint8* values)
{
	wc0 = _mm_xor_si128(wc0, _mm_load_si128((__m128i*)values));
	wc1 = _mm_xor_si128(wc1, _mm_load_si128((__m128i*)(values + 16)));
	wc2 = _mm_xor_si128(wc2, _mm_load_si128((__m128i*)(values + 32)));
	wc3 = _mm_xor_si128(wc3, _mm_load_si128((__m128i*)(values + 48)));
	wc4 = _mm_xor_si128(wc4, _mm_load_si128((__m128i*)(values + 64)));
	wc5 = _mm_xor_si128(wc5, _mm_load_si128((__m128i*)(values + 80)));
	wc6 = _mm_xor_si128(wc6, _mm_load_si128((__m128i*)(values + 96)));
	wc7 = _mm_xor_si128(wc7, _mm_load_si128((__m128i*)(values + 112)));
}

__forceinline void tm_avx_r128s_8::alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry)
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

__forceinline void tm_avx_r128s_8::alg_5(WC_ARGS, uint16* rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg5_values_128_8 + (*rng_seed * 16)));

	alg_5_sub(wc6, wc7, carry);
	alg_5_sub(wc4, wc5, carry);
	alg_5_sub(wc2, wc3, carry);
	alg_5_sub(wc0, wc1, carry);
}

__forceinline void tm_avx_r128s_8::alg_6(WC_ARGS, uint16* rng_seed)
{
	uint8* rng_start = rng->alg6_values_128_8_shuffled + ((*rng_seed) * 128);

	alg_6_sub(wc0, rng_start);
	alg_6_sub(wc1, rng_start + 16);
	alg_6_sub(wc2, rng_start + 32);
	alg_6_sub(wc3, rng_start + 48);
	alg_6_sub(wc4, rng_start + 64);
	alg_6_sub(wc5, rng_start + 80);
	alg_6_sub(wc6, rng_start + 96);
	alg_6_sub(wc7, rng_start + 112);
}

__forceinline void tm_avx_r128s_8::alg_6_sub(__m128i& working_code, uint8* rng_start)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_srli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_7F);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_7(WC_ARGS)
{
	wc0 = _mm_xor_si128(wc0, mask_FF);
	wc1 = _mm_xor_si128(wc1, mask_FF);
	wc2 = _mm_xor_si128(wc2, mask_FF);
	wc3 = _mm_xor_si128(wc3, mask_FF);
	wc4 = _mm_xor_si128(wc4, mask_FF);
	wc5 = _mm_xor_si128(wc5, mask_FF);
	wc6 = _mm_xor_si128(wc6, mask_FF);
	wc7 = _mm_xor_si128(wc7, mask_FF);
}

__forceinline void tm_avx_r128s_8::add_alg(WC_ARGS, uint16* rng_seed, uint8* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);
	wc0 = _mm_add_epi8(wc0, _mm_load_si128((__m128i*)rng_start));
	wc1 = _mm_add_epi8(wc1, _mm_load_si128((__m128i*)(rng_start + 16)));
	wc2 = _mm_add_epi8(wc2, _mm_load_si128((__m128i*)(rng_start + 32)));
	wc3 = _mm_add_epi8(wc3, _mm_load_si128((__m128i*)(rng_start + 48)));
	wc4 = _mm_add_epi8(wc4, _mm_load_si128((__m128i*)(rng_start + 64)));
	wc5 = _mm_add_epi8(wc5, _mm_load_si128((__m128i*)(rng_start + 80)));
	wc6 = _mm_add_epi8(wc6, _mm_load_si128((__m128i*)(rng_start + 96)));
	wc7 = _mm_add_epi8(wc7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline bool tm_avx_r128s_8::check_carnival_world_checksum(WC_ARGS)
{
	return _calculate_carnival_world_checksum(WC_PASS) == _fetch_carnival_world_checksum_value(wc0, wc1, wc2, wc3);
}

__forceinline bool tm_avx_r128s_8::check_other_world_checksum(WC_ARGS)
{
	return _calculate_other_world_checksum(WC_PASS) == _fetch_other_world_checksum_value(wc0, wc1, wc2, wc3);
}

__forceinline void tm_avx_r128s_8::mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask)
{
	__m128i temp_masked = _mm_and_si128(working_code, sum_mask);

	__m128i temp1_lo_lo = _mm_and_si128(temp_masked, lo_mask);
	__m128i temp1_lo_hi = _mm_srli_epi16(temp_masked, 8);

	sum = _mm_add_epi16(sum, temp1_lo_lo);
	sum = _mm_add_epi16(sum, temp1_lo_hi);
}

__forceinline void tm_avx_r128s_8::_run_one_map(WC_ARGS, const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
	uint16 nibble_selector = schedule_entry.nibble_selector;

	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		_mm_store_si128((__m128i*)(working_code_data), wc0);
		_mm_store_si128((__m128i*)(working_code_data + 16), wc1);

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

		_run_alg(WC_PASS, algorithm_id, &rng_seed);
	}
}

__forceinline void tm_avx_r128s_8::_run_all_maps(WC_ARGS)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries->entries.begin(); it != schedule_entries->entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

		_run_one_map(WC_PASS, schedule_entry);
	}
}

void tm_avx_r128s_8::decrypt_carnival_world()
{
	WC_VARS;
	_load_from_mem(WC_PASS);

	_decrypt_carnival_world(WC_PASS);

	_store_to_mem(WC_PASS);
}

void tm_avx_r128s_8::decrypt_other_world()
{
	WC_VARS;
	_load_from_mem(WC_PASS);

	_decrypt_other_world(WC_PASS);

	_store_to_mem(WC_PASS);
}

__forceinline void tm_avx_r128s_8::_decrypt_carnival_world(WC_ARGS)
{
	xor_alg(WC_PASS, carnival_world_data_shuffled);
}

__forceinline void tm_avx_r128s_8::_decrypt_other_world(WC_ARGS)
{
	xor_alg(WC_PASS, other_world_data_shuffled);
}

__forceinline uint16 tm_avx_r128s_8::masked_checksum(WC_ARGS, uint8* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m128i sum_mask = _mm_load_si128((__m128i*)(mask));
	mid_sum(sum, wc0, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 16));
	mid_sum(sum, wc1, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 32));
	mid_sum(sum, wc2, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 48));
	mid_sum(sum, wc3, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 64));
	mid_sum(sum, wc4, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 80));
	mid_sum(sum, wc5, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 96));
	mid_sum(sum, wc6, sum_mask, lo_mask);

	sum_mask = _mm_load_si128((__m128i*)(mask + 112));
	mid_sum(sum, wc7, sum_mask, lo_mask);

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
	WC_VARS;
	_load_from_mem(WC_PASS);

	return _calculate_carnival_world_checksum(WC_PASS);
}

uint16 tm_avx_r128s_8::calculate_other_world_checksum()
{
	WC_VARS;
	_load_from_mem(WC_PASS);

	return _calculate_other_world_checksum(WC_PASS);
}

__forceinline uint16 tm_avx_r128s_8::_calculate_carnival_world_checksum(WC_ARGS)
{
	return masked_checksum(WC_PASS, carnival_world_checksum_mask_shuffled);
}

__forceinline uint16 tm_avx_r128s_8::_calculate_other_world_checksum(WC_ARGS)
{
	return masked_checksum(WC_PASS, other_world_checksum_mask_shuffled);
}

__forceinline uint16 tm_avx_r128s_8::fetch_checksum_value(__m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3, uint8 code_length)
{
	_mm_store_si128((__m128i*)(working_code_data), wc0);
	_mm_store_si128((__m128i*)(working_code_data + 16), wc1);
	_mm_store_si128((__m128i*)(working_code_data + 32), wc2);
	_mm_store_si128((__m128i*)(working_code_data + 48), wc3);

	unsigned char checksum_low = (uint8)((uint8*)working_code_data)[shuffle_8((127 - code_length), 128)];
	unsigned char checksum_hi = (uint8)((uint8*)working_code_data)[shuffle_8((127 - (code_length + 1)), 128)];
	uint16 checksum = (checksum_hi << 8) | checksum_low;

	return checksum;
}

uint16 tm_avx_r128s_8::fetch_carnival_world_checksum_value()
{
	__m128i wc0 = _mm_load_si128((__m128i*)(working_code_data));
	__m128i wc1 = _mm_load_si128((__m128i*)(working_code_data + 16));
	__m128i wc2 = _mm_load_si128((__m128i*)(working_code_data + 32));
	__m128i wc3 = _mm_load_si128((__m128i*)(working_code_data + 48));

	return _fetch_carnival_world_checksum_value(wc0, wc1, wc2, wc3);
}

uint16 tm_avx_r128s_8::fetch_other_world_checksum_value()
{
	__m128i wc0 = _mm_load_si128((__m128i*)(working_code_data));
	__m128i wc1 = _mm_load_si128((__m128i*)(working_code_data + 16));
	__m128i wc2 = _mm_load_si128((__m128i*)(working_code_data + 32));
	__m128i wc3 = _mm_load_si128((__m128i*)(working_code_data + 48));

	return _fetch_other_world_checksum_value(wc0, wc1, wc2, wc3);
}

__forceinline uint16 tm_avx_r128s_8::_fetch_carnival_world_checksum_value(__m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3)
{
	return fetch_checksum_value(wc0, wc1, wc2, wc3, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16 tm_avx_r128s_8::_fetch_other_world_checksum_value(__m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3)
{
	return fetch_checksum_value(wc0, wc1, wc2, wc3, OTHER_WORLD_CODE_LENGTH - 2);
}
/*
void tm_avx_r128s_8::run_bruteforce_data(uint32 start_data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	run_bruteforce_boinc(start_data, amount_to_run, report_progress, result_data, result_max_size, result_size);
}*/

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx_r128s_8::_run_bruteforce(WC_ARGS, uint32 data, uint8* result_data, uint32* result_size)
{
	_expand_code(data, WC_PASS);

	_run_all_maps(WC_PASS);

	__m128i wc0_xor = wc0;
	__m128i wc1_xor = wc1;
	__m128i wc2_xor = wc2;
	__m128i wc3_xor = wc3;
	__m128i wc4_xor = wc4;
	__m128i wc5_xor = wc5;
	__m128i wc6_xor = wc6;
	__m128i wc7_xor = wc7;

	_decrypt_carnival_world(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
	
	if constexpr (CHECK_CHECKSUMS)
	{
		if (check_carnival_world_checksum(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor))
		{
			_store_to_mem(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);

			*((uint32*)(&result_data[*result_size])) = data;

			uint8 unshuffled_data[128];
			unshuffle_mem(working_code_data, unshuffled_data, 128, false);

			result_data[*result_size + 4] = check_machine_code(unshuffled_data, CARNIVAL_WORLD);
			*result_size += 5;
		}
		else
		{
			wc0_xor = wc0;
			wc1_xor = wc1;
			wc2_xor = wc2;
			wc3_xor = wc3;
			wc4_xor = wc4;
			wc5_xor = wc5;
			wc6_xor = wc6;
			wc7_xor = wc7;

			_decrypt_other_world(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);

			if (check_other_world_checksum(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor))
			{
				_store_to_mem(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);

				*((uint32*)(&result_data[*result_size])) = data;

				uint8 unshuffled_data[128];
				unshuffle_mem(working_code_data, unshuffled_data, 128, false);

				result_data[*result_size + 4] = check_machine_code(unshuffled_data, OTHER_WORLD) | OTHER_WORLD;
				*result_size += 5;
			}
		}
	}
	else
	{
		_store_to_mem(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
		uint8 unshuffled_data[128];
		unshuffle_mem(working_code_data, unshuffled_data, 128, false);

		result_data[*result_size] = check_machine_code(unshuffled_data, CARNIVAL_WORLD);

		wc0_xor = wc0;
		wc1_xor = wc1;
		wc2_xor = wc2;
		wc3_xor = wc3;
		wc4_xor = wc4;
		wc5_xor = wc5;
		wc6_xor = wc6;
		wc7_xor = wc7;

		_decrypt_other_world(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
		_store_to_mem(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
		unshuffle_mem(working_code_data, unshuffled_data, 128, false);

		result_data[*result_size + 1] = check_machine_code(unshuffled_data, OTHER_WORLD) | OTHER_WORLD;

		*result_size += 2;
	}
}


void tm_avx_r128s_8::run_bruteforce_boinc(uint32 start_data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	WC_VARS;

	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
		{
			return;
		}
		uint32 data = start_data + i;

		_run_bruteforce<true>(WC_PASS, data, result_data, result_size);

		report_progress((float)(i + 1) / amount_to_run);
	}
}

void tm_avx_r128s_8::compute_challenge_flags(uint32 data, uint8& carnival_flags_out, uint8& other_flags_out)
{
	WC_VARS;

	uint8 result_data[2];
	uint32 result_pos = 0;

	_run_bruteforce<false>(WC_PASS, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
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

void tm_avx_r128s_8::test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed)
{
	WC_VARS;
	load_data(data);
	_load_from_mem(WC_PASS);
	_run_alg(WC_PASS, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS);
	fetch_data(data);
}

void tm_avx_r128s_8::test_expansion(uint32_t data, uint8* result_out)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_store_to_mem(WC_PASS);
	fetch_data(result_out);
}

void tm_avx_r128s_8::test_bruteforce_data(uint32 data, uint8* result_out)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_run_all_maps(WC_PASS);
	_store_to_mem(WC_PASS);
	fetch_data(result_out);
}

bool tm_avx_r128s_8::test_bruteforce_checksum(uint32 data, int world)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_run_all_maps(WC_PASS);

	__m128i wc0_xor = wc0;
	__m128i wc1_xor = wc1;
	__m128i wc2_xor = wc2;
	__m128i wc3_xor = wc3;
	__m128i wc4_xor = wc4;
	__m128i wc5_xor = wc5;
	__m128i wc6_xor = wc6;
	__m128i wc7_xor = wc7;

	if (world == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
		return check_carnival_world_checksum(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
	}
	else
	{
		_decrypt_other_world(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
		return check_other_world_checksum(wc0_xor, wc1_xor, wc2_xor, wc3_xor, wc4_xor, wc5_xor, wc6_xor, wc7_xor);
	}
}


alignas(16) const __m128i tm_avx_r128s_8::mask_FF = _mm_set1_epi16(0xFFFF);
alignas(16) const __m128i tm_avx_r128s_8::mask_FE = _mm_set1_epi16(0xFEFE);
alignas(16) const __m128i tm_avx_r128s_8::mask_7F = _mm_set1_epi16(0x7F7F);
alignas(16) const __m128i tm_avx_r128s_8::mask_80 = _mm_set1_epi16(0x8080);
alignas(16) const __m128i tm_avx_r128s_8::mask_01 = _mm_set1_epi16(0x0101);
alignas(16) const __m128i tm_avx_r128s_8::mask_top_01 = _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0);
alignas(16) const __m128i tm_avx_r128s_8::mask_top_80 = _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0);