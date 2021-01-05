#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2

#include "data_sizes.h"
#include "tm_sse2_8_shuffled.h"

int shuffle_128(int addr)
{
	return (addr / 32) * 32 + (addr % 2) * 16 + ((addr / 2) % 16);
}

tm_sse2_8_shuffled::tm_sse2_8_shuffled(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_sse2_8_shuffled::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();
		rng->generate_regular_rng_values_8_lo();
		rng->generate_regular_rng_values_8_hi();
		rng->generate_regular_rng_values_128_8_shuffled();

		rng->generate_alg0_values_128_8_shuffled();
		rng->generate_alg2_values_128_8();
		rng->generate_alg4_values_128_8_shuffled();
		rng->generate_alg5_values_128_8();
		rng->generate_alg6_values_128_8_shuffled();

		initialized = true;
	}
	obj_name = "tm_sse2_8_shuffled";
}

void tm_sse2_8_shuffled::expand(uint32 key, uint32 data)
{
	uint8* x = (uint8*)working_code_data;
	for (int i = 0; i < 128; i += 8)
	{
		x[shuffle_128(i)] = (key >> 24) & 0xFF;
		x[shuffle_128(i + 1)] = (key >> 16) & 0xFF;
		x[shuffle_128(i + 2)] = (key >> 8) & 0xFF;
		x[shuffle_128(i + 3)] = key & 0xFF;

		x[shuffle_128(i + 4)] = (data >> 24) & 0xFF;
		x[shuffle_128(i + 5)] = (data >> 16) & 0xFF;
		x[shuffle_128(i + 6)] = (data >> 8) & 0xFF;
		x[shuffle_128(i + 7)] = data & 0xFF;
	}

	uint16 rng_seed = (key >> 16) & 0xFFFF;
	for (int i = 0; i < 128; i++)
	{
		x[shuffle_128(i)] += rng->expansion_values_8[rng_seed * 128 + i];
	}
}

void tm_sse2_8_shuffled::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[shuffle_128(i)] = new_data[i];
	}
}

void tm_sse2_8_shuffled::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8*)working_code_data)[shuffle_128(i)];
	}
}

void tm_sse2_8_shuffled::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	__m128i working_code0 = _mm_loadu_si128((__m128i*)(working_code_data));
	__m128i working_code1 = _mm_loadu_si128((__m128i*)(working_code_data + 16));
	__m128i working_code2 = _mm_loadu_si128((__m128i*)(working_code_data + 32));
	__m128i working_code3 = _mm_loadu_si128((__m128i*)(working_code_data + 48));
	__m128i working_code4 = _mm_loadu_si128((__m128i*)(working_code_data + 64));
	__m128i working_code5 = _mm_loadu_si128((__m128i*)(working_code_data + 80));
	__m128i working_code6 = _mm_loadu_si128((__m128i*)(working_code_data + 96));
	__m128i working_code7 = _mm_loadu_si128((__m128i*)(working_code_data + 112));

	__m128i mask_FF = _mm_set1_epi8(0xFF);
	__m128i mask_FE = _mm_set1_epi8(0xFE);
	__m128i mask_7F = _mm_set1_epi8(0x7F);
	__m128i mask_01 = _mm_set1_epi8(0x01);
	__m128i mask_80 = _mm_set1_epi8(0x80);
	__m128i mask_top_01 = _mm_set_epi8(0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m128i mask_top_80 = _mm_set_epi8(0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (algorithm_id == 0)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_0(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_FE);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 1)
	{
		for (int i = 0; i < iterations; i++)
		{
			add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng->regular_rng_values_128_8_shuffled, rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_2(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 3)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_3(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 4)
	{
		for (int i = 0; i < iterations; i++)
		{
			add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng->alg4_values_128_8_shuffled, rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 5)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_5(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_6(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed, mask_7F);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 7)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_7(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, mask_FF);
		}
	}

	_mm_store_si128((__m128i*)(working_code_data), working_code0);
	_mm_store_si128((__m128i*)(working_code_data + 16), working_code1);
	_mm_store_si128((__m128i*)(working_code_data + 32), working_code2);
	_mm_store_si128((__m128i*)(working_code_data + 48), working_code3);
	_mm_store_si128((__m128i*)(working_code_data + 64), working_code4);
	_mm_store_si128((__m128i*)(working_code_data + 80), working_code5);
	_mm_store_si128((__m128i*)(working_code_data + 96), working_code6);
	_mm_store_si128((__m128i*)(working_code_data + 112), working_code7);
}


__forceinline void tm_sse2_8_shuffled::alg_0(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_FE)
{
	uint8* rng_start = rng->alg0_values_128_8_shuffled + (*rng_seed * 128);
	working_code0 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code0, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start)));
	working_code1 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code1, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start + 16)));
	working_code2 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code2, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start + 32)));
	working_code3 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code3, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start + 48)));
	working_code4 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code4, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start + 64)));
	working_code5 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code5, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start + 80)));
	working_code6 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code6, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start + 96)));
	working_code7 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(working_code7, 1), mask_FE), _mm_loadu_si128((__m128i*)(rng_start + 112)));
}

__forceinline void tm_sse2_8_shuffled::alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
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

__forceinline void tm_sse2_8_shuffled::alg_2(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg2_values_128_8 + (*rng_seed * 16)));

	alg_2_sub(working_code6, working_code7, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code4, working_code5, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code2, working_code3, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
	alg_2_sub(working_code0, working_code1, carry, mask_top_01, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_sse2_8_shuffled::alg_3(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16 * rng_seed)
{
	uint8* rng_start = rng->regular_rng_values_128_8_shuffled + ((*rng_seed) * 128);
	working_code0 = _mm_xor_si128(working_code0, _mm_load_si128((__m128i*)rng_start));
	working_code1 = _mm_xor_si128(working_code1, _mm_load_si128((__m128i*)(rng_start + 16)));
	working_code2 = _mm_xor_si128(working_code2, _mm_load_si128((__m128i*)(rng_start + 32)));
	working_code3 = _mm_xor_si128(working_code3, _mm_load_si128((__m128i*)(rng_start + 48)));
	working_code4 = _mm_xor_si128(working_code4, _mm_load_si128((__m128i*)(rng_start + 64)));
	working_code5 = _mm_xor_si128(working_code5, _mm_load_si128((__m128i*)(rng_start + 80)));
	working_code6 = _mm_xor_si128(working_code6, _mm_load_si128((__m128i*)(rng_start + 96)));
	working_code7 = _mm_xor_si128(working_code7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline void tm_sse2_8_shuffled::alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
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

__forceinline void tm_sse2_8_shuffled::alg_5(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg5_values_128_8 + (*rng_seed * 16)));

	alg_5_sub(working_code6, working_code7, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code4, working_code5, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code2, working_code3, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code0, working_code1, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
}

__forceinline void tm_sse2_8_shuffled::alg_6(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_7F)
{
	uint8* rng_start = rng->alg6_values_128_8_shuffled + (*rng_seed * 128);
	working_code0 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code0, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start)));
	working_code1 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code1, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start + 16)));
	working_code2 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code2, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start + 32)));
	working_code3 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code3, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start + 48)));
	working_code4 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code4, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start + 64)));
	working_code5 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code5, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start + 80)));
	working_code6 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code6, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start + 96)));
	working_code7 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(working_code7, 1), mask_7F), _mm_loadu_si128((__m128i*)(rng_start + 112)));
}

__forceinline void tm_sse2_8_shuffled::alg_7(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& mask_FF)
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

__forceinline void tm_sse2_8_shuffled::add_alg(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint8* rng_start, uint16* rng_seed)
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

void tm_sse2_8_shuffled::run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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
		unsigned char current_byte = ((uint8*)working_code_data)[i];

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char alg_id = (current_byte >> 1) & 0x07;

		run_alg(alg_id, &rng_seed, 1);
	}
}

void tm_sse2_8_shuffled::run_all_maps(const key_schedule& schedule_entries)
{
	__m128i working_code0 = _mm_loadu_si128((__m128i*)(working_code_data));
	__m128i working_code1 = _mm_loadu_si128((__m128i*)(working_code_data + 16));
	__m128i working_code2 = _mm_loadu_si128((__m128i*)(working_code_data + 32));
	__m128i working_code3 = _mm_loadu_si128((__m128i*)(working_code_data + 48));
	__m128i working_code4 = _mm_loadu_si128((__m128i*)(working_code_data + 64));
	__m128i working_code5 = _mm_loadu_si128((__m128i*)(working_code_data + 80));
	__m128i working_code6 = _mm_loadu_si128((__m128i*)(working_code_data + 96));
	__m128i working_code7 = _mm_loadu_si128((__m128i*)(working_code_data + 112));

	__m128i mask_FF = _mm_set1_epi8(0xFF);
	__m128i mask_FE = _mm_set1_epi8(0xFE);
	__m128i mask_7F = _mm_set1_epi8(0x7F);
	__m128i mask_01 = _mm_set1_epi8(0x01);
	__m128i mask_80 = _mm_set1_epi8(0x80);
	__m128i mask_top_01 = _mm_set_epi8(0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m128i mask_top_80 = _mm_set_epi8(0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries.entries.begin(); it != schedule_entries.entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

		uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
		uint16 nibble_selector = schedule_entry.nibble_selector;

		// Next, the working code is processed with the same steps 16 times:
		for (int i = 0; i < 16; i++)
		{
			// Get the highest bit of the nibble selector to use as a flag
			unsigned char nibble = (nibble_selector >> 15) & 0x01;
			// Shift the nibble selector up one bit
			nibble_selector = nibble_selector << 1;

			_mm_store_si128((__m128i*)(working_code_data), working_code0);
			_mm_store_si128((__m128i*)(working_code_data + 16), working_code1);

			// If the flag is a 1, get the high nibble of the current byte
			// Otherwise use the low nibble
			unsigned char current_byte = ((uint8*)working_code_data)[shuffle_128(i)];

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
			else if (algorithm_id == 1)
			{
				add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng->regular_rng_values_128_8_shuffled, &rng_seed);
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
			else if (algorithm_id == 4)
			{
				add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng->alg4_values_128_8_shuffled, &rng_seed);
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

	_mm_store_si128((__m128i*)(working_code_data), working_code0);
	_mm_store_si128((__m128i*)(working_code_data + 16), working_code1);
	_mm_store_si128((__m128i*)(working_code_data + 32), working_code2);
	_mm_store_si128((__m128i*)(working_code_data + 48), working_code3);
	_mm_store_si128((__m128i*)(working_code_data + 64), working_code4);
	_mm_store_si128((__m128i*)(working_code_data + 80), working_code5);
	_mm_store_si128((__m128i*)(working_code_data + 96), working_code6);
	_mm_store_si128((__m128i*)(working_code_data + 112), working_code7);
}


bool tm_sse2_8_shuffled::initialized = false;