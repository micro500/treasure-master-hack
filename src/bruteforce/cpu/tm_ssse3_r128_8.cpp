#include "tm_ssse3_r128_8.h"

tm_ssse3_r128_8::tm_ssse3_r128_8(RNG* rng_obj) : tm_ssse3_r128_8(rng_obj, 0) {}

tm_ssse3_r128_8::tm_ssse3_r128_8(RNG* rng_obj, const uint32_t key) : tm_ssse3_r128_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_ssse3_r128_8::tm_ssse3_r128_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj),
	mask_FF(_mm_set1_epi8(static_cast<int8_t>(0xFF))),
	mask_FE(_mm_set1_epi8(static_cast<int8_t>(0xFE))),
	mask_7F(_mm_set1_epi8(0x7F)),
	mask_80(_mm_set1_epi8(static_cast<int8_t>(0x80))),
	mask_01(_mm_set1_epi8(0x01)),
	mask_top_01(_mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0)),
	mask_top_80(_mm_set_epi16(static_cast<int16_t>(0x8000), 0, 0, 0, 0, 0, 0, 0))
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;
}

__forceinline void tm_ssse3_r128_8::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();

		rng->generate_alg0_values_8();
		rng->generate_alg2_values_8_8();
		rng->generate_alg4_values_8();
		rng->generate_alg5_values_8_8();
		rng->generate_alg6_values_8();

		initialized = true;
	}
	obj_name = "tm_ssse3_r128_8";
}

__forceinline void tm_ssse3_r128_8::_load_from_mem(WC_ARGS_128)
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

__forceinline void tm_ssse3_r128_8::_store_to_mem(WC_ARGS_128)
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

__forceinline void tm_ssse3_r128_8::_expand_code(uint32_t data, WC_ARGS_128)
{
	uint64_t x = ((uint64_t)key << 32) | data;
	__m128i a = _mm_cvtsi64_si128(static_cast<int64_t>(x));
	__m128i nat_mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7);
	__m128i pattern = _mm_shuffle_epi8(a, nat_mask);

	wc0 = pattern;
	wc1 = pattern;
	wc2 = pattern;
	wc3 = pattern;
	wc4 = pattern;
	wc5 = pattern;
	wc6 = pattern;
	wc7 = pattern;

	uint8_t* rng_start = rng->expansion_values_8;
	uint16_t rng_seed = (key >> 16) & 0xFFFF;

	add_alg(WC_PASS_128, &rng_seed, rng_start);
}

__forceinline void tm_ssse3_r128_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = new_data[i];
	}
}

__forceinline void tm_ssse3_r128_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = working_code_data[i];
	}
}

__forceinline void tm_ssse3_r128_8::_run_alg(WC_ARGS_128, int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_128, rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		uint8_t* rng_start = rng->regular_rng_values_8;

		if (algorithm_id == 4)
		{
			rng_start = rng->alg4_values_8;
		}

		add_alg(WC_PASS_128, rng_seed, rng_start);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS_128, rng_seed);
		*rng_seed = rng->seed_forward_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS_128, rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS_128, rng_seed);
		*rng_seed = rng->seed_forward_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS_128, rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS_128);
	}
}

__forceinline void tm_ssse3_r128_8::alg_0(WC_ARGS_128, uint16_t* rng_seed)
{
	uint8_t* rng_start = rng->alg0_values_8 + (*rng_seed) * 128;

	wc0 = _mm_slli_epi16(wc0, 1);
	wc0 = _mm_and_si128(wc0, mask_FE);
	wc0 = _mm_or_si128(wc0, _mm_load_si128((__m128i*)(rng_start)));

	wc1 = _mm_slli_epi16(wc1, 1);
	wc1 = _mm_and_si128(wc1, mask_FE);
	wc1 = _mm_or_si128(wc1, _mm_load_si128((__m128i*)(rng_start + 16)));

	wc2 = _mm_slli_epi16(wc2, 1);
	wc2 = _mm_and_si128(wc2, mask_FE);
	wc2 = _mm_or_si128(wc2, _mm_load_si128((__m128i*)(rng_start + 32)));

	wc3 = _mm_slli_epi16(wc3, 1);
	wc3 = _mm_and_si128(wc3, mask_FE);
	wc3 = _mm_or_si128(wc3, _mm_load_si128((__m128i*)(rng_start + 48)));

	wc4 = _mm_slli_epi16(wc4, 1);
	wc4 = _mm_and_si128(wc4, mask_FE);
	wc4 = _mm_or_si128(wc4, _mm_load_si128((__m128i*)(rng_start + 64)));

	wc5 = _mm_slli_epi16(wc5, 1);
	wc5 = _mm_and_si128(wc5, mask_FE);
	wc5 = _mm_or_si128(wc5, _mm_load_si128((__m128i*)(rng_start + 80)));

	wc6 = _mm_slli_epi16(wc6, 1);
	wc6 = _mm_and_si128(wc6, mask_FE);
	wc6 = _mm_or_si128(wc6, _mm_load_si128((__m128i*)(rng_start + 96)));

	wc7 = _mm_slli_epi16(wc7, 1);
	wc7 = _mm_and_si128(wc7, mask_FE);
	wc7 = _mm_or_si128(wc7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline void tm_ssse3_r128_8::alg_2_sub(__m128i& wc, __m128i& carry)
{
	__m128i part_lo = _mm_and_si128(_mm_srli_epi16(wc, 1), _mm_set1_epi16(0x007F));
	__m128i part_hi_bit = _mm_and_si128(_mm_srli_epi16(wc, 8), _mm_set1_epi16(0x0080));
	__m128i new_lo = _mm_or_si128(part_lo, part_hi_bit);

	__m128i low_bit0 = _mm_and_si128(wc, _mm_set1_epi16(0x0001));
	__m128i carry_in = _mm_or_si128(
		_mm_srli_si128(_mm_slli_epi16(low_bit0, 8), 2),
		carry);

	__m128i new_hi = _mm_or_si128(
		_mm_and_si128(_mm_slli_epi16(wc, 1), _mm_set1_epi16(static_cast<int16_t>(0xFE00))),
		carry_in);

	carry = _mm_and_si128(
		_mm_slli_si128(_mm_and_si128(wc, _mm_set1_epi16(0x0001)), 15),
		mask_top_01);

	wc = _mm_or_si128(
		_mm_and_si128(new_lo, _mm_set1_epi16(0x00FF)),
		_mm_and_si128(new_hi, _mm_set1_epi16(static_cast<int16_t>(0xFF00))));
}

__forceinline void tm_ssse3_r128_8::alg_2(WC_ARGS_128, uint16_t* rng_seed)
{
	__m128i carry = _mm_and_si128(
		_mm_set1_epi8(static_cast<int8_t>(rng->alg2_values_8_8[*rng_seed])),
		mask_top_01);

	alg_2_sub(wc7, carry);
	alg_2_sub(wc6, carry);
	alg_2_sub(wc5, carry);
	alg_2_sub(wc4, carry);
	alg_2_sub(wc3, carry);
	alg_2_sub(wc2, carry);
	alg_2_sub(wc1, carry);
	alg_2_sub(wc0, carry);
}

__forceinline void tm_ssse3_r128_8::alg_3(WC_ARGS_128, uint16_t* rng_seed)
{
	uint8_t* rng_start = rng->regular_rng_values_8 + (*rng_seed) * 128;
	xor_alg(WC_PASS_128, rng_start);
}

__forceinline void tm_ssse3_r128_8::alg_5_sub(__m128i& wc, __m128i& carry)
{
	__m128i part_lo = _mm_and_si128(_mm_slli_epi16(wc, 1), _mm_set1_epi16(0x00FE));
	__m128i part_lo_bit = _mm_and_si128(_mm_srli_epi16(wc, 8), _mm_set1_epi16(0x0001));
	__m128i new_lo = _mm_or_si128(part_lo, part_lo_bit);

	__m128i low_bit7 = _mm_and_si128(wc, _mm_set1_epi16(0x0080));
	__m128i carry_in = _mm_or_si128(
		_mm_srli_si128(_mm_slli_epi16(low_bit7, 8), 2),
		carry);

	__m128i new_hi = _mm_or_si128(
		_mm_and_si128(_mm_srli_epi16(wc, 1), _mm_set1_epi16(0x7F00)),
		carry_in);

	carry = _mm_and_si128(
		_mm_slli_si128(_mm_and_si128(wc, _mm_set1_epi16(0x0080)), 15),
		mask_top_80);

	wc = _mm_or_si128(
		_mm_and_si128(new_lo, _mm_set1_epi16(0x00FF)),
		_mm_and_si128(new_hi, _mm_set1_epi16(static_cast<int16_t>(0xFF00))));
}

__forceinline void tm_ssse3_r128_8::alg_5(WC_ARGS_128, uint16_t* rng_seed)
{
	__m128i carry = _mm_and_si128(
		_mm_set1_epi8(static_cast<int8_t>(rng->alg5_values_8_8[*rng_seed])),
		mask_top_80);

	alg_5_sub(wc7, carry);
	alg_5_sub(wc6, carry);
	alg_5_sub(wc5, carry);
	alg_5_sub(wc4, carry);
	alg_5_sub(wc3, carry);
	alg_5_sub(wc2, carry);
	alg_5_sub(wc1, carry);
	alg_5_sub(wc0, carry);
}

__forceinline void tm_ssse3_r128_8::alg_6(WC_ARGS_128, uint16_t* rng_seed)
{
	uint8_t* rng_start = rng->alg6_values_8 + (*rng_seed) * 128;

	wc0 = _mm_srli_epi16(wc0, 1);
	wc0 = _mm_and_si128(wc0, mask_7F);
	wc0 = _mm_or_si128(wc0, _mm_load_si128((__m128i*)(rng_start)));

	wc1 = _mm_srli_epi16(wc1, 1);
	wc1 = _mm_and_si128(wc1, mask_7F);
	wc1 = _mm_or_si128(wc1, _mm_load_si128((__m128i*)(rng_start + 16)));

	wc2 = _mm_srli_epi16(wc2, 1);
	wc2 = _mm_and_si128(wc2, mask_7F);
	wc2 = _mm_or_si128(wc2, _mm_load_si128((__m128i*)(rng_start + 32)));

	wc3 = _mm_srli_epi16(wc3, 1);
	wc3 = _mm_and_si128(wc3, mask_7F);
	wc3 = _mm_or_si128(wc3, _mm_load_si128((__m128i*)(rng_start + 48)));

	wc4 = _mm_srli_epi16(wc4, 1);
	wc4 = _mm_and_si128(wc4, mask_7F);
	wc4 = _mm_or_si128(wc4, _mm_load_si128((__m128i*)(rng_start + 64)));

	wc5 = _mm_srli_epi16(wc5, 1);
	wc5 = _mm_and_si128(wc5, mask_7F);
	wc5 = _mm_or_si128(wc5, _mm_load_si128((__m128i*)(rng_start + 80)));

	wc6 = _mm_srli_epi16(wc6, 1);
	wc6 = _mm_and_si128(wc6, mask_7F);
	wc6 = _mm_or_si128(wc6, _mm_load_si128((__m128i*)(rng_start + 96)));

	wc7 = _mm_srli_epi16(wc7, 1);
	wc7 = _mm_and_si128(wc7, mask_7F);
	wc7 = _mm_or_si128(wc7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline void tm_ssse3_r128_8::alg_7(WC_ARGS_128)
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

__forceinline void tm_ssse3_r128_8::add_alg(WC_ARGS_128, uint16_t* rng_seed, uint8_t* rng_start)
{
	rng_start = rng_start + (*rng_seed) * 128;
	wc0 = _mm_add_epi8(wc0, _mm_load_si128((__m128i*)(rng_start)));
	wc1 = _mm_add_epi8(wc1, _mm_load_si128((__m128i*)(rng_start + 16)));
	wc2 = _mm_add_epi8(wc2, _mm_load_si128((__m128i*)(rng_start + 32)));
	wc3 = _mm_add_epi8(wc3, _mm_load_si128((__m128i*)(rng_start + 48)));
	wc4 = _mm_add_epi8(wc4, _mm_load_si128((__m128i*)(rng_start + 64)));
	wc5 = _mm_add_epi8(wc5, _mm_load_si128((__m128i*)(rng_start + 80)));
	wc6 = _mm_add_epi8(wc6, _mm_load_si128((__m128i*)(rng_start + 96)));
	wc7 = _mm_add_epi8(wc7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline void tm_ssse3_r128_8::xor_alg(WC_ARGS_128, uint8_t* values)
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

__forceinline void tm_ssse3_r128_8::_run_one_map(WC_ARGS_128, const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16_t rng_seed = static_cast<uint16_t>((schedule_entry.rng1 << 8) | schedule_entry.rng2);
	uint16_t nibble_selector = schedule_entry.nibble_selector;

	for (int i = 0; i < 16; i++)
	{
		// Natural layout: bytes 0..15 are in wc0, so store only wc0 to read byte i
		_mm_store_si128((__m128i*)(working_code_data), wc0);

		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);

		uint8_t current_byte = working_code_data[i];

		if (nibble == 1)
		{
			current_byte = static_cast<uint8_t>(current_byte >> 4);
		}

		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(WC_PASS_128, algorithm_id, &rng_seed);
	}
}

__forceinline void tm_ssse3_r128_8::_run_all_maps(WC_ARGS_128)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries->entries.begin(); it != schedule_entries->entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;
		_run_one_map(WC_PASS_128, schedule_entry);
	}
}

__forceinline void tm_ssse3_r128_8::_decrypt_carnival_world(WC_ARGS_128)
{
	xor_alg(WC_PASS_128, carnival_world_data);
}

__forceinline void tm_ssse3_r128_8::_decrypt_other_world(WC_ARGS_128)
{
	xor_alg(WC_PASS_128, other_world_data);
}

__forceinline void tm_ssse3_r128_8::mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask)
{
	__m128i temp_masked = _mm_and_si128(working_code, sum_mask);

	__m128i temp1_lo_lo = _mm_and_si128(temp_masked, lo_mask);
	__m128i temp1_lo_hi = _mm_srli_epi16(temp_masked, 8);

	sum = _mm_add_epi16(sum, temp1_lo_lo);
	sum = _mm_add_epi16(sum, temp1_lo_hi);
}

__forceinline uint16_t tm_ssse3_r128_8::masked_checksum(WC_ARGS_128, uint8_t* mask)
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

	int code_sum = _mm_extract_epi16(sum, 0) +
		_mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) +
		_mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) +
		_mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) +
		_mm_extract_epi16(sum, 7);

	return static_cast<uint16_t>(code_sum);
}

__forceinline uint16_t tm_ssse3_r128_8::_calculate_carnival_world_checksum(WC_ARGS_128)
{
	return masked_checksum(WC_PASS_128, carnival_world_checksum_mask);
}

__forceinline uint16_t tm_ssse3_r128_8::_calculate_other_world_checksum(WC_ARGS_128)
{
	return masked_checksum(WC_PASS_128, other_world_checksum_mask);
}

__forceinline uint16_t tm_ssse3_r128_8::fetch_checksum_value(WC_ARGS_128, uint8_t code_length)
{
	_mm_store_si128((__m128i*)(working_code_data), wc0);
	_mm_store_si128((__m128i*)(working_code_data + 16), wc1);
	_mm_store_si128((__m128i*)(working_code_data + 32), wc2);
	_mm_store_si128((__m128i*)(working_code_data + 48), wc3);

	unsigned char checksum_low = working_code_data[127 - code_length];
	unsigned char checksum_hi = working_code_data[127 - (code_length + 1)];
	return static_cast<uint16_t>((checksum_hi << 8) | checksum_low);
}

__forceinline uint16_t tm_ssse3_r128_8::_fetch_carnival_world_checksum_value(WC_ARGS_128)
{
	return fetch_checksum_value(WC_PASS_128, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_ssse3_r128_8::_fetch_other_world_checksum_value(WC_ARGS_128)
{
	return fetch_checksum_value(WC_PASS_128, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_ssse3_r128_8::check_carnival_world_checksum(WC_ARGS_128)
{
	return _calculate_carnival_world_checksum(WC_PASS_128) == _fetch_carnival_world_checksum_value(WC_PASS_128);
}

__forceinline bool tm_ssse3_r128_8::check_other_world_checksum(WC_ARGS_128)
{
	return _calculate_other_world_checksum(WC_PASS_128) == _fetch_other_world_checksum_value(WC_PASS_128);
}

template<bool CHECK_CHECKSUM, int WORLD>
__forceinline std::optional<uint8_t> tm_ssse3_r128_8::_decrypt_check(WC_ARGS_128)
{
	WC_XOR_VARS_128;
	WC_COPY_XOR_128;
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(WC_XOR_PASS_128);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_carnival_world_checksum(WC_XOR_PASS_128))
			{
				return std::nullopt;
			}
		}
	}
	else
	{
		_decrypt_other_world(WC_XOR_PASS_128);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum(WC_XOR_PASS_128))
			{
				return std::nullopt;
			}
		}
	}

	_store_to_mem(WC_XOR_PASS_128);
	return check_machine_code(working_code_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_ssse3_r128_8::_run_bruteforce(WC_ARGS_128, uint32_t data, uint8_t* result_data, uint32_t* result_size)
{
	_expand_code(data, WC_PASS_128);

	_run_all_maps(WC_PASS_128);

	auto carnival_flags = _decrypt_check<CHECK_CHECKSUMS, CARNIVAL_WORLD>(WC_PASS_128);
	if constexpr (CHECK_CHECKSUMS)
	{
		if (carnival_flags.has_value())
		{
			*((uint32_t*)(&result_data[*result_size])) = data;
			result_data[*result_size + 4] = *carnival_flags;
			*result_size += 5;

			return;
		}
	}

	auto other_flags = _decrypt_check<CHECK_CHECKSUMS, OTHER_WORLD>(WC_PASS_128);
	if constexpr (CHECK_CHECKSUMS)
	{
		if (other_flags.has_value())
		{
			*((uint32_t*)(&result_data[*result_size])) = data;
			result_data[*result_size + 4] = *other_flags;
			*result_size += 5;

			return;
		}
	}
	else
	{
		result_data[*result_size] = carnival_flags.value();
		result_data[*result_size + 1] = other_flags.value();
		*result_size += 2;
	}
}

void tm_ssse3_r128_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
{
	WC_VARS_128;

	for (uint32_t i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
		{
			return;
		}
		uint32_t data = start_data + i;

		_run_bruteforce<true>(WC_PASS_128, data, result_data, result_size);

		report_progress(static_cast<double>(i + 1) / static_cast<double>(amount_to_run));
	}
}

void tm_ssse3_r128_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_128;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_128, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_ssse3_r128_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	WC_VARS_128;
	load_data(data);
	_load_from_mem(WC_PASS_128);
	_run_alg(WC_PASS_128, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_128);
	fetch_data(data);
}

void tm_ssse3_r128_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_128;
	load_data(data);
	_load_from_mem(WC_PASS_128);
	for (int i = 0; i < iterations; i++)
		_run_alg(WC_PASS_128, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_128);
	fetch_data(data);
}

void tm_ssse3_r128_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_128;
	_expand_code(data, WC_PASS_128);
	_store_to_mem(WC_PASS_128);
	fetch_data(result_out);
}

void tm_ssse3_r128_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_128;
	_expand_code(data, WC_PASS_128);
	_run_all_maps(WC_PASS_128);
	_store_to_mem(WC_PASS_128);
	fetch_data(result_out);
}

bool tm_ssse3_r128_8::test_bruteforce_checksum(uint32_t data, int world)
{
	WC_VARS_128;
	_expand_code(data, WC_PASS_128);
	_run_all_maps(WC_PASS_128);

	if (world == CARNIVAL_WORLD)
	{
		return _decrypt_check<true, CARNIVAL_WORLD>(WC_PASS_128).has_value();
	}
	else
	{
		return _decrypt_check<true, OTHER_WORLD>(WC_PASS_128).has_value();
	}
}

bool tm_ssse3_r128_8::initialized = false;