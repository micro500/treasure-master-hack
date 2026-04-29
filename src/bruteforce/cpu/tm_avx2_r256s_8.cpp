#include "tm_avx2_r256s_8.h"



tm_avx2_r256s_8::tm_avx2_r256s_8(RNG* rng_obj) : tm_avx2_r256s_8(rng_obj, 0) {}

tm_avx2_r256s_8::tm_avx2_r256s_8(RNG* rng_obj, const uint32_t key) : tm_avx2_r256s_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx2_r256s_8::tm_avx2_r256s_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
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
}


__forceinline void tm_avx2_r256s_8::initialize()
{
	if (!_initialized)
	{
		auto _r0 = rng->generate_expansion_values_256_8_shuffled();
		auto _r1 = rng->generate_seed_forward_1();
		auto _r2 = rng->generate_seed_forward_128();
		auto _r3 = rng->generate_regular_rng_values_8();
		auto _r4 = rng->generate_regular_rng_values_256_8_shuffled();
		auto _r5 = rng->generate_alg0_values_256_8_shuffled();
		auto _r6 = rng->generate_alg2_values_8_8();
		auto _r7 = rng->generate_alg4_values_256_8_shuffled();
		auto _r8 = rng->generate_alg5_values_8_8();
		auto _r9 = rng->generate_alg6_values_256_8_shuffled();

		_expansion_256s = static_cast<uint8_t*>(_r0.get());
		_seed_fwd_1     = static_cast<uint16_t*>(_r1.get());
		_seed_fwd_128   = static_cast<uint16_t*>(_r2.get());
		_regular_8      = static_cast<uint8_t*>(_r3.get());
		_regular_256s   = static_cast<uint8_t*>(_r4.get());
		_alg0_256s      = static_cast<uint8_t*>(_r5.get());
		_alg2_8_8       = static_cast<uint8_t*>(_r6.get());
		_alg4_256s      = static_cast<uint8_t*>(_r7.get());
		_alg5_8_8       = static_cast<uint8_t*>(_r8.get());
		_alg6_256s      = static_cast<uint8_t*>(_r9.get());

		_table_refs = { _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7, _r8, _r9 };
		_initialized = true;
	}
	obj_name = "tm_avx2_r256s_8";
}

__forceinline void tm_avx2_r256s_8::_store_to_mem(WC_ARGS_256)
{
	_mm256_store_si256((__m256i*)(working_code_data), wc0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), wc1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), wc2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), wc3);
}

__forceinline void tm_avx2_r256s_8::_load_from_mem(WC_ARGS_256)
{
	wc0 = _mm256_load_si256((__m256i*)(working_code_data));
	wc1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	wc2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	wc3 = _mm256_load_si256((__m256i*)(working_code_data + 96));
}


__forceinline void tm_avx2_r256s_8::_expand_code(uint32_t data, WC_ARGS_256)
{
	__m128i a = _mm_insert_epi32(_mm_cvtsi32_si128(static_cast<int32_t>(data)), static_cast<int32_t>(key), 1);

	__m128i lo_mask = _mm_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m128i hi_mask = _mm_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);

	__m128i lo_128 = _mm_shuffle_epi8(a, lo_mask);
	__m128i hi_128 = _mm_shuffle_epi8(a, hi_mask);

	__m256i lo = _mm256_setr_m128i(lo_128, lo_128);
	__m256i hi = _mm256_setr_m128i(hi_128, hi_128);

	wc0 = lo;
	wc1 = hi;
	wc2 = lo;
	wc3 = hi;

	uint8_t* rng_start = _expansion_256s;
	uint16_t rng_seed = (key >> 16) & 0xFFFF;

	add_alg(wc0, wc1, wc2, wc3, &rng_seed, rng_start);
}

void tm_avx2_r256s_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8_t*)working_code_data)[shuffle_8(i, 256)] = new_data[i];
	}

}

void tm_avx2_r256s_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8_t*)working_code_data)[shuffle_8(i, 256)];
	}
}

__forceinline void tm_avx2_r256s_8::alg_0(WC_ARGS_256, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg0_256s + ((*rng_seed) * 128);

	wc0 = _mm256_slli_epi16(wc0, 1);
	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	wc0 = _mm256_and_si256(wc0, mask_FE);
	wc0 = _mm256_or_si256(wc0, rng_val);

	wc1 = _mm256_slli_epi16(wc1, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	wc1 = _mm256_and_si256(wc1, mask_FE);
	wc1 = _mm256_or_si256(wc1, rng_val);

	wc2 = _mm256_slli_epi16(wc2, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	wc2 = _mm256_and_si256(wc2, mask_FE);
	wc2 = _mm256_or_si256(wc2, rng_val);

	wc3 = _mm256_slli_epi16(wc3, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	wc3 = _mm256_and_si256(wc3, mask_FE);
	wc3 = _mm256_or_si256(wc3, rng_val);
}

__forceinline void tm_avx2_r256s_8::alg_2_sub(__m256i& working_a, __m256i& working_b, __m256i& carry)
{
	// bitwise right shift
	__m256i temp1 = _mm256_srli_epi16(working_a, 1);
	// Mask off top bits
	__m256i cur_val1_most = _mm256_and_si256(temp1, mask_7F);

	// Mask off the top bits
	__m256i cur_val2_masked = _mm256_and_si256(working_b, mask_80);

	// bytewise right shift
	__m256i bridge = _mm256_permute2x128_si256(working_a, working_a, 0x81);
	__m256i cur_val1_srl = _mm256_alignr_epi8(bridge, working_a, 1);
	// mask off only the relevant low bit
	__m256i cur_val1_bit = _mm256_and_si256(cur_val1_srl, mask_01);
	// add the carry to the top
	cur_val1_bit = _mm256_or_si256(cur_val1_bit, carry);

	// bitwise right shift
	__m256i temp2 = _mm256_slli_epi16(working_b, 1);
	// mask off lowest bit
	__m256i cur_val2_most = _mm256_and_si256(temp2, mask_FE);

	// Save the next carry
	uint8_t bit0 = _mm256_extract_epi8(working_a, 0) & 0x01;
	__m256i next_carry = _mm256_insert_epi8(_mm256_setzero_si256(), bit0, 31);

	working_a = _mm256_or_si256(cur_val1_most, cur_val2_masked);
	working_b = _mm256_or_si256(cur_val2_most, cur_val1_bit);

	carry = next_carry;
}

__forceinline void tm_avx2_r256s_8::alg_2(WC_ARGS_256, uint16_t* rng_seed)
{
	__m256i carry = _mm256_and_si256(
		_mm256_set1_epi8(static_cast<int8_t>(_alg2_8_8[*rng_seed])),
		mask_top_01);

	alg_2_sub(wc2, wc3, carry);
	alg_2_sub(wc0, wc1, carry);
}

__forceinline void tm_avx2_r256s_8::alg_3(WC_ARGS_256, uint16_t* rng_seed)
{
	uint8_t* rng_start = _regular_256s + ((*rng_seed) * 128);

	xor_alg(WC_PASS_256, rng_start);
}

__forceinline void tm_avx2_r256s_8::alg_5_sub(__m256i& working_a, __m256i& working_b, __m256i& carry)
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

	working_a = _mm256_or_si256(cur_val1_most, cur_val2_masked);
	working_b = _mm256_or_si256(cur_val2_most, cur_val1_srl_w_carry);

	carry = next_carry;
}

__forceinline void tm_avx2_r256s_8::alg_5(WC_ARGS_256, uint16_t* rng_seed)
{
	__m256i carry = _mm256_and_si256(
		_mm256_set1_epi8(static_cast<int8_t>(_alg5_8_8[*rng_seed])),
		mask_top_80);

	alg_5_sub(wc2, wc3, carry);
	alg_5_sub(wc0, wc1, carry);
}

__forceinline void tm_avx2_r256s_8::alg_6(WC_ARGS_256, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg6_256s + ((*rng_seed) * 128);

	wc0 = _mm256_srli_epi16(wc0, 1);
	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	wc0 = _mm256_and_si256(wc0, mask_7F);
	wc0 = _mm256_or_si256(wc0, rng_val);

	wc1 = _mm256_srli_epi16(wc1, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	wc1 = _mm256_and_si256(wc1, mask_7F);
	wc1 = _mm256_or_si256(wc1, rng_val);

	wc2 = _mm256_srli_epi16(wc2, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	wc2 = _mm256_and_si256(wc2, mask_7F);
	wc2 = _mm256_or_si256(wc2, rng_val);

	wc3 = _mm256_srli_epi16(wc3, 1);
	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	wc3 = _mm256_and_si256(wc3, mask_7F);
	wc3 = _mm256_or_si256(wc3, rng_val);
}

__forceinline void tm_avx2_r256s_8::alg_7(WC_ARGS_256)
{
	wc0 = _mm256_xor_si256(wc0, mask_FF);
	wc1 = _mm256_xor_si256(wc1, mask_FF);
	wc2 = _mm256_xor_si256(wc2, mask_FF);
	wc3 = _mm256_xor_si256(wc3, mask_FF);
}

__forceinline void tm_avx2_r256s_8::add_alg(WC_ARGS_256, uint16_t* rng_seed, uint8_t* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);

	__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start));
	wc0 = _mm256_add_epi8(wc0, rng_val);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 32));
	wc1 = _mm256_add_epi8(wc1, rng_val);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 64));
	wc2 = _mm256_add_epi8(wc2, rng_val);

	rng_val = _mm256_load_si256((__m256i*)(rng_start + 96));
	wc3 = _mm256_add_epi8(wc3, rng_val);
}

void tm_avx2_r256s_8::_run_one_map(WC_ARGS_256, const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16_t rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
	uint16_t nibble_selector = schedule_entry.nibble_selector;

	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		_mm256_store_si256((__m256i*)(working_code_data), wc0);
		_mm256_store_si256((__m256i*)(working_code_data + 32), wc1);

		// Get the highest bit of the nibble selector to use as a flag
		unsigned char nibble = (nibble_selector >> 15) & 0x01;
		// Shift the nibble selector up one bit
		nibble_selector = nibble_selector << 1;

		// If the flag is a 1, get the high nibble of the current byte
		// Otherwise use the low nibble
		unsigned char current_byte = (uint8_t)((uint8_t*)working_code_data)[shuffle_8(i, 256)];

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char algorithm_id = (current_byte >> 1) & 0x07;

		_run_alg(WC_PASS_256, algorithm_id, &rng_seed);
	}
}

__forceinline void tm_avx2_r256s_8::_run_alg(WC_ARGS_256, int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_256, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		uint8_t* rng_start = _regular_256s;

		if (algorithm_id == 4)
		{
			rng_start = _alg4_256s;
		}

		add_alg(WC_PASS_256, rng_seed, rng_start);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS_256, rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS_256, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS_256, rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS_256, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS_256);
	}
}

__forceinline void tm_avx2_r256s_8::_run_all_maps(WC_ARGS_256)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries->entries.begin(); it != schedule_entries->entries.end(); it++)
	{
		_run_one_map(WC_PASS_256, *it);
	}
}

void tm_avx2_r256s_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	WC_VARS_256;
	load_data(data);
	_load_from_mem(WC_PASS_256);
	_run_alg(WC_PASS_256, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_256);
	fetch_data(data);
}

void tm_avx2_r256s_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_256;
	load_data(data);
	_load_from_mem(WC_PASS_256);
	for (int i = 0; i < iterations; i++)
		_run_alg(WC_PASS_256, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_256);
	fetch_data(data);
}

void tm_avx2_r256s_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

void tm_avx2_r256s_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_run_all_maps(WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx2_r256s_8::_decrypt_check(WC_ARGS_256)
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


bool tm_avx2_r256s_8::test_bruteforce_checksum(uint32_t data, int world)
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

__forceinline void tm_avx2_r256s_8::xor_alg(WC_ARGS_256, uint8_t* values)
{
	wc0 = _mm256_xor_si256(wc0, _mm256_load_si256((__m256i*)(values)));
	wc1 = _mm256_xor_si256(wc1, _mm256_load_si256((__m256i*)(values + 32)));
	wc2 = _mm256_xor_si256(wc2, _mm256_load_si256((__m256i*)(values + 64)));
	wc3 = _mm256_xor_si256(wc3, _mm256_load_si256((__m256i*)(values + 96)));
}


__forceinline void tm_avx2_r256s_8::_decrypt_carnival_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, carnival_world_data_shuffled);
}

__forceinline void tm_avx2_r256s_8::_decrypt_other_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, other_world_data_shuffled);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx2_r256s_8::_run_bruteforce(WC_ARGS_256, uint32_t data, uint8_t* result_data, uint32_t* result_size)
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

__forceinline void tm_avx2_r256s_8::mid_sum(
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

__forceinline uint16_t tm_avx2_r256s_8::masked_checksum(WC_ARGS_256, uint8_t* mask)
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

__forceinline uint16_t tm_avx2_r256s_8::_calculate_carnival_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, carnival_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx2_r256s_8::_calculate_other_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, other_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx2_r256s_8::fetch_checksum_value(WC_ARGS_256, uint8_t code_length)
{
	_store_to_mem(WC_PASS_256);
	uint8_t lo = working_code_data[shuffle_8(127 - code_length, 256)];
	uint8_t hi = working_code_data[shuffle_8(127 - (code_length + 1), 256)];
	return (uint16_t)((hi << 8) | lo);
}

__forceinline uint16_t tm_avx2_r256s_8::_fetch_carnival_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx2_r256s_8::_fetch_other_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx2_r256s_8::check_carnival_world_checksum(WC_ARGS_256)
{
	return _calculate_carnival_world_checksum(WC_PASS_256)
		== _fetch_carnival_world_checksum_value(WC_PASS_256);
}

__forceinline bool tm_avx2_r256s_8::check_other_world_checksum(WC_ARGS_256)
{
	return _calculate_other_world_checksum(WC_PASS_256)
		== _fetch_other_world_checksum_value(WC_PASS_256);
}


void tm_avx2_r256s_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_256;
	uint8_t result_data[2];
	uint32_t result_pos = 0;
	_run_bruteforce<false>(WC_PASS_256, data, result_data, &result_pos);
	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}
void tm_avx2_r256s_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
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


