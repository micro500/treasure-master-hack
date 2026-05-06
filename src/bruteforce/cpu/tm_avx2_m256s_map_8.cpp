#include "tm_avx2_m256s_map_8.h"

tm_avx2_m256s_map_8::tm_avx2_m256s_map_8(RNG* rng_obj) : tm_avx2_m256s_map_8(rng_obj, 0) {}

tm_avx2_m256s_map_8::tm_avx2_m256s_map_8(RNG* rng_obj, const uint32_t key) : tm_avx2_m256s_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx2_m256s_map_8::tm_avx2_m256s_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj),
	mask_FF(_mm256_set1_epi8(static_cast<int8_t>(0xFF))),
	mask_FE(_mm256_set1_epi8(static_cast<int8_t>(0xFE))),
	mask_7F(_mm256_set1_epi8(0x7F)),
	mask_80(_mm256_set1_epi8(static_cast<int8_t>(0x80))),
	mask_01(_mm256_set1_epi8(0x01)),
	mask_top_01(_mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
	mask_top_80(_mm256_set_epi16(static_cast<int16_t>(0x8000), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
	mask_00FF(_mm256_set1_epi16(0x00FF))
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;

	shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 256, false);
	shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 256, false);
	shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 256, false);
	shuffle_mem(other_world_data, other_world_data_shuffled, 256, false);

	generate_map_rng();
}

tm_avx2_m256s_map_8::~tm_avx2_m256s_map_8()
{
}

__forceinline void tm_avx2_m256s_map_8::initialize()
{
	if (!_initialized)
	{
		_table_refs.push_back(rng->generate_seed_forward_1());
		_table_refs.push_back(rng->generate_seed_forward_128());

		_initialized = true;
	}
	obj_name = "tm_avx2_m256s_map_8";
}

void tm_avx2_m256s_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(expansion_values_for_seed_256_8_shuffled, (key >> 16) & 0xFFFF, false, 256);
	regular_rng_values_for_seeds_8 = rng->generate_regular_rng_values_for_seeds_8(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	alg0_values_for_seeds_8 = rng->generate_alg0_values_for_seeds_8(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	alg6_values_for_seeds_8 = rng->generate_alg6_values_for_seeds_8(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
}

void tm_avx2_m256s_map_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8_t*)working_code_data)[shuffle_8(i, 256)] = new_data[i];
	}
}

void tm_avx2_m256s_map_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8_t*)working_code_data)[shuffle_8(i, 256)];
	}
}

void tm_avx2_m256s_map_8::test_algorithm_chain(const uint8_t* algorithm_ids, int chain_length,
                                               uint8_t* data, uint16_t* rng_seed)
{
	load_data(data);

	alg0_values_for_seeds_8 = rng->generate_alg0_values_for_seeds_8(rng_seed, 1);
	regular_rng_values_for_seeds_8 = rng->generate_regular_rng_values_for_seeds_8(rng_seed, 1);
	alg6_values_for_seeds_8 = rng->generate_alg6_values_for_seeds_8(rng_seed, 1);

	const uint8_t* reg_base = regular_rng_values_for_seeds_8.get();
	const uint8_t* alg0_base = alg0_values_for_seeds_8.get();
	const uint8_t* alg6_base = alg6_values_for_seeds_8.get();
	uint16_t local_pos = 2047;
	for (int i = 0; i < chain_length; ++i)
	{
		_run_alg(algorithm_ids[i], &local_pos, reg_base, alg0_base, alg6_base);
	}

	fetch_data(data);
}

void tm_avx2_m256s_map_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
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

	load_data(data);
	const uint8_t* reg_base = regular_rng_values_for_seeds_8.get();
	const uint8_t* alg0_base = alg0_values_for_seeds_8.get();
	const uint8_t* alg6_base = alg6_values_for_seeds_8.get();
	uint16_t local_pos = 2047;
	for (int i = 0; i < iterations; i++)
	{
		if (local_pos < 128)
			local_pos = 2047;
		_run_alg(algorithm_id, &local_pos, reg_base, alg0_base, alg6_base);
	}
	fetch_data(data);
}

void tm_avx2_m256s_map_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	fetch_data(result_out);
}

void tm_avx2_m256s_map_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	_run_all_maps();
	fetch_data(result_out);
}

bool tm_avx2_m256s_map_8::test_bruteforce_checksum(uint32_t data, int world)
{
	_expand_code(data);
	_run_all_maps();
	if (world == CARNIVAL_WORLD)
		return _decrypt_check<true, CARNIVAL_WORLD>().has_value();
	else
		return _decrypt_check<true, OTHER_WORLD>().has_value();
}

void tm_avx2_m256s_map_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
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

void tm_avx2_m256s_map_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	uint8_t result_data[2];
	uint32_t result_pos = 0;
	_run_bruteforce<false>(data, result_data, &result_pos);
	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

__forceinline void tm_avx2_m256s_map_8::_expand_code(uint32_t data)
{
	__m128i a = _mm_insert_epi32(_mm_cvtsi32_si128(static_cast<int32_t>(data)), static_cast<int32_t>(key), 1);
	__m128i lo_mask = _mm_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m128i hi_mask = _mm_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);
	__m128i lo = _mm_shuffle_epi8(a, lo_mask);
	__m128i hi = _mm_shuffle_epi8(a, hi_mask);
	__m256i lo_wide = _mm256_setr_m128i(lo, lo);
	__m256i hi_wide = _mm256_setr_m128i(hi, hi);
	for (int i = 0; i < 2; i++)
	{
		_mm256_store_si256((__m256i*)(working_code_data + i * 64), lo_wide);
		_mm256_store_si256((__m256i*)(working_code_data + i * 64 + 32), hi_wide);
	}
	uint8_t* rng_start = expansion_values_for_seed_256_8_shuffled.get();
	uint16_t rng_seed = (key >> 16) & 0xFFFF;
	add_alg(rng_start);
}

__forceinline void tm_avx2_m256s_map_8::add_alg(const uint8_t* rng_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(rng_start, r0, r1, r2, r3);
	_mm256_store_si256((__m256i*)(working_code_data),      _mm256_add_epi8(_mm256_load_si256((__m256i*)(working_code_data)),      r0));
	_mm256_store_si256((__m256i*)(working_code_data + 32), _mm256_add_epi8(_mm256_load_si256((__m256i*)(working_code_data + 32)), r1));
	_mm256_store_si256((__m256i*)(working_code_data + 64), _mm256_add_epi8(_mm256_load_si256((__m256i*)(working_code_data + 64)), r2));
	_mm256_store_si256((__m256i*)(working_code_data + 96), _mm256_add_epi8(_mm256_load_si256((__m256i*)(working_code_data + 96)), r3));
}

__forceinline void tm_avx2_m256s_map_8::sub_alg(const uint8_t* rng_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(rng_start, r0, r1, r2, r3);
	_mm256_store_si256((__m256i*)(working_code_data),      _mm256_sub_epi8(_mm256_load_si256((__m256i*)(working_code_data)),      r0));
	_mm256_store_si256((__m256i*)(working_code_data + 32), _mm256_sub_epi8(_mm256_load_si256((__m256i*)(working_code_data + 32)), r1));
	_mm256_store_si256((__m256i*)(working_code_data + 64), _mm256_sub_epi8(_mm256_load_si256((__m256i*)(working_code_data + 64)), r2));
	_mm256_store_si256((__m256i*)(working_code_data + 96), _mm256_sub_epi8(_mm256_load_si256((__m256i*)(working_code_data + 96)), r3));
}

__forceinline void tm_avx2_m256s_map_8::xor_alg(const uint8_t* values)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(values, r0, r1, r2, r3);
	_mm256_store_si256((__m256i*)(working_code_data),      _mm256_xor_si256(_mm256_load_si256((__m256i*)(working_code_data)),      r0));
	_mm256_store_si256((__m256i*)(working_code_data + 32), _mm256_xor_si256(_mm256_load_si256((__m256i*)(working_code_data + 32)), r1));
	_mm256_store_si256((__m256i*)(working_code_data + 64), _mm256_xor_si256(_mm256_load_si256((__m256i*)(working_code_data + 64)), r2));
	_mm256_store_si256((__m256i*)(working_code_data + 96), _mm256_xor_si256(_mm256_load_si256((__m256i*)(working_code_data + 96)), r3));
}

__forceinline void tm_avx2_m256s_map_8::_load_fwd(const uint8_t* block_start, __m256i& r0, __m256i& r1, __m256i& r2, __m256i& r3)
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

__forceinline void tm_avx2_m256s_map_8::alg_0(const uint8_t* block_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(block_start, r0, r1, r2, r3);
	__m256i wc0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i wc1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i wc2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i wc3 = _mm256_load_si256((__m256i*)(working_code_data + 96));
	_mm256_store_si256((__m256i*)(working_code_data),      _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc0, 1), mask_FE), r0));
	_mm256_store_si256((__m256i*)(working_code_data + 32), _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc1, 1), mask_FE), r1));
	_mm256_store_si256((__m256i*)(working_code_data + 64), _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc2, 1), mask_FE), r2));
	_mm256_store_si256((__m256i*)(working_code_data + 96), _mm256_or_si256(_mm256_and_si256(_mm256_slli_epi16(wc3, 1), mask_FE), r3));
}

__forceinline void tm_avx2_m256s_map_8::alg_2_sub(__m256i& working_a, __m256i& working_b, __m256i& carry)
{
	__m256i temp1 = _mm256_srli_epi16(working_a, 1);
	__m256i cur_val1_most = _mm256_and_si256(temp1, mask_7F);
	__m256i cur_val2_masked = _mm256_and_si256(working_b, mask_80);
	__m256i bridge = _mm256_permute2x128_si256(working_a, working_a, 0x81);
	__m256i cur_val1_srl = _mm256_alignr_epi8(bridge, working_a, 1);
	__m256i cur_val1_bit = _mm256_and_si256(cur_val1_srl, mask_01);
	cur_val1_bit = _mm256_or_si256(cur_val1_bit, carry);
	__m256i temp2 = _mm256_slli_epi16(working_b, 1);
	__m256i cur_val2_most = _mm256_and_si256(temp2, mask_FE);
	uint8_t bit0 = _mm256_extract_epi8(working_a, 0) & 0x01;
	__m256i next_carry = _mm256_insert_epi8(_mm256_setzero_si256(), bit0, 31);
	working_a = _mm256_or_si256(cur_val1_most, cur_val2_masked);
	working_b = _mm256_or_si256(cur_val2_most, cur_val1_bit);
	carry = next_carry;
}

__forceinline void tm_avx2_m256s_map_8::alg_2(uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_01);
	for (int i = 3; i >= 0; i -= 2)
	{
		__m256i wc_a = _mm256_load_si256((__m256i*)(working_code_data + (i-1) * 32));
		__m256i wc_b = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		alg_2_sub(wc_a, wc_b, carry);
		_mm256_store_si256((__m256i*)(working_code_data + (i-1) * 32), wc_a);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), wc_b);
	}
}

__forceinline void tm_avx2_m256s_map_8::alg_5_sub(__m256i& working_a, __m256i& working_b, __m256i& carry)
{
	__m256i cur_val1_most = _mm256_and_si256(_mm256_slli_epi16(working_a, 1), mask_FE);
	__m256i cur_val2_most = _mm256_and_si256(_mm256_srli_epi16(working_b, 1), mask_7F);
	__m256i cur_val2_masked = _mm256_and_si256(working_b, mask_01);
	__m256i cur_val1_bit = _mm256_and_si256(working_a, mask_80);
	__m256i mask_val = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(3, 0, 0, 3));
	__m256i cur_val1_srl = _mm256_alignr_epi8(mask_val, cur_val1_bit, 1);
	__m256i cur_val1_srl_w_carry = _mm256_or_si256(cur_val1_srl, carry);
	__m256i lo_to_hi = _mm256_permute2x128_si256(cur_val1_bit, cur_val1_bit, _MM_SHUFFLE(0, 0, 3, 0));
	__m256i next_carry = _mm256_bslli_epi128(lo_to_hi, 15);
	working_a = _mm256_or_si256(cur_val1_most, cur_val2_masked);
	working_b = _mm256_or_si256(cur_val2_most, cur_val1_srl_w_carry);
	carry = next_carry;
}

__forceinline void tm_avx2_m256s_map_8::alg_5(uint8_t carry_byte)
{
	__m256i carry = _mm256_and_si256(_mm256_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_80);
	for (int i = 3; i >= 0; i -= 2)
	{
		__m256i wc_a = _mm256_load_si256((__m256i*)(working_code_data + (i-1) * 32));
		__m256i wc_b = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		alg_5_sub(wc_a, wc_b, carry);
		_mm256_store_si256((__m256i*)(working_code_data + (i-1) * 32), wc_a);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), wc_b);
	}
}

__forceinline void tm_avx2_m256s_map_8::alg_6(const uint8_t* block_start)
{
	__m256i r0, r1, r2, r3;
	_load_fwd(block_start, r0, r1, r2, r3);
	__m256i wc0 = _mm256_load_si256((__m256i*)(working_code_data));
	__m256i wc1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	__m256i wc2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	__m256i wc3 = _mm256_load_si256((__m256i*)(working_code_data + 96));
	_mm256_store_si256((__m256i*)(working_code_data),      _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc0, 1), mask_7F), r0));
	_mm256_store_si256((__m256i*)(working_code_data + 32), _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc1, 1), mask_7F), r1));
	_mm256_store_si256((__m256i*)(working_code_data + 64), _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc2, 1), mask_7F), r2));
	_mm256_store_si256((__m256i*)(working_code_data + 96), _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi16(wc3, 1), mask_7F), r3));
}

__forceinline void tm_avx2_m256s_map_8::alg_7()
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_xor_si256(cur_val, mask_FF));
	}
}

__forceinline void tm_avx2_m256s_map_8::_run_alg(int algorithm_id, uint16_t* local_pos,
	const uint8_t* reg_base, const uint8_t* alg0_base, const uint8_t* alg6_base)
{
	if (algorithm_id == 0)
	{
		alg_0(alg0_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 1)
	{
		add_alg(reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 2)
	{
		alg_2(reg_base[*local_pos] >> 7);
		*local_pos -= 1;
	}
	else if (algorithm_id == 3)
	{
		xor_alg(reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 4)
	{
		sub_alg(reg_base + *local_pos - 127);
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

__forceinline void tm_avx2_m256s_map_8::_run_one_map(int map_idx)
{
	uint16_t nibble_selector = schedule_entries->entries[static_cast<size_t>(map_idx)].nibble_selector;
	const uint8_t* reg_base = regular_rng_values_for_seeds_8.get() + map_idx * 2048;
	const uint8_t* alg0_base = alg0_values_for_seeds_8.get() + map_idx * 2048;
	const uint8_t* alg6_base = alg6_values_for_seeds_8.get() + map_idx * 2048;
	uint16_t local_pos = 2047;

	for (int i = 0; i < 16; i++)
	{
		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);
		uint8_t current_byte = working_code_data[shuffle_8(i, 256)];
		if (nibble == 1) current_byte = static_cast<uint8_t>(current_byte >> 4);
		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);
		_run_alg(algorithm_id, &local_pos, reg_base, alg0_base, alg6_base);
	}
}

__forceinline void tm_avx2_m256s_map_8::_run_all_maps()
{
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
		_run_one_map(map_idx);
}

__forceinline void tm_avx2_m256s_map_8::_decrypt_carnival_world()
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_load_si256((__m256i*)(carnival_world_data_shuffled + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_xor_si256(cur_val, rng_val));
	}
}

__forceinline void tm_avx2_m256s_map_8::_decrypt_other_world()
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_load_si256((__m256i*)(other_world_data_shuffled + i * 32));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_xor_si256(cur_val, rng_val));
	}
}

__forceinline void tm_avx2_m256s_map_8::mid_sum(__m128i& sum, __m256i& working_code, __m256i& sum_mask)
{
	__m256i temp_masked = _mm256_and_si256(working_code, sum_mask);
	__m256i temp1_lo = _mm256_and_si256(temp_masked, mask_00FF);
	__m256i temp1_hi = _mm256_srli_epi16(temp_masked, 8);
	__m128i lo_128 = _mm256_castsi256_si128(temp1_lo);
	__m128i hi_128 = _mm256_extracti128_si256(temp1_lo, 1);
	sum = _mm_add_epi16(sum, lo_128);
	sum = _mm_add_epi16(sum, hi_128);
	lo_128 = _mm256_castsi256_si128(temp1_hi);
	hi_128 = _mm256_extracti128_si256(temp1_hi, 1);
	sum = _mm_add_epi16(sum, lo_128);
	sum = _mm_add_epi16(sum, hi_128);
}

__forceinline uint16_t tm_avx2_m256s_map_8::masked_checksum(uint8_t* mask)
{
	__m128i sum = _mm_setzero_si128();
	for (int i = 0; i < 4; i++)
	{
		__m256i working_code = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i sum_mask = _mm256_load_si256((__m256i*)(mask + i * 32));
		mid_sum(sum, working_code, sum_mask);
	}
	return static_cast<uint16_t>(_mm_extract_epi16(sum, 0) + _mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) + _mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) + _mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) + _mm_extract_epi16(sum, 7));
}

__forceinline uint16_t tm_avx2_m256s_map_8::_calculate_carnival_world_checksum()
{
	return masked_checksum(carnival_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx2_m256s_map_8::_calculate_other_world_checksum()
{
	return masked_checksum(other_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx2_m256s_map_8::_fetch_carnival_world_checksum_value()
{
	unsigned char checksum_low = working_code_data[shuffle_8(127 - (CARNIVAL_WORLD_CODE_LENGTH - 2), 256)];
	unsigned char checksum_hi = working_code_data[shuffle_8(127 - (CARNIVAL_WORLD_CODE_LENGTH - 2 + 1), 256)];
	return static_cast<uint16_t>((checksum_hi << 8) | checksum_low);
}

__forceinline uint16_t tm_avx2_m256s_map_8::_fetch_other_world_checksum_value()
{
	unsigned char checksum_low = working_code_data[shuffle_8(127 - (OTHER_WORLD_CODE_LENGTH - 2), 256)];
	unsigned char checksum_hi = working_code_data[shuffle_8(127 - (OTHER_WORLD_CODE_LENGTH - 2 + 1), 256)];
	return static_cast<uint16_t>((checksum_hi << 8) | checksum_low);
}

__forceinline bool tm_avx2_m256s_map_8::check_carnival_world_checksum()
{
	return _calculate_carnival_world_checksum() == _fetch_carnival_world_checksum_value();
}

__forceinline bool tm_avx2_m256s_map_8::check_other_world_checksum()
{
	return _calculate_other_world_checksum() == _fetch_other_world_checksum_value();
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx2_m256s_map_8::_decrypt_check()
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

	uint8_t unshuffled_data[128];
	unshuffle_mem(working_code_data, unshuffled_data, 256, false);
	uint8_t result = check_machine_code(unshuffled_data, WORLD);
	for (int i = 0; i < 128; i++)
		working_code_data[i] = saved_data[i];
	return result;
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx2_m256s_map_8::_run_bruteforce(uint32_t data, uint8_t* result_data, uint32_t* result_size)
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


void tm_avx2_m256s_map_8::set_key(uint32_t new_key)
{
	TM_base::set_key(new_key);
	generate_map_rng();
}
