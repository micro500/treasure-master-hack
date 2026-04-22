#include "tm_avx2_r256s_map_8.h"

tm_avx2_r256s_map_8::tm_avx2_r256s_map_8(RNG* rng_obj) : tm_avx2_r256s_map_8(rng_obj, 0) {}

tm_avx2_r256s_map_8::tm_avx2_r256s_map_8(RNG* rng_obj, const uint32_t key) : tm_avx2_r256s_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx2_r256s_map_8::tm_avx2_r256s_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
	: TM_base(rng_obj),
	  mask_FF(_mm256_set1_epi8(static_cast<int8_t>(0xFF))),
	  mask_FE(_mm256_set1_epi8(static_cast<int8_t>(0xFE))),
	  mask_7F(_mm256_set1_epi8(0x7F)),
	  mask_80(_mm256_set1_epi8(static_cast<int8_t>(0x80))),
	  mask_01(_mm256_set1_epi8(0x01)),
	  mask_top_01(_mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
	  mask_top_80(_mm256_set_epi16(static_cast<int16_t>(0x8000), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;

	shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 256, false);
	shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 256, false);
	shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 256, false);
	shuffle_mem(other_world_data, other_world_data_shuffled, 256, false);

	expansion_values_for_seed_256_8_shuffled = nullptr;
	regular_rng_values_for_seeds_8 = nullptr;
	alg0_values_for_seeds_8 = nullptr;
	alg6_values_for_seeds_8 = nullptr;

	generate_map_rng();
}

tm_avx2_r256s_map_8::~tm_avx2_r256s_map_8()
{
}

__forceinline void tm_avx2_r256s_map_8::initialize()
{
	if (!initialized)
	{
		initialized = true;
	}
	obj_name = "tm_avx2_r256s_map_8";
}

void tm_avx2_r256s_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(&expansion_values_for_seed_256_8_shuffled, (key >> 16) & 0xFFFF, true, 256);
	rng->generate_regular_rng_values_for_seeds_8(&regular_rng_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg0_values_for_seeds_8(&alg0_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
}

__forceinline void tm_avx2_r256s_map_8::_load_from_mem(WC_ARGS_256)
{
	wc0 = _mm256_load_si256((__m256i*)(working_code_data));
	wc1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	wc2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	wc3 = _mm256_load_si256((__m256i*)(working_code_data + 96));
}

__forceinline void tm_avx2_r256s_map_8::_store_to_mem(WC_ARGS_256)
{
	_mm256_store_si256((__m256i*)(working_code_data),      wc0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), wc1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), wc2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), wc3);
}

void tm_avx2_r256s_map_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		working_code_data[shuffle_8(i, 256)] = new_data[i];
}

void tm_avx2_r256s_map_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		new_data[i] = working_code_data[shuffle_8(i, 256)];
}

__forceinline void tm_avx2_r256s_map_8::_expand_code(uint32_t data, WC_ARGS_256)
{
	uint64_t x = ((uint64_t)key << 32) | data;
	__m128i a = _mm_cvtsi64_si128(static_cast<int64_t>(x));
	__m128i lo_mask = _mm_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m128i hi_mask = _mm_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);
	__m128i lo_128 = _mm_shuffle_epi8(a, lo_mask);
	__m128i hi_128 = _mm_shuffle_epi8(a, hi_mask);
	__m256i lo = _mm256_setr_m128i(lo_128, lo_128);
	__m256i hi = _mm256_setr_m128i(hi_128, hi_128);

	wc0 = lo; wc1 = hi; wc2 = lo; wc3 = hi;

	uint8_t* rng_start = expansion_values_for_seed_256_8_shuffled;
	wc0 = _mm256_add_epi8(wc0, _mm256_load_si256((const __m256i*)(rng_start)));
	wc1 = _mm256_add_epi8(wc1, _mm256_load_si256((const __m256i*)(rng_start + 32)));
	wc2 = _mm256_add_epi8(wc2, _mm256_load_si256((const __m256i*)(rng_start + 64)));
	wc3 = _mm256_add_epi8(wc3, _mm256_load_si256((const __m256i*)(rng_start + 96)));
}

__forceinline void tm_avx2_r256s_map_8::_load_fwd(const uint8_t* block_start, __m256i& r0, __m256i& r1, __m256i& r2, __m256i& r3)
{
	__m128i sel_even = _mm_set_epi8(
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		14, 12, 10, 8, 6, 4, 2, 0);
	__m128i sel_odd = _mm_set_epi8(
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80), static_cast<int8_t>(0x80),
		15, 13, 11, 9, 7, 5, 3, 1);

	__m128i a = _mm_loadu_si128((const __m128i*)(block_start));
	__m128i b = _mm_loadu_si128((const __m128i*)(block_start + 16));
	__m128i c = _mm_loadu_si128((const __m128i*)(block_start + 32));
	__m128i d = _mm_loadu_si128((const __m128i*)(block_start + 48));

	r0 = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(c, sel_even), _mm_shuffle_epi8(d, sel_even)));
	r1 = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(c, sel_odd),  _mm_shuffle_epi8(d, sel_odd)));

	a = _mm_loadu_si128((const __m128i*)(block_start + 64));
	b = _mm_loadu_si128((const __m128i*)(block_start + 80));
	c = _mm_loadu_si128((const __m128i*)(block_start + 96));
	d = _mm_loadu_si128((const __m128i*)(block_start + 112));

	r2 = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(c, sel_even), _mm_shuffle_epi8(d, sel_even)));
	r3 = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(c, sel_odd),  _mm_shuffle_epi8(d, sel_odd)));
}

__forceinline void tm_avx2_r256s_map_8::alg_0(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(block_start, r0, r1, r2, r3);
	wc0 = _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc0, 1), mask_FE), r0);
	wc1 = _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc1, 1), mask_FE), r1);
	wc2 = _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc2, 1), mask_FE), r2);
	wc3 = _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc3, 1), mask_FE), r3);
}

__forceinline void tm_avx2_r256s_map_8::alg_1(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(block_start, r0, r1, r2, r3);
	wc0 = _mm256_add_epi8(wc0, r0);
	wc1 = _mm256_add_epi8(wc1, r1);
	wc2 = _mm256_add_epi8(wc2, r2);
	wc3 = _mm256_add_epi8(wc3, r3);
}

__forceinline void tm_avx2_r256s_map_8::alg_2_sub(__m256i& working_a, __m256i& working_b, __m256i& carry)
{
	__m256i cur_val1_most = _mm256_and_si256(_mm256_srli_epi16(working_a, 1), mask_7F);
	__m256i cur_val2_masked = _mm256_and_si256(working_b, mask_80);

	__m256i bridge = _mm256_permute2x128_si256(working_a, working_a, 0x81);
	__m256i cur_val1_srl = _mm256_alignr_epi8(bridge, working_a, 1);
	__m256i cur_val1_bit = _mm256_and_si256(cur_val1_srl, mask_01);
	cur_val1_bit = _mm256_or_si256(cur_val1_bit, carry);

	__m256i cur_val2_most = _mm256_and_si256(_mm256_slli_epi16(working_b, 1), mask_FE);

	uint8_t bit0 = static_cast<uint8_t>(_mm256_extract_epi8(working_a, 0)) & 0x01;
	__m256i next_carry = _mm256_insert_epi8(_mm256_setzero_si256(), static_cast<int>(bit0), 31);

	working_a = _mm256_or_si256(cur_val1_most, cur_val2_masked);
	working_b = _mm256_or_si256(cur_val2_most, cur_val1_bit);
	carry = next_carry;
}

__forceinline void tm_avx2_r256s_map_8::alg_2(WC_ARGS_256, uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_01);
	alg_2_sub(wc2, wc3, carry);
	alg_2_sub(wc0, wc1, carry);
}

__forceinline void tm_avx2_r256s_map_8::alg_3(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(block_start, r0, r1, r2, r3);
	wc0 = _mm256_xor_si256(wc0, r0);
	wc1 = _mm256_xor_si256(wc1, r1);
	wc2 = _mm256_xor_si256(wc2, r2);
	wc3 = _mm256_xor_si256(wc3, r3);
}

__forceinline void tm_avx2_r256s_map_8::alg_4(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(block_start, r0, r1, r2, r3);
	wc0 = _mm256_sub_epi8(wc0, r0);
	wc1 = _mm256_sub_epi8(wc1, r1);
	wc2 = _mm256_sub_epi8(wc2, r2);
	wc3 = _mm256_sub_epi8(wc3, r3);
}

__forceinline void tm_avx2_r256s_map_8::alg_5_sub(__m256i& working_a, __m256i& working_b, __m256i& carry)
{
	__m256i cur_val1_most = _mm256_and_si256(_mm256_slli_epi16(working_a, 1), mask_FE);
	__m256i cur_val2_masked = _mm256_and_si256(working_b, mask_01);

	__m256i cur_val1_bit = _mm256_and_si256(working_a, mask_80);
	__m256i mask = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(3, 0, 0, 3));
	__m256i cur_val1_srl = _mm256_alignr_epi8(mask, cur_val1_bit, 1);
	__m256i cur_val1_srl_w_carry = _mm256_or_si256(cur_val1_srl, carry);

	__m256i cur_val2_most = _mm256_and_si256(_mm256_srli_epi16(working_b, 1), mask_7F);

	__m256i lo_to_hi = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(0, 0, 3, 0));
	__m256i next_carry = _mm256_bslli_epi128(lo_to_hi, 15);

	working_a = _mm256_or_si256(cur_val1_most, cur_val2_masked);
	working_b = _mm256_or_si256(cur_val2_most, cur_val1_srl_w_carry);
	carry = next_carry;
}

__forceinline void tm_avx2_r256s_map_8::alg_5(WC_ARGS_256, uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_80);
	alg_5_sub(wc2, wc3, carry);
	alg_5_sub(wc0, wc1, carry);
}

__forceinline void tm_avx2_r256s_map_8::alg_6(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(block_start, r0, r1, r2, r3);
	wc0 = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc0, 1), mask_7F), r0);
	wc1 = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc1, 1), mask_7F), r1);
	wc2 = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc2, 1), mask_7F), r2);
	wc3 = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc3, 1), mask_7F), r3);
}

__forceinline void tm_avx2_r256s_map_8::alg_7(WC_ARGS_256)
{
	wc0 = _mm256_xor_si256(wc0, mask_FF);
	wc1 = _mm256_xor_si256(wc1, mask_FF);
	wc2 = _mm256_xor_si256(wc2, mask_FF);
	wc3 = _mm256_xor_si256(wc3, mask_FF);
}

__forceinline void tm_avx2_r256s_map_8::xor_alg(WC_ARGS_256, uint8_t* values)
{
	wc0 = _mm256_xor_si256(wc0, _mm256_load_si256((__m256i*)(values)));
	wc1 = _mm256_xor_si256(wc1, _mm256_load_si256((__m256i*)(values + 32)));
	wc2 = _mm256_xor_si256(wc2, _mm256_load_si256((__m256i*)(values + 64)));
	wc3 = _mm256_xor_si256(wc3, _mm256_load_si256((__m256i*)(values + 96)));
}

__forceinline void tm_avx2_r256s_map_8::_run_alg(WC_ARGS_256, int algorithm_id, uint16_t* local_pos,
	const uint8_t* reg_base, const uint8_t* alg0_base,
	const uint8_t* alg6_base)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_256, alg0_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 1)
	{
		alg_1(WC_PASS_256, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS_256, reg_base[*local_pos] >> 7);
		*local_pos -= 1;
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS_256, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 4)
	{
		alg_4(WC_PASS_256, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS_256, reg_base[*local_pos] & 0x80);
		*local_pos -= 1;
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS_256, alg6_base + (2047 - *local_pos));
		*local_pos -= 128;
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS_256);
	}
}

__forceinline void tm_avx2_r256s_map_8::_run_one_map(WC_ARGS_256, int map_idx)
{
	uint16_t nibble_selector = schedule_entries->entries[static_cast<size_t>(map_idx)].nibble_selector;
	const uint8_t* reg_base = regular_rng_values_for_seeds_8 + map_idx * 2048;
	const uint8_t* alg0_base = alg0_values_for_seeds_8 + map_idx * 2048;
	const uint8_t* alg6_base = alg6_values_for_seeds_8 + map_idx * 2048;
	uint16_t local_pos = 2047;

	for (int i = 0; i < 16; i++)
	{
		_mm256_store_si256((__m256i*)(working_code_data),      wc0);
		_mm256_store_si256((__m256i*)(working_code_data + 32), wc1);

		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector <<= 1;

		uint8_t current_byte = working_code_data[shuffle_8(i, 256)];
		if (nibble == 1)
			current_byte >>= 4;
		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(WC_PASS_256, algorithm_id, &local_pos, reg_base, alg0_base, alg6_base);
	}
}

__forceinline void tm_avx2_r256s_map_8::_run_all_maps(WC_ARGS_256)
{
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
	{
		_run_one_map(WC_PASS_256, map_idx);
	}
}

__forceinline void tm_avx2_r256s_map_8::_decrypt_carnival_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, carnival_world_data_shuffled);
}

__forceinline void tm_avx2_r256s_map_8::_decrypt_other_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, other_world_data_shuffled);
}

__forceinline void tm_avx2_r256s_map_8::mid_sum(
	__m128i& sum, __m256i& working_code, __m256i& sum_mask, __m128i& lo_mask)
{
	__m128i masked_lo = _mm_and_si128(
		_mm256_castsi256_si128(working_code),
		_mm256_castsi256_si128(sum_mask));
	__m128i masked_hi = _mm_and_si128(
		_mm256_extractf128_si256(working_code, 1),
		_mm256_extractf128_si256(sum_mask, 1));

	sum = _mm_add_epi16(sum, _mm_and_si128(masked_lo, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_lo, 8));
	sum = _mm_add_epi16(sum, _mm_and_si128(masked_hi, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_hi, 8));
}

__forceinline uint16_t tm_avx2_r256s_map_8::masked_checksum(WC_ARGS_256, uint8_t* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m256i sum_mask = _mm256_load_si256((__m256i*)(mask));       mid_sum(sum, wc0, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 32));          mid_sum(sum, wc1, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 64));          mid_sum(sum, wc2, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 96));          mid_sum(sum, wc3, sum_mask, lo_mask);

	return (uint16_t)(
		_mm_extract_epi16(sum, 0) + _mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) + _mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) + _mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) + _mm_extract_epi16(sum, 7));
}

__forceinline uint16_t tm_avx2_r256s_map_8::_calculate_carnival_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, carnival_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx2_r256s_map_8::_calculate_other_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, other_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx2_r256s_map_8::fetch_checksum_value(WC_ARGS_256, uint8_t code_length)
{
	_store_to_mem(WC_PASS_256);
	uint8_t lo = working_code_data[shuffle_8(127 - code_length, 256)];
	uint8_t hi = working_code_data[shuffle_8(127 - (code_length + 1), 256)];
	return (uint16_t)((hi << 8) | lo);
}

__forceinline uint16_t tm_avx2_r256s_map_8::_fetch_carnival_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx2_r256s_map_8::_fetch_other_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx2_r256s_map_8::check_carnival_world_checksum(WC_ARGS_256)
{
	return _calculate_carnival_world_checksum(WC_PASS_256)
		== _fetch_carnival_world_checksum_value(WC_PASS_256);
}

__forceinline bool tm_avx2_r256s_map_8::check_other_world_checksum(WC_ARGS_256)
{
	return _calculate_other_world_checksum(WC_PASS_256)
		== _fetch_other_world_checksum_value(WC_PASS_256);
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx2_r256s_map_8::_decrypt_check(WC_ARGS_256)
{
	WC_XOR_VARS_256;
	WC_COPY_XOR_256;
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(WC_XOR_PASS_256);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_carnival_world_checksum(WC_XOR_PASS_256))
			{
				return std::nullopt;
			}
		}
	}
	else
	{
		_decrypt_other_world(WC_XOR_PASS_256);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum(WC_XOR_PASS_256))
			{
				return std::nullopt;
			}
		}
	}

	_store_to_mem(WC_XOR_PASS_256);
	uint8_t unshuffled_data[128];
	unshuffle_mem(working_code_data, unshuffled_data, 256, false);
	return check_machine_code(unshuffled_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx2_r256s_map_8::_run_bruteforce(WC_ARGS_256, uint32_t data, uint8_t* result_data, uint32_t* result_size)
{
	_expand_code(data, WC_PASS_256);

	_run_all_maps(WC_PASS_256);

	auto carnival_flags = _decrypt_check<CHECK_CHECKSUMS, CARNIVAL_WORLD>(WC_PASS_256);
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

	auto other_flags = _decrypt_check<CHECK_CHECKSUMS, OTHER_WORLD>(WC_PASS_256);
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

void tm_avx2_r256s_map_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	test_algorithm_n(algorithm_id, data, rng_seed, 1);
}

void tm_avx2_r256s_map_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_256;
	load_data(data);
	_load_from_mem(WC_PASS_256);

	if (algorithm_id == 0)
	{
		rng->generate_alg0_values_for_seeds_8(&alg0_values_for_seeds_8, rng_seed, 1);
	}
	else if (algorithm_id == 1 || algorithm_id == 2 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 5)
	{
		rng->generate_regular_rng_values_for_seeds_8(&regular_rng_values_for_seeds_8, rng_seed, 1);
	}
	else if (algorithm_id == 6)
	{
		rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, rng_seed, 1);
	}

	uint16_t local_pos = 2047;
	for (int i = 0; i < iterations; i++)
	{
		if (local_pos < 128)
		{
			local_pos = 2047;
		}
		_run_alg(WC_PASS_256, algorithm_id, &local_pos,
			regular_rng_values_for_seeds_8, alg0_values_for_seeds_8,
			alg6_values_for_seeds_8);
	}

	_store_to_mem(WC_PASS_256);
	fetch_data(data);
}

void tm_avx2_r256s_map_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

void tm_avx2_r256s_map_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_run_all_maps(WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

bool tm_avx2_r256s_map_8::test_bruteforce_checksum(uint32_t data, int world)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_run_all_maps(WC_PASS_256);

	if (world == CARNIVAL_WORLD)
	{
		return _decrypt_check<true, CARNIVAL_WORLD>(WC_PASS_256).has_value();
	}
	else
	{
		return _decrypt_check<true, OTHER_WORLD>(WC_PASS_256).has_value();
	}
}

void tm_avx2_r256s_map_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
{
	WC_VARS_256;

	for (uint32_t i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
		{
			return;
		}
		uint32_t data = start_data + i;

		_run_bruteforce<true>(WC_PASS_256, data, result_data, result_size);

		report_progress(static_cast<double>(i + 1) / static_cast<double>(amount_to_run));
	}
}

void tm_avx2_r256s_map_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_256;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_256, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

bool tm_avx2_r256s_map_8::initialized = false;
