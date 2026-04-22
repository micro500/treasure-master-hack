#include "tm_avx512vl_r256_map_8.h"

tm_avx512vl_r256_map_8::tm_avx512vl_r256_map_8(RNG* rng_obj) : tm_avx512vl_r256_map_8(rng_obj, 0) {}

tm_avx512vl_r256_map_8::tm_avx512vl_r256_map_8(RNG* rng_obj, const uint32_t key) : tm_avx512vl_r256_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx512vl_r256_map_8::tm_avx512vl_r256_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
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

	expansion_values_for_seed_128_8 = nullptr;
	regular_rng_values_for_seeds_8 = nullptr;
	alg0_values_for_seeds_8 = nullptr;
	alg6_values_for_seeds_8 = nullptr;

	generate_map_rng();
}

tm_avx512vl_r256_map_8::~tm_avx512vl_r256_map_8()
{
}

__forceinline void tm_avx512vl_r256_map_8::initialize()
{
	if (!initialized)
	{
		initialized = true;
	}
	obj_name = "tm_avx512vl_r256_map_8";
}

void tm_avx512vl_r256_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(&expansion_values_for_seed_128_8, (key >> 16) & 0xFFFF, false, 128);
	rng->generate_regular_rng_values_for_seeds_8(&regular_rng_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg0_values_for_seeds_8(&alg0_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
}

__forceinline void tm_avx512vl_r256_map_8::_load_from_mem(WC_ARGS_256)
{
	wc0 = _mm256_load_si256((__m256i*)(working_code_data));
	wc1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	wc2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	wc3 = _mm256_load_si256((__m256i*)(working_code_data + 96));
}

__forceinline void tm_avx512vl_r256_map_8::_store_to_mem(WC_ARGS_256)
{
	_mm256_store_si256((__m256i*)(working_code_data), wc0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), wc1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), wc2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), wc3);
}

void tm_avx512vl_r256_map_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		working_code_data[i] = new_data[i];
}

void tm_avx512vl_r256_map_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		new_data[i] = working_code_data[i];
}

__forceinline void tm_avx512vl_r256_map_8::_expand_code(uint32_t data, WC_ARGS_256)
{
	uint64_t x = ((uint64_t)key << 32) | data;
	__m128i a = _mm_cvtsi64_si128(static_cast<int64_t>(x));
	__m128i nat_mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7);
	__m128i pattern = _mm_shuffle_epi8(a, nat_mask);
	__m256i pat256 = _mm256_broadcastsi128_si256(pattern);

	wc0 = pat256; wc1 = pat256; wc2 = pat256; wc3 = pat256;

	uint8_t* rng_start = expansion_values_for_seed_128_8;
	wc0 = _mm256_add_epi8(wc0, _mm256_loadu_si256((const __m256i*)(rng_start)));
	wc1 = _mm256_add_epi8(wc1, _mm256_loadu_si256((const __m256i*)(rng_start + 32)));
	wc2 = _mm256_add_epi8(wc2, _mm256_loadu_si256((const __m256i*)(rng_start + 64)));
	wc3 = _mm256_add_epi8(wc3, _mm256_loadu_si256((const __m256i*)(rng_start + 96)));
}

__forceinline void tm_avx512vl_r256_map_8::alg_2_sub(__m256i& wc, __m256i& carry)
{
	__m256i part_lo = _mm256_and_si256(_mm256_srli_epi16(wc, 1), _mm256_set1_epi16(0x007F));
	__m256i part_hi_bit = _mm256_and_si256(_mm256_srli_epi16(wc, 8), _mm256_set1_epi16(0x0080));
	__m256i new_lo = _mm256_or_si256(part_lo, part_hi_bit);

	__m256i low_bit0 = _mm256_and_si256(wc, _mm256_set1_epi16(0x0001));
	__m256i low_bit0_up = _mm256_slli_epi16(low_bit0, 8);
	__m256i carry_in = _mm256_or_si256(
		_mm256_alignr_epi8(
			_mm256_permute2x128_si256(low_bit0_up, _mm256_setzero_si256(), 0x81),
			low_bit0_up,
			2),
		carry);

	__m256i new_hi = _mm256_or_si256(
		_mm256_and_si256(_mm256_slli_epi16(wc, 1), _mm256_set1_epi16(static_cast<int16_t>(0xFE00))),
		carry_in);

	__m128i lo_lane = _mm256_castsi256_si128(_mm256_and_si256(wc, _mm256_set1_epi16(0x0001)));
	__m128i lo_shifted = _mm_slli_si128(lo_lane, 15);
	carry = _mm256_and_si256(
		_mm256_inserti128_si256(_mm256_setzero_si256(), lo_shifted, 1),
		mask_top_01);

	wc = _mm256_or_si256(
		_mm256_and_si256(new_lo, _mm256_set1_epi16(0x00FF)),
		_mm256_and_si256(new_hi, _mm256_set1_epi16(static_cast<int16_t>(0xFF00))));
}

__forceinline void tm_avx512vl_r256_map_8::alg_5_sub(__m256i& wc, __m256i& carry)
{
	__m256i part_lo = _mm256_and_si256(_mm256_slli_epi16(wc, 1), _mm256_set1_epi16(0x00FE));
	__m256i part_lo_bit = _mm256_and_si256(_mm256_srli_epi16(wc, 8), _mm256_set1_epi16(0x0001));
	__m256i new_lo = _mm256_or_si256(part_lo, part_lo_bit);

	__m256i low_bit7 = _mm256_and_si256(wc, _mm256_set1_epi16(0x0080));
	__m256i low_bit7_up = _mm256_slli_epi16(low_bit7, 8);
	__m256i carry_in = _mm256_or_si256(
		_mm256_alignr_epi8(
			_mm256_permute2x128_si256(low_bit7_up, _mm256_setzero_si256(), 0x81),
			low_bit7_up,
			2),
		carry);

	__m256i new_hi = _mm256_or_si256(
		_mm256_and_si256(_mm256_srli_epi16(wc, 1), _mm256_set1_epi16(static_cast<int16_t>(0x7F00))),
		carry_in);

	__m128i lo_lane = _mm256_castsi256_si128(_mm256_and_si256(wc, _mm256_set1_epi16(0x0080)));
	__m128i lo_shifted = _mm_slli_si128(lo_lane, 15);
	carry = _mm256_and_si256(
		_mm256_inserti128_si256(_mm256_setzero_si256(), lo_shifted, 1),
		mask_top_80);

	wc = _mm256_or_si256(
		_mm256_and_si256(new_lo, _mm256_set1_epi16(0x00FF)),
		_mm256_and_si256(new_hi, _mm256_set1_epi16(static_cast<int16_t>(0xFF00))));
}

__forceinline void tm_avx512vl_r256_map_8::alg_0(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i rng_val = _mm256_loadu_si256((const __m256i*)(block_start));
	wc0 = _mm256_ternarylogic_epi32(_mm256_slli_epi16(wc0, 1), mask_FE, rng_val, 0xEA);

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 32));
	wc1 = _mm256_ternarylogic_epi32(_mm256_slli_epi16(wc1, 1), mask_FE, rng_val, 0xEA);

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 64));
	wc2 = _mm256_ternarylogic_epi32(_mm256_slli_epi16(wc2, 1), mask_FE, rng_val, 0xEA);

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 96));
	wc3 = _mm256_ternarylogic_epi32(_mm256_slli_epi16(wc3, 1), mask_FE, rng_val, 0xEA);
}

__forceinline void tm_avx512vl_r256_map_8::alg_1(WC_ARGS_256, const uint8_t* block_start)
{
	wc0 = _mm256_add_epi8(wc0, _mm256_loadu_si256((const __m256i*)(block_start)));
	wc1 = _mm256_add_epi8(wc1, _mm256_loadu_si256((const __m256i*)(block_start + 32)));
	wc2 = _mm256_add_epi8(wc2, _mm256_loadu_si256((const __m256i*)(block_start + 64)));
	wc3 = _mm256_add_epi8(wc3, _mm256_loadu_si256((const __m256i*)(block_start + 96)));
}

__forceinline void tm_avx512vl_r256_map_8::alg_2(WC_ARGS_256, uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_01);
	alg_2_sub(wc3, carry);
	alg_2_sub(wc2, carry);
	alg_2_sub(wc1, carry);
	alg_2_sub(wc0, carry);
}

__forceinline void tm_avx512vl_r256_map_8::alg_3(WC_ARGS_256, const uint8_t* block_start)
{
	wc0 = _mm256_xor_si256(wc0, _mm256_loadu_si256((const __m256i*)(block_start)));
	wc1 = _mm256_xor_si256(wc1, _mm256_loadu_si256((const __m256i*)(block_start + 32)));
	wc2 = _mm256_xor_si256(wc2, _mm256_loadu_si256((const __m256i*)(block_start + 64)));
	wc3 = _mm256_xor_si256(wc3, _mm256_loadu_si256((const __m256i*)(block_start + 96)));
}

__forceinline void tm_avx512vl_r256_map_8::alg_4(WC_ARGS_256, const uint8_t* block_start)
{
	wc0 = _mm256_sub_epi8(wc0, _mm256_loadu_si256((const __m256i*)(block_start)));
	wc1 = _mm256_sub_epi8(wc1, _mm256_loadu_si256((const __m256i*)(block_start + 32)));
	wc2 = _mm256_sub_epi8(wc2, _mm256_loadu_si256((const __m256i*)(block_start + 64)));
	wc3 = _mm256_sub_epi8(wc3, _mm256_loadu_si256((const __m256i*)(block_start + 96)));
}

__forceinline void tm_avx512vl_r256_map_8::alg_5(WC_ARGS_256, uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_80);
	alg_5_sub(wc3, carry);
	alg_5_sub(wc2, carry);
	alg_5_sub(wc1, carry);
	alg_5_sub(wc0, carry);
}

__forceinline void tm_avx512vl_r256_map_8::alg_6(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i rng_val = _mm256_loadu_si256((const __m256i*)(block_start));
	wc0 = _mm256_ternarylogic_epi32(_mm256_srli_epi16(wc0, 1), mask_7F, rng_val, 0xEA);

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 32));
	wc1 = _mm256_ternarylogic_epi32(_mm256_srli_epi16(wc1, 1), mask_7F, rng_val, 0xEA);

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 64));
	wc2 = _mm256_ternarylogic_epi32(_mm256_srli_epi16(wc2, 1), mask_7F, rng_val, 0xEA);

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 96));
	wc3 = _mm256_ternarylogic_epi32(_mm256_srli_epi16(wc3, 1), mask_7F, rng_val, 0xEA);
}

__forceinline void tm_avx512vl_r256_map_8::alg_7(WC_ARGS_256)
{
	wc0 = _mm256_xor_si256(wc0, mask_FF);
	wc1 = _mm256_xor_si256(wc1, mask_FF);
	wc2 = _mm256_xor_si256(wc2, mask_FF);
	wc3 = _mm256_xor_si256(wc3, mask_FF);
}

__forceinline void tm_avx512vl_r256_map_8::xor_alg(WC_ARGS_256, uint8_t* values)
{
	wc0 = _mm256_xor_si256(wc0, _mm256_load_si256((__m256i*)(values)));
	wc1 = _mm256_xor_si256(wc1, _mm256_load_si256((__m256i*)(values + 32)));
	wc2 = _mm256_xor_si256(wc2, _mm256_load_si256((__m256i*)(values + 64)));
	wc3 = _mm256_xor_si256(wc3, _mm256_load_si256((__m256i*)(values + 96)));
}

__forceinline void tm_avx512vl_r256_map_8::_run_alg(WC_ARGS_256, int algorithm_id, uint16_t* local_pos,
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

__forceinline void tm_avx512vl_r256_map_8::_run_one_map(WC_ARGS_256, int map_idx)
{
	uint16_t nibble_selector = schedule_entries->entries[static_cast<size_t>(map_idx)].nibble_selector;
	const uint8_t* reg_base = regular_rng_values_for_seeds_8 + map_idx * 2048;
	const uint8_t* alg0_base = alg0_values_for_seeds_8 + map_idx * 2048;
	const uint8_t* alg6_base = alg6_values_for_seeds_8 + map_idx * 2048;
	uint16_t local_pos = 2047;

	for (int i = 0; i < 16; i++)
	{
		_mm256_store_si256((__m256i*)(working_code_data), wc0);

		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector <<= 1;

		uint8_t current_byte = working_code_data[i];
		if (nibble == 1)
			current_byte >>= 4;
		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(WC_PASS_256, algorithm_id, &local_pos, reg_base, alg0_base, alg6_base);
	}
}

__forceinline void tm_avx512vl_r256_map_8::_run_all_maps(WC_ARGS_256)
{
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
	{
		_run_one_map(WC_PASS_256, map_idx);
	}
}

__forceinline void tm_avx512vl_r256_map_8::_decrypt_carnival_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, carnival_world_data);
}

__forceinline void tm_avx512vl_r256_map_8::_decrypt_other_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, other_world_data);
}

__forceinline void tm_avx512vl_r256_map_8::mid_sum(
	__m128i& sum, __m256i& working_code, __m256i& sum_mask, __m128i& lo_mask)
{
	__m128i masked_lo = _mm_and_si128(
		_mm256_castsi256_si128(working_code),
		_mm256_castsi256_si128(sum_mask));
	__m128i masked_hi = _mm_and_si128(
		_mm256_extracti128_si256(working_code, 1),
		_mm256_extracti128_si256(sum_mask, 1));

	sum = _mm_add_epi16(sum, _mm_and_si128(masked_lo, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_lo, 8));
	sum = _mm_add_epi16(sum, _mm_and_si128(masked_hi, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_hi, 8));
}

__forceinline uint16_t tm_avx512vl_r256_map_8::masked_checksum(WC_ARGS_256, uint8_t* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m256i sum_mask = _mm256_load_si256((__m256i*)(mask));
	mid_sum(sum, wc0, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 32));
	mid_sum(sum, wc1, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 64));
	mid_sum(sum, wc2, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 96));
	mid_sum(sum, wc3, sum_mask, lo_mask);

	return static_cast<uint16_t>(
		_mm_extract_epi16(sum, 0) + _mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) + _mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) + _mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) + _mm_extract_epi16(sum, 7));
}

__forceinline uint16_t tm_avx512vl_r256_map_8::_calculate_carnival_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, carnival_world_checksum_mask);
}

__forceinline uint16_t tm_avx512vl_r256_map_8::_calculate_other_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, other_world_checksum_mask);
}

__forceinline uint16_t tm_avx512vl_r256_map_8::fetch_checksum_value(WC_ARGS_256, uint8_t code_length)
{
	_store_to_mem(WC_PASS_256);
	uint8_t lo = working_code_data[127 - code_length];
	uint8_t hi = working_code_data[127 - (code_length + 1)];
	return static_cast<uint16_t>((hi << 8) | lo);
}

__forceinline uint16_t tm_avx512vl_r256_map_8::_fetch_carnival_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx512vl_r256_map_8::_fetch_other_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx512vl_r256_map_8::check_carnival_world_checksum(WC_ARGS_256)
{
	return _calculate_carnival_world_checksum(WC_PASS_256)
	    == _fetch_carnival_world_checksum_value(WC_PASS_256);
}

__forceinline bool tm_avx512vl_r256_map_8::check_other_world_checksum(WC_ARGS_256)
{
	return _calculate_other_world_checksum(WC_PASS_256)
	    == _fetch_other_world_checksum_value(WC_PASS_256);
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx512vl_r256_map_8::_decrypt_check(WC_ARGS_256)
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
	return check_machine_code(working_code_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx512vl_r256_map_8::_run_bruteforce(WC_ARGS_256, uint32_t data, uint8_t* result_data, uint32_t* result_size)
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

void tm_avx512vl_r256_map_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
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

void tm_avx512vl_r256_map_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_256;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_256, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_avx512vl_r256_map_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	test_algorithm_n(algorithm_id, data, rng_seed, 1);
}

void tm_avx512vl_r256_map_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
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

void tm_avx512vl_r256_map_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

void tm_avx512vl_r256_map_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_run_all_maps(WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

bool tm_avx512vl_r256_map_8::test_bruteforce_checksum(uint32_t data, int world)
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

bool tm_avx512vl_r256_map_8::initialized = false;
