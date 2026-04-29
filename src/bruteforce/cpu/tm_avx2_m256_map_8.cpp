#include "tm_avx2_m256_map_8.h"

tm_avx2_m256_map_8::tm_avx2_m256_map_8(RNG* rng_obj) : tm_avx2_m256_map_8(rng_obj, 0) {}

tm_avx2_m256_map_8::tm_avx2_m256_map_8(RNG* rng_obj, const uint32_t key) : tm_avx2_m256_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx2_m256_map_8::tm_avx2_m256_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
	: TM_base(rng_obj),
	  mask_FF(_mm256_set1_epi8(static_cast<int8_t>(0xFF))),
	  mask_FE(_mm256_set1_epi8(static_cast<int8_t>(0xFE))),
	  mask_7F(_mm256_set1_epi8(0x7F)),
	  mask_80(_mm256_set1_epi8(static_cast<int8_t>(0x80))),
	  mask_01(_mm256_set1_epi8(0x01)),
	  mask_top_01(_mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
	  mask_top_80(_mm256_set_epi16(static_cast<int16_t>(0x8000), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
	  mask_0001(_mm256_set1_epi16(0x0001)),
	  mask_0080(_mm256_set1_epi16(0x0080)),
	  mask_007F(_mm256_set1_epi16(0x007F)),
	  mask_00FE(_mm256_set1_epi16(0x00FE)),
	  mask_00FF(_mm256_set1_epi16(0x00FF)),
	  mask_FF00(_mm256_set1_epi16(static_cast<int16_t>(0xFF00))),
	  mask_FE00(_mm256_set1_epi16(static_cast<int16_t>(0xFE00))),
	  mask_7F00(_mm256_set1_epi16(0x7F00))
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;

	generate_map_rng();
}

tm_avx2_m256_map_8::~tm_avx2_m256_map_8()
{
}

void tm_avx2_m256_map_8::initialize()
{
	if (!_initialized)
	{
		_initialized = true;
	}
	obj_name = "tm_avx2_m256_map_8";
}

void tm_avx2_m256_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(expansion_values_for_seed_256_8, (key >> 16) & 0xFFFF, false, 128);
	regular_rng_values_for_seeds_8 = rng->generate_regular_rng_values_for_seeds_8(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	alg0_values_for_seeds_8 = rng->generate_alg0_values_for_seeds_8(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	alg6_values_for_seeds_8 = rng->generate_alg6_values_for_seeds_8(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
}

void tm_avx2_m256_map_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		working_code_data[i] = new_data[i];
}

void tm_avx2_m256_map_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		new_data[i] = working_code_data[i];
}

__forceinline void tm_avx2_m256_map_8::_expand_code(uint32_t data)
{
	__m128i a = _mm_insert_epi32(_mm_cvtsi32_si128(static_cast<int32_t>(data)), static_cast<int32_t>(key), 1);
	__m128i nat_mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7);
	__m256i pat256 = _mm256_broadcastsi128_si256(_mm_shuffle_epi8(a, nat_mask));

	uint8_t* rng_start = expansion_values_for_seed_256_8.get();
	for (int i = 0; i < 4; i++)
	{
		__m256i rng_val = _mm256_loadu_si256((const __m256i*)(rng_start + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_add_epi8(pat256, rng_val));
	}
}

__forceinline void tm_avx2_m256_map_8::alg_2_sub(__m256i& wc, __m256i& carry)
{
	__m256i part_lo = _mm256_and_si256(_mm256_srli_epi16(wc, 1), mask_007F);
	__m256i part_hi_bit = _mm256_and_si256(_mm256_srli_epi16(wc, 8), mask_0080);
	__m256i new_lo = _mm256_or_si256(part_lo, part_hi_bit);

	__m256i low_bit0 = _mm256_and_si256(wc, mask_0001);
	__m256i low_bit0_up = _mm256_slli_epi16(low_bit0, 8);
	__m256i carry_in = _mm256_or_si256(
		_mm256_alignr_epi8(
			_mm256_permute2x128_si256(low_bit0_up, _mm256_setzero_si256(), 0x81),
			low_bit0_up,
			2),
		carry);

	__m256i new_hi = _mm256_or_si256(
		_mm256_and_si256(_mm256_slli_epi16(wc, 1), mask_FE00),
		carry_in);

	__m128i lo_lane = _mm256_castsi256_si128(_mm256_and_si256(wc, mask_0001));
	__m128i lo_shifted = _mm_slli_si128(lo_lane, 15);
	carry = _mm256_and_si256(
		_mm256_inserti128_si256(_mm256_setzero_si256(), lo_shifted, 1),
		mask_top_01);

	wc = _mm256_or_si256(
		_mm256_and_si256(new_lo, mask_00FF),
		_mm256_and_si256(new_hi, mask_FF00));
}

__forceinline void tm_avx2_m256_map_8::alg_5_sub(__m256i& wc, __m256i& carry)
{
	__m256i part_lo = _mm256_and_si256(_mm256_slli_epi16(wc, 1), mask_00FE);
	__m256i part_lo_bit = _mm256_and_si256(_mm256_srli_epi16(wc, 8), mask_0001);
	__m256i new_lo = _mm256_or_si256(part_lo, part_lo_bit);

	__m256i low_bit7 = _mm256_and_si256(wc, mask_0080);
	__m256i low_bit7_up = _mm256_slli_epi16(low_bit7, 8);
	__m256i carry_in = _mm256_or_si256(
		_mm256_alignr_epi8(
			_mm256_permute2x128_si256(low_bit7_up, _mm256_setzero_si256(), 0x81),
			low_bit7_up,
			2),
		carry);

	__m256i new_hi = _mm256_or_si256(
		_mm256_and_si256(_mm256_srli_epi16(wc, 1), mask_7F00),
		carry_in);

	__m128i lo_lane = _mm256_castsi256_si128(_mm256_and_si256(wc, mask_0080));
	__m128i lo_shifted = _mm_slli_si128(lo_lane, 15);
	carry = _mm256_and_si256(
		_mm256_inserti128_si256(_mm256_setzero_si256(), lo_shifted, 1),
		mask_top_80);

	wc = _mm256_or_si256(
		_mm256_and_si256(new_lo, mask_00FF),
		_mm256_and_si256(new_hi, mask_FF00));
}

__forceinline void tm_avx2_m256_map_8::alg_0(const uint8_t* block_start)
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_loadu_si256((const __m256i*)(block_start + i * 32));
		cur_val = _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(cur_val, 1), mask_FE), rng_val);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx2_m256_map_8::alg_1(const uint8_t* block_start)
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_loadu_si256((const __m256i*)(block_start + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_add_epi8(cur_val, rng_val));
	}
}

__forceinline void tm_avx2_m256_map_8::alg_2(uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_01);
	for (int i = 3; i >= 0; i--)
	{
		__m256i wc = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		alg_2_sub(wc, carry);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), wc);
	}
}

__forceinline void tm_avx2_m256_map_8::alg_3(const uint8_t* block_start)
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_loadu_si256((const __m256i*)(block_start + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_xor_si256(cur_val, rng_val));
	}
}

__forceinline void tm_avx2_m256_map_8::alg_4(const uint8_t* block_start)
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_loadu_si256((const __m256i*)(block_start + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_sub_epi8(cur_val, rng_val));
	}
}

__forceinline void tm_avx2_m256_map_8::alg_5(uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_80);
	for (int i = 3; i >= 0; i--)
	{
		__m256i wc = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		alg_5_sub(wc, carry);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), wc);
	}
}

__forceinline void tm_avx2_m256_map_8::alg_6(const uint8_t* block_start)
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_loadu_si256((const __m256i*)(block_start + i * 32));
		cur_val = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(cur_val, 1), mask_7F), rng_val);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx2_m256_map_8::alg_7()
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_xor_si256(cur_val, mask_FF));
	}
}

__forceinline void tm_avx2_m256_map_8::_run_alg(int algorithm_id, uint16_t* local_pos,
	const uint8_t* reg_base, const uint8_t* alg0_base,
	const uint8_t* alg6_base)
{
	if (algorithm_id == 0)
	{
		alg_0(alg0_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 1)
	{
		alg_1(reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 2)
	{
		alg_2(reg_base[*local_pos] >> 7);
		*local_pos -= 1;
	}
	else if (algorithm_id == 3)
	{
		alg_3(reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 4)
	{
		alg_4(reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 5)
	{
		alg_5(reg_base[*local_pos] & 0x80);
		*local_pos -= 1;
	}
	else if (algorithm_id == 6)
	{
		alg_6(alg6_base + (2047 - *local_pos));
		*local_pos -= 128;
	}
	else if (algorithm_id == 7)
	{
		alg_7();
	}
}

__forceinline void tm_avx2_m256_map_8::_run_one_map(int map_idx)
{
	uint16_t nibble_selector = schedule_entries->entries[static_cast<size_t>(map_idx)].nibble_selector;
	const uint8_t* reg_base = regular_rng_values_for_seeds_8.get() + map_idx * 2048;
	const uint8_t* alg0_base = alg0_values_for_seeds_8.get() + map_idx * 2048;
	const uint8_t* alg6_base = alg6_values_for_seeds_8.get() + map_idx * 2048;
	uint16_t local_pos = 2047;

	for (int i = 0; i < 16; i++)
	{
		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector <<= 1;

		uint8_t current_byte = working_code_data[i];
		if (nibble == 1)
			current_byte >>= 4;
		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(algorithm_id, &local_pos, reg_base, alg0_base, alg6_base);
	}
}

__forceinline void tm_avx2_m256_map_8::_run_all_maps()
{
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
	{
		_run_one_map(map_idx);
	}
}

__forceinline void tm_avx2_m256_map_8::_decrypt_carnival_world()
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i world_val = _mm256_load_si256((const __m256i*)(carnival_world_data + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_xor_si256(cur_val, world_val));
	}
}

__forceinline void tm_avx2_m256_map_8::_decrypt_other_world()
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i world_val = _mm256_load_si256((const __m256i*)(other_world_data + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_xor_si256(cur_val, world_val));
	}
}

__forceinline void tm_avx2_m256_map_8::mid_sum(
	__m128i& sum, __m256i& working_code, __m256i& sum_mask)
{
	__m128i masked_lo = _mm_and_si128(
		_mm256_castsi256_si128(working_code),
		_mm256_castsi256_si128(sum_mask));
	__m128i masked_hi = _mm_and_si128(
		_mm256_extracti128_si256(working_code, 1),
		_mm256_extracti128_si256(sum_mask, 1));

	sum = _mm_add_epi16(sum, _mm_and_si128(masked_lo, _mm256_castsi256_si128(mask_00FF)));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_lo, 8));
	sum = _mm_add_epi16(sum, _mm_and_si128(masked_hi, _mm256_castsi256_si128(mask_00FF)));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_hi, 8));
}

__forceinline uint16_t tm_avx2_m256_map_8::masked_checksum(uint8_t* mask)
{
	__m128i sum = _mm_setzero_si128();

	for (int i = 0; i < 4; i++)
	{
		__m256i working_code = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i sum_mask = _mm256_load_si256((__m256i*)(mask + i * 32));
		mid_sum(sum, working_code, sum_mask);
	}

	return static_cast<uint16_t>(
		_mm_extract_epi16(sum, 0) + _mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) + _mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) + _mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) + _mm_extract_epi16(sum, 7));
}

__forceinline uint16_t tm_avx2_m256_map_8::_calculate_carnival_world_checksum()
{
	return masked_checksum(carnival_world_checksum_mask);
}

__forceinline uint16_t tm_avx2_m256_map_8::_calculate_other_world_checksum()
{
	return masked_checksum(other_world_checksum_mask);
}

__forceinline uint16_t tm_avx2_m256_map_8::_fetch_carnival_world_checksum_value()
{
	uint8_t lo = working_code_data[127 - (CARNIVAL_WORLD_CODE_LENGTH - 2)];
	uint8_t hi = working_code_data[127 - (CARNIVAL_WORLD_CODE_LENGTH - 2 + 1)];
	return static_cast<uint16_t>((hi << 8) | lo);
}

__forceinline uint16_t tm_avx2_m256_map_8::_fetch_other_world_checksum_value()
{
	uint8_t lo = working_code_data[127 - (OTHER_WORLD_CODE_LENGTH - 2)];
	uint8_t hi = working_code_data[127 - (OTHER_WORLD_CODE_LENGTH - 2 + 1)];
	return static_cast<uint16_t>((hi << 8) | lo);
}

__forceinline bool tm_avx2_m256_map_8::check_carnival_world_checksum()
{
	return _calculate_carnival_world_checksum() == _fetch_carnival_world_checksum_value();
}

__forceinline bool tm_avx2_m256_map_8::check_other_world_checksum()
{
	return _calculate_other_world_checksum() == _fetch_other_world_checksum_value();
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx2_m256_map_8::_decrypt_check()
{
	uint8_t saved_data[128];
	for (int i = 0; i < 128; i++)
		saved_data[i] = working_code_data[i];

	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world();

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_carnival_world_checksum())
			{
				for (int i = 0; i < 128; i++)
					working_code_data[i] = saved_data[i];
				return std::nullopt;
			}
		}
	}
	else
	{
		_decrypt_other_world();

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum())
			{
				for (int i = 0; i < 128; i++)
					working_code_data[i] = saved_data[i];
				return std::nullopt;
			}
		}
	}

	uint8_t result = check_machine_code(working_code_data, WORLD);
	for (int i = 0; i < 128; i++)
		working_code_data[i] = saved_data[i];
	return result;
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx2_m256_map_8::_run_bruteforce(uint32_t data, uint8_t* result_data, uint32_t* result_size)
{
	_expand_code(data);
	_run_all_maps();

	auto carnival_flags = _decrypt_check<CHECK_CHECKSUMS, CARNIVAL_WORLD>();
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

	auto other_flags = _decrypt_check<CHECK_CHECKSUMS, OTHER_WORLD>();
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

void tm_avx2_m256_map_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
{
	for (uint32_t i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
		{
			return;
		}
		_run_bruteforce<true>(start_data + i, result_data, result_size);
		report_progress(static_cast<double>(i + 1) / static_cast<double>(amount_to_run));
	}
}

void tm_avx2_m256_map_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	uint8_t result_data[2];
	uint32_t result_pos = 0;
	_run_bruteforce<false>(data, result_data, &result_pos);
	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_avx2_m256_map_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	test_algorithm_n(algorithm_id, data, rng_seed, 1);
}

void tm_avx2_m256_map_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	load_data(data);

	if (algorithm_id == 0)
	{
		alg0_values_for_seeds_8 = rng->generate_alg0_values_for_seeds_8(rng_seed, 1);
	}
	else if (algorithm_id == 1 || algorithm_id == 2 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 5)
	{
		regular_rng_values_for_seeds_8 = rng->generate_regular_rng_values_for_seeds_8(rng_seed, 1);
	}
	else if (algorithm_id == 6)
	{
		alg6_values_for_seeds_8 = rng->generate_alg6_values_for_seeds_8(rng_seed, 1);
	}

	uint16_t local_pos = 2047;
	for (int i = 0; i < iterations; i++)
	{
		if (local_pos < 128)
			local_pos = 2047;
		_run_alg(algorithm_id, &local_pos,
			regular_rng_values_for_seeds_8.get(), alg0_values_for_seeds_8.get(),
			alg6_values_for_seeds_8.get());
	}

	fetch_data(data);
}

void tm_avx2_m256_map_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	fetch_data(result_out);
}

void tm_avx2_m256_map_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	_run_all_maps();
	fetch_data(result_out);
}

bool tm_avx2_m256_map_8::test_bruteforce_checksum(uint32_t data, int world)
{
	_expand_code(data);
	_run_all_maps();

	if (world == CARNIVAL_WORLD)
		return _decrypt_check<true, CARNIVAL_WORLD>().has_value();
	else
		return _decrypt_check<true, OTHER_WORLD>().has_value();
}

