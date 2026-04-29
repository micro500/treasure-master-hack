#include "tm_avx_r128s_8.h"

tm_avx_r128s_8::tm_avx_r128s_8(RNG* rng_obj) : tm_avx_r128s_8(rng_obj, 0) {}

tm_avx_r128s_8::tm_avx_r128s_8(RNG* rng_obj, const uint32_t key) : tm_avx_r128s_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx_r128s_8::tm_avx_r128s_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
	: TM_base(rng_obj),
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

	shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 128, false);
	shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 128, false);

	shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 128, false);
	shuffle_mem(other_world_data, other_world_data_shuffled, 128, false);
}

__forceinline void tm_avx_r128s_8::initialize()
{
	if (!_initialized)
	{
		auto _r0 = rng->generate_expansion_values_128_8_shuffled();
		auto _r1 = rng->generate_seed_forward_1();
		auto _r2 = rng->generate_seed_forward_128();
		auto _r3 = rng->generate_regular_rng_values_8();
		auto _r4 = rng->generate_regular_rng_values_128_8_shuffled();
		auto _r5 = rng->generate_alg0_values_128_8_shuffled();
		auto _r6 = rng->generate_alg2_values_8_8();
		auto _r7 = rng->generate_alg4_values_128_8_shuffled();
		auto _r8 = rng->generate_alg5_values_8_8();
		auto _r9 = rng->generate_alg6_values_128_8_shuffled();

		_expansion_128s = static_cast<uint8_t*>(_r0.get());
		_seed_fwd_1     = static_cast<uint16_t*>(_r1.get());
		_seed_fwd_128   = static_cast<uint16_t*>(_r2.get());
		_regular_8      = static_cast<uint8_t*>(_r3.get());
		_regular_128s   = static_cast<uint8_t*>(_r4.get());
		_alg0_128s      = static_cast<uint8_t*>(_r5.get());
		_alg2_8_8       = static_cast<uint8_t*>(_r6.get());
		_alg4_128s      = static_cast<uint8_t*>(_r7.get());
		_alg5_8_8       = static_cast<uint8_t*>(_r8.get());
		_alg6_128s      = static_cast<uint8_t*>(_r9.get());

		_table_refs = { _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7, _r8, _r9 };
		_initialized = true;
	}
	obj_name = "tm_avx_r128s_8";
}

__forceinline void tm_avx_r128s_8::_load_from_mem(WC_ARGS_128)
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

__forceinline void tm_avx_r128s_8::_store_to_mem(WC_ARGS_128)
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

__forceinline void tm_avx_r128s_8::_expand_code(uint32_t data, WC_ARGS_128)
{
	__m128i a = _mm_insert_epi32(_mm_cvtsi32_si128(static_cast<int32_t>(data)), static_cast<int32_t>(key), 1);
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

	uint8_t* rng_start = _expansion_128s;
	uint16_t rng_seed = (key >> 16) & 0xFFFF;

	add_alg(WC_PASS_128, &rng_seed, rng_start);
}

void tm_avx_r128s_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8_t*)working_code_data)[shuffle_8(i, 128)] = new_data[i];
	}
}

void tm_avx_r128s_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8_t*)working_code_data)[shuffle_8(i, 128)];
	}
}

__forceinline void tm_avx_r128s_8::_run_alg(WC_ARGS_128, int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_128, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		uint8_t* rng_start = _regular_128s;

		if (algorithm_id == 4)
		{
			rng_start = _alg4_128s;
		}

		add_alg(WC_PASS_128, rng_seed, rng_start);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS_128, rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS_128, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS_128, rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS_128, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS_128);
	}
}

__forceinline void tm_avx_r128s_8::alg_0(WC_ARGS_128, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg0_128s + ((*rng_seed) * 128);

	alg_0_sub(wc0, rng_start);
	alg_0_sub(wc1, rng_start + 16);
	alg_0_sub(wc2, rng_start + 32);
	alg_0_sub(wc3, rng_start + 48);
	alg_0_sub(wc4, rng_start + 64);
	alg_0_sub(wc5, rng_start + 80);
	alg_0_sub(wc6, rng_start + 96);
	alg_0_sub(wc7, rng_start + 112);
}

__forceinline void tm_avx_r128s_8::alg_0_sub(__m128i& working_code, uint8_t* rng_start)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_slli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_FE);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry)
{
	__m128i temp1 = _mm_srli_epi16(working_a, 1);
	__m128i cur_val1_most = _mm_and_si128(temp1, mask_7F);

	__m128i cur_val2_masked = _mm_and_si128(working_b, mask_80);

	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, mask_01);
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	__m128i temp2 = _mm_slli_epi16(working_b, 1);
	__m128i cur_val2_most = _mm_and_si128(temp2, mask_FE);

	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), mask_top_01);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);

	carry = next_carry;
}

__forceinline void tm_avx_r128s_8::alg_2(WC_ARGS_128, uint16_t* rng_seed)
{
	__m128i carry = _mm_and_si128(
		_mm_set1_epi8(static_cast<int8_t>(_alg2_8_8[*rng_seed])),
		mask_top_01);

	alg_2_sub(wc6, wc7, carry);
	alg_2_sub(wc4, wc5, carry);
	alg_2_sub(wc2, wc3, carry);
	alg_2_sub(wc0, wc1, carry);
}

__forceinline void tm_avx_r128s_8::alg_3(WC_ARGS_128, uint16_t* rng_seed)
{
	uint8_t* rng_start = _regular_128s + ((*rng_seed) * 128);

	xor_alg(WC_PASS_128, rng_start);
}

__forceinline void tm_avx_r128s_8::xor_alg(WC_ARGS_128, uint8_t* values)
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
	__m128i temp1 = _mm_slli_epi16(working_a, 1);
	__m128i cur_val1_most = _mm_and_si128(temp1, mask_FE);

	__m128i cur_val2_masked = _mm_and_si128(working_b, mask_01);

	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, mask_80);
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	__m128i temp2 = _mm_srli_epi16(working_b, 1);
	__m128i cur_val2_most = _mm_and_si128(temp2, mask_7F);

	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), mask_top_80);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);

	carry = next_carry;
}

__forceinline void tm_avx_r128s_8::alg_5(WC_ARGS_128, uint16_t* rng_seed)
{
	__m128i carry = _mm_and_si128(
		_mm_set1_epi8(static_cast<int8_t>(_alg5_8_8[*rng_seed])),
		mask_top_80);

	alg_5_sub(wc6, wc7, carry);
	alg_5_sub(wc4, wc5, carry);
	alg_5_sub(wc2, wc3, carry);
	alg_5_sub(wc0, wc1, carry);
}

__forceinline void tm_avx_r128s_8::alg_6(WC_ARGS_128, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg6_128s + ((*rng_seed) * 128);

	alg_6_sub(wc0, rng_start);
	alg_6_sub(wc1, rng_start + 16);
	alg_6_sub(wc2, rng_start + 32);
	alg_6_sub(wc3, rng_start + 48);
	alg_6_sub(wc4, rng_start + 64);
	alg_6_sub(wc5, rng_start + 80);
	alg_6_sub(wc6, rng_start + 96);
	alg_6_sub(wc7, rng_start + 112);
}

__forceinline void tm_avx_r128s_8::alg_6_sub(__m128i& working_code, uint8_t* rng_start)
{
	__m128i rng_val = _mm_load_si128((__m128i*)(rng_start));
	working_code = _mm_srli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_7F);
	working_code = _mm_or_si128(working_code, rng_val);
}

__forceinline void tm_avx_r128s_8::alg_7(WC_ARGS_128)
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

__forceinline void tm_avx_r128s_8::add_alg(WC_ARGS_128, uint16_t* rng_seed, uint8_t* rng_start)
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

__forceinline void tm_avx_r128s_8::_run_one_map(WC_ARGS_128, const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16_t rng_seed = static_cast<uint16_t>((schedule_entry.rng1 << 8) | schedule_entry.rng2);
	uint16_t nibble_selector = schedule_entry.nibble_selector;

	for (int i = 0; i < 16; i++)
	{
		// Shuffled layout: byte i may live in wc0 or wc1 depending on shuffle_8(i, 128)
		_mm_store_si128((__m128i*)(working_code_data), wc0);
		_mm_store_si128((__m128i*)(working_code_data + 16), wc1);

		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);

		uint8_t current_byte = static_cast<uint8_t>(static_cast<uint8_t*>(working_code_data)[shuffle_8(i, 128)]);
		if (nibble == 1)
		{
			current_byte = static_cast<uint8_t>(current_byte >> 4);
		}

		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);
		_run_alg(WC_PASS_128, algorithm_id, &rng_seed);
	}
}

__forceinline void tm_avx_r128s_8::_run_all_maps(WC_ARGS_128)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries->entries.begin(); it != schedule_entries->entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;
		_run_one_map(WC_PASS_128, schedule_entry);
	}
}

__forceinline void tm_avx_r128s_8::_decrypt_carnival_world(WC_ARGS_128)
{
	xor_alg(WC_PASS_128, carnival_world_data_shuffled);
}

__forceinline void tm_avx_r128s_8::_decrypt_other_world(WC_ARGS_128)
{
	xor_alg(WC_PASS_128, other_world_data_shuffled);
}

__forceinline void tm_avx_r128s_8::mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask)
{
	__m128i temp_masked = _mm_and_si128(working_code, sum_mask);

	__m128i temp1_lo_lo = _mm_and_si128(temp_masked, lo_mask);
	__m128i temp1_lo_hi = _mm_srli_epi16(temp_masked, 8);

	sum = _mm_add_epi16(sum, temp1_lo_lo);
	sum = _mm_add_epi16(sum, temp1_lo_hi);
}

__forceinline uint16_t tm_avx_r128s_8::masked_checksum(WC_ARGS_128, uint8_t* mask)
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

__forceinline uint16_t tm_avx_r128s_8::_calculate_carnival_world_checksum(WC_ARGS_128)
{
	return masked_checksum(WC_PASS_128, carnival_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx_r128s_8::_calculate_other_world_checksum(WC_ARGS_128)
{
	return masked_checksum(WC_PASS_128, other_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx_r128s_8::fetch_checksum_value(WC_ARGS_128, uint8_t code_length)
{
	// Store wc0..wc3 (covers shuffled bytes 0..63, which contain both checksum positions)
	_mm_store_si128((__m128i*)(working_code_data), wc0);
	_mm_store_si128((__m128i*)(working_code_data + 16), wc1);
	_mm_store_si128((__m128i*)(working_code_data + 32), wc2);
	_mm_store_si128((__m128i*)(working_code_data + 48), wc3);

	uint8_t checksum_low = static_cast<uint8_t>(working_code_data[shuffle_8(127 - code_length, 128)]);
	uint8_t checksum_hi = static_cast<uint8_t>(working_code_data[shuffle_8(127 - (code_length + 1), 128)]);
	return static_cast<uint16_t>((checksum_hi << 8) | checksum_low);
}

__forceinline uint16_t tm_avx_r128s_8::_fetch_carnival_world_checksum_value(WC_ARGS_128)
{
	return fetch_checksum_value(WC_PASS_128, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx_r128s_8::_fetch_other_world_checksum_value(WC_ARGS_128)
{
	return fetch_checksum_value(WC_PASS_128, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx_r128s_8::check_carnival_world_checksum(WC_ARGS_128)
{
	return _calculate_carnival_world_checksum(WC_PASS_128) == _fetch_carnival_world_checksum_value(WC_PASS_128);
}

__forceinline bool tm_avx_r128s_8::check_other_world_checksum(WC_ARGS_128)
{
	return _calculate_other_world_checksum(WC_PASS_128) == _fetch_other_world_checksum_value(WC_PASS_128);
}

template<bool CHECK_CHECKSUM, int WORLD>
__forceinline std::optional<uint8_t> tm_avx_r128s_8::_decrypt_check(WC_ARGS_128)
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
	uint8_t unshuffled_data[128];
	unshuffle_mem(working_code_data, unshuffled_data, 128, false);
	return check_machine_code(unshuffled_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx_r128s_8::_run_bruteforce(WC_ARGS_128, uint32_t data, uint8_t* result_data, uint32_t* result_size)
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

void tm_avx_r128s_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
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

void tm_avx_r128s_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_128;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_128, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_avx_r128s_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	WC_VARS_128;
	load_data(data);
	_load_from_mem(WC_PASS_128);
	_run_alg(WC_PASS_128, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_128);
	fetch_data(data);
}

void tm_avx_r128s_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_128;
	load_data(data);
	_load_from_mem(WC_PASS_128);
	for (int i = 0; i < iterations; i++)
		_run_alg(WC_PASS_128, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_128);
	fetch_data(data);
}

void tm_avx_r128s_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_128;
	_expand_code(data, WC_PASS_128);
	_store_to_mem(WC_PASS_128);
	fetch_data(result_out);
}

void tm_avx_r128s_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_128;
	_expand_code(data, WC_PASS_128);
	_run_all_maps(WC_PASS_128);
	_store_to_mem(WC_PASS_128);
	fetch_data(result_out);
}

bool tm_avx_r128s_8::test_bruteforce_checksum(uint32_t data, int world)
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

