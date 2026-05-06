#include "tm_avx512bwvl_r512s_map_8.h"

tm_avx512bwvl_r512s_map_8::tm_avx512bwvl_r512s_map_8(RNG* rng_obj) : tm_avx512bwvl_r512s_map_8(rng_obj, 0) {}

tm_avx512bwvl_r512s_map_8::tm_avx512bwvl_r512s_map_8(RNG* rng_obj, const uint32_t key) : tm_avx512bwvl_r512s_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx512bwvl_r512s_map_8::tm_avx512bwvl_r512s_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
	: TM_base(rng_obj),
	  mask_FF(_mm512_set1_epi8(static_cast<int8_t>(0xFF))),
	  mask_FE(_mm512_set1_epi8(static_cast<int8_t>(0xFE))),
	  mask_7F(_mm512_set1_epi8(0x7F)),
	  mask_80(_mm512_set1_epi8(static_cast<int8_t>(0x80))),
	  mask_01(_mm512_set1_epi8(0x01)),
	  mask_top_01(_mm512_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
	  mask_top_80(_mm512_set_epi16(static_cast<int16_t>(0x8000), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;

	generate_map_rng();

	shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 512, false);
	shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 512, false);
	shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 512, false);
	shuffle_mem(other_world_data, other_world_data_shuffled, 512, false);
}

tm_avx512bwvl_r512s_map_8::~tm_avx512bwvl_r512s_map_8()
{
}

__forceinline void tm_avx512bwvl_r512s_map_8::initialize()
{
	if (!_initialized)
	{
		_initialized = true;
	}
	obj_name = "tm_avx512bwvl_r512s_map_8";
}

void tm_avx512bwvl_r512s_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(expansion_values_for_seed_128_8, (key >> 16) & 0xFFFF, true, 512);
	regular_rng_values_for_seeds_512_8_shuffled = rng->generate_regular_rng_values_for_seeds(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count, false);
	alg0_values_for_seeds_512_8_shuffled = rng->generate_alg0_values_for_seeds(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count, false);
	alg6_values_for_seeds_512_8_shuffled = rng->generate_alg6_values_for_seeds(const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count, false);
}

__forceinline void tm_avx512bwvl_r512s_map_8::_load_from_mem(WC_ARGS_512)
{
	wc0 = _mm512_loadu_si512(working_code_data);
	wc1 = _mm512_loadu_si512(working_code_data + 64);
}

__forceinline void tm_avx512bwvl_r512s_map_8::_store_to_mem(WC_ARGS_512)
{
	_mm512_storeu_si512(working_code_data, wc0);
	_mm512_storeu_si512(working_code_data + 64, wc1);
}

void tm_avx512bwvl_r512s_map_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8_t*)working_code_data)[shuffle_8(i, 512)] = new_data[i];
	}
}

void tm_avx512bwvl_r512s_map_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8_t*)working_code_data)[shuffle_8(i, 512)];
	}
}

__forceinline void tm_avx512bwvl_r512s_map_8::_expand_code(uint32_t data, WC_ARGS_512)
{
	uint64_t x = ((uint64_t)key << 32) | data;

	__m512i a = _mm512_set1_epi64(static_cast<int64_t>(x));
	__m512i lo_mask = _mm512_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m512i hi_mask = _mm512_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);

	__m512i lo = _mm512_shuffle_epi8(a, lo_mask);
	__m512i hi = _mm512_shuffle_epi8(a, hi_mask);

	wc0 = lo;
	wc1 = hi;

	uint8_t* rng_start = expansion_values_for_seed_128_8.get();

	wc0 = _mm512_add_epi8(wc0, _mm512_loadu_si512(rng_start));
	wc1 = _mm512_add_epi8(wc1, _mm512_loadu_si512(rng_start + 64));
}

__forceinline void tm_avx512bwvl_r512s_map_8::_load_deinterleave(const uint8_t* block_start, __m512i& r0, __m512i& r1)
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
	__m128i e = _mm_loadu_si128((const __m128i*)(block_start + 64));
	__m128i f = _mm_loadu_si128((const __m128i*)(block_start + 80));
	__m128i g = _mm_loadu_si128((const __m128i*)(block_start + 96));
	__m128i h = _mm_loadu_si128((const __m128i*)(block_start + 112));

	__m256i lo_even = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(c, sel_even), _mm_shuffle_epi8(d, sel_even)));
	__m256i hi_even = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(e, sel_even), _mm_shuffle_epi8(f, sel_even)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(g, sel_even), _mm_shuffle_epi8(h, sel_even)));
	r0 = _mm512_inserti64x4(_mm512_castsi256_si512(lo_even), hi_even, 1);

	__m256i lo_odd = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd), _mm_shuffle_epi8(b, sel_odd)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(c, sel_odd), _mm_shuffle_epi8(d, sel_odd)));
	__m256i hi_odd = _mm256_setr_m128i(
		_mm_unpacklo_epi64(_mm_shuffle_epi8(e, sel_odd), _mm_shuffle_epi8(f, sel_odd)),
		_mm_unpacklo_epi64(_mm_shuffle_epi8(g, sel_odd), _mm_shuffle_epi8(h, sel_odd)));
	r1 = _mm512_inserti64x4(_mm512_castsi256_si512(lo_odd), hi_odd, 1);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_0(WC_ARGS_512, const uint8_t* block_start)
{
	__m512i r0, r1;
	_load_deinterleave(block_start, r0, r1);
	wc0 = _mm512_ternarylogic_epi32(_mm512_slli_epi64(wc0, 1), mask_FE, r0, 0xEA);
	wc1 = _mm512_ternarylogic_epi32(_mm512_slli_epi64(wc1, 1), mask_FE, r1, 0xEA);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_1(WC_ARGS_512, const uint8_t* block_start)
{
	__m512i r0, r1;
	_load_deinterleave(block_start, r0, r1);
	wc0 = _mm512_add_epi8(wc0, r0);
	wc1 = _mm512_add_epi8(wc1, r1);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_2_sub(__m512i& working_a, __m512i& working_b, __m512i& carry)
{
	__m512i cur_val1_most = _mm512_and_si512(_mm512_srli_epi64(working_a, 1), mask_7F);
	__m512i cur_val2_most = _mm512_and_si512(_mm512_slli_epi64(working_b, 1), mask_FE);

	__m512i cur_val2_masked = _mm512_and_si512(working_b, mask_80);

	__m512i cur_val1_bit = _mm512_and_si512(working_a, mask_01);

	__m512i mask = _mm512_maskz_permutexvar_epi32(_cvtu32_mask16(0x0FFF), _mm512_set_epi32(0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4), cur_val1_bit);
	__m512i cur_val1_srl = _mm512_alignr_epi8(mask, cur_val1_bit, 1);
	__m512i cur_val1_srl_w_carry = _mm512_or_si512(cur_val1_srl, carry);

	working_a = _mm512_or_si512(cur_val1_most, cur_val2_masked);
	working_b = _mm512_or_si512(cur_val2_most, cur_val1_srl_w_carry);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_2(WC_ARGS_512, uint8_t carry_byte)
{
	__m512i carry = _mm512_and_si512(_mm512_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_01);
	alg_2_sub(wc0, wc1, carry);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_3(WC_ARGS_512, const uint8_t* block_start)
{
	__m512i r0, r1;
	_load_deinterleave(block_start, r0, r1);
	wc0 = _mm512_xor_si512(wc0, r0);
	wc1 = _mm512_xor_si512(wc1, r1);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_4(WC_ARGS_512, const uint8_t* block_start)
{
	__m512i r0, r1;
	_load_deinterleave(block_start, r0, r1);
	wc0 = _mm512_sub_epi8(wc0, r0);
	wc1 = _mm512_sub_epi8(wc1, r1);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_5_sub(__m512i& working_a, __m512i& working_b, __m512i& carry)
{
	__m512i cur_val1_most = _mm512_and_si512(_mm512_slli_epi64(working_a, 1), mask_FE);
	__m512i cur_val2_most = _mm512_and_si512(_mm512_srli_epi64(working_b, 1), mask_7F);

	__m512i cur_val2_masked = _mm512_and_si512(working_b, mask_01);

	__m512i cur_val1_bit = _mm512_and_si512(working_a, mask_80);

	__m512i mask = _mm512_maskz_permutexvar_epi32(_cvtu32_mask16(0x0FFF), _mm512_set_epi32(0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4), cur_val1_bit);
	__m512i cur_val1_srl = _mm512_alignr_epi8(mask, cur_val1_bit, 1);
	__m512i cur_val1_srl_w_carry = _mm512_or_si512(cur_val1_srl, carry);

	working_a = _mm512_or_si512(cur_val1_most, cur_val2_masked);
	working_b = _mm512_or_si512(cur_val2_most, cur_val1_srl_w_carry);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_5(WC_ARGS_512, uint8_t carry_byte)
{
	__m512i carry = _mm512_and_si512(_mm512_set1_epi8(static_cast<int8_t>(carry_byte)), mask_top_80);
	alg_5_sub(wc0, wc1, carry);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_6(WC_ARGS_512, const uint8_t* block_start)
{
	__m512i r0, r1;
	_load_deinterleave(block_start, r0, r1);
	wc0 = _mm512_ternarylogic_epi32(_mm512_srli_epi64(wc0, 1), mask_7F, r0, 0xEA);
	wc1 = _mm512_ternarylogic_epi32(_mm512_srli_epi64(wc1, 1), mask_7F, r1, 0xEA);
}

__forceinline void tm_avx512bwvl_r512s_map_8::alg_7(WC_ARGS_512)
{
	wc0 = _mm512_xor_si512(wc0, mask_FF);
	wc1 = _mm512_xor_si512(wc1, mask_FF);
}

__forceinline void tm_avx512bwvl_r512s_map_8::xor_alg(WC_ARGS_512, uint8_t* values)
{
	wc0 = _mm512_xor_si512(wc0, _mm512_load_si512(values));
	wc1 = _mm512_xor_si512(wc1, _mm512_load_si512(values + 64));
}

__forceinline void tm_avx512bwvl_r512s_map_8::_run_alg(WC_ARGS_512, int algorithm_id, uint16_t* local_pos,
	const uint8_t* reg_base, const uint8_t* alg0_base,
	const uint8_t* alg6_base)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_512, alg0_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 1)
	{
		alg_1(WC_PASS_512, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS_512, reg_base[*local_pos] >> 7);
		*local_pos -= 1;
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS_512, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 4)
	{
		alg_4(WC_PASS_512, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS_512, reg_base[*local_pos] & 0x80);
		*local_pos -= 1;
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS_512, alg6_base + (2047 - *local_pos));
		*local_pos -= 128;
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS_512);
	}
}

__forceinline void tm_avx512bwvl_r512s_map_8::_run_one_map(WC_ARGS_512, int map_idx)
{
	uint16_t nibble_selector = schedule_entries->entries[static_cast<size_t>(map_idx)].nibble_selector;
	const uint8_t* reg_base = regular_rng_values_for_seeds_512_8_shuffled.get() + map_idx * 2048;
	const uint8_t* alg0_base = alg0_values_for_seeds_512_8_shuffled.get() + map_idx * 2048;
	const uint8_t* alg6_base = alg6_values_for_seeds_512_8_shuffled.get() + map_idx * 2048;
	uint16_t local_pos = 2047;

	for (int i = 0; i < 16; i++)
	{
		_mm512_storeu_si512(working_code_data, wc0);
		_mm512_storeu_si512(working_code_data + 64, wc1);

		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);

		uint8_t current_byte = ((uint8_t*)working_code_data)[shuffle_8(i, 512)];

		if (nibble == 1)
		{
			current_byte = static_cast<uint8_t>(current_byte >> 4);
		}

		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(WC_PASS_512, algorithm_id, &local_pos, reg_base, alg0_base, alg6_base);
	}
}

__forceinline void tm_avx512bwvl_r512s_map_8::_run_all_maps(WC_ARGS_512)
{
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
	{
		_run_one_map(WC_PASS_512, map_idx);
	}
}

__forceinline void tm_avx512bwvl_r512s_map_8::_decrypt_carnival_world(WC_ARGS_512)
{
	xor_alg(WC_PASS_512, carnival_world_data_shuffled);
}

__forceinline void tm_avx512bwvl_r512s_map_8::_decrypt_other_world(WC_ARGS_512)
{
	xor_alg(WC_PASS_512, other_world_data_shuffled);
}

__forceinline uint16_t tm_avx512bwvl_r512s_map_8::masked_checksum(WC_ARGS_512, uint8_t* mask)
{
	__m512i z = _mm512_setzero_si512();

	__m512i sum = _mm512_sad_epu8(_mm512_and_si512(wc0, _mm512_loadu_si512(mask)), z);
	sum = _mm512_add_epi64(sum, _mm512_sad_epu8(_mm512_and_si512(wc1, _mm512_loadu_si512(mask + 64)), z));

	return static_cast<uint16_t>(_mm512_reduce_add_epi64(sum));
}

__forceinline uint16_t tm_avx512bwvl_r512s_map_8::_calculate_carnival_world_checksum(WC_ARGS_512)
{
	return masked_checksum(WC_PASS_512, carnival_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx512bwvl_r512s_map_8::_calculate_other_world_checksum(WC_ARGS_512)
{
	return masked_checksum(WC_PASS_512, other_world_checksum_mask_shuffled);
}

__forceinline bool tm_avx512bwvl_r512s_map_8::check_carnival_world_checksum(WC_ARGS_512)
{
	return _calculate_carnival_world_checksum(WC_PASS_512) == _fetch_carnival_world_checksum_value(WC_PASS_512);
}

__forceinline bool tm_avx512bwvl_r512s_map_8::check_other_world_checksum(WC_ARGS_512)
{
	return _calculate_other_world_checksum(WC_PASS_512) == _fetch_other_world_checksum_value(WC_PASS_512);
}

__forceinline uint16_t tm_avx512bwvl_r512s_map_8::fetch_checksum_value(WC_ARGS_512, uint8_t code_length)
{
	_mm512_storeu_si512(working_code_data, wc0);
	_mm512_storeu_si512(working_code_data + 64, wc1);

	uint8_t lo = ((uint8_t*)working_code_data)[shuffle_8(127 - code_length, 512)];
	uint8_t hi = ((uint8_t*)working_code_data)[shuffle_8(127 - (code_length + 1), 512)];
	return static_cast<uint16_t>((hi << 8) | lo);
}

__forceinline uint16_t tm_avx512bwvl_r512s_map_8::_fetch_carnival_world_checksum_value(WC_ARGS_512)
{
	return fetch_checksum_value(WC_PASS_512, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx512bwvl_r512s_map_8::_fetch_other_world_checksum_value(WC_ARGS_512)
{
	return fetch_checksum_value(WC_PASS_512, OTHER_WORLD_CODE_LENGTH - 2);
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx512bwvl_r512s_map_8::_decrypt_check(WC_ARGS_512)
{
	WC_XOR_VARS_512;
	WC_COPY_XOR_512;
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(WC_XOR_PASS_512);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_carnival_world_checksum(WC_XOR_PASS_512))
			{
				return std::nullopt;
			}
		}
	}
	else
	{
		_decrypt_other_world(WC_XOR_PASS_512);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum(WC_XOR_PASS_512))
			{
				return std::nullopt;
			}
		}
	}

	_store_to_mem(WC_XOR_PASS_512);
	uint8_t unshuffled_data[128];
	unshuffle_mem(working_code_data, unshuffled_data, 512, false);
	return check_machine_code(unshuffled_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx512bwvl_r512s_map_8::_run_bruteforce(WC_ARGS_512, uint32_t data, uint8_t* result_data, uint32_t* result_size)
{
	_expand_code(data, WC_PASS_512);

	_run_all_maps(WC_PASS_512);

	auto carnival_flags = _decrypt_check<CHECK_CHECKSUMS, CARNIVAL_WORLD>(WC_PASS_512);
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

	auto other_flags = _decrypt_check<CHECK_CHECKSUMS, OTHER_WORLD>(WC_PASS_512);
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

void tm_avx512bwvl_r512s_map_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
{
	WC_VARS_512;

	for (uint32_t i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
		{
			return;
		}
		uint32_t data = start_data + i;

		_run_bruteforce<true>(WC_PASS_512, data, result_data, result_size);

		report_progress(static_cast<double>(i + 1) / static_cast<double>(amount_to_run));
	}
}

void tm_avx512bwvl_r512s_map_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_512;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_512, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_avx512bwvl_r512s_map_8::test_algorithm_chain(const uint8_t* algorithm_ids, int chain_length,
                                                     uint8_t* data, uint16_t* rng_seed)
{
	WC_VARS_512;
	load_data(data);
	_load_from_mem(WC_PASS_512);

	alg0_values_for_seeds_512_8_shuffled = rng->generate_alg0_values_for_seeds(rng_seed, 1, false);
	regular_rng_values_for_seeds_512_8_shuffled = rng->generate_regular_rng_values_for_seeds(rng_seed, 1, false);
	alg6_values_for_seeds_512_8_shuffled = rng->generate_alg6_values_for_seeds(rng_seed, 1, false);

	uint16_t local_pos = 2047;
	for (int i = 0; i < chain_length; ++i)
	{
		_run_alg(WC_PASS_512, algorithm_ids[i], &local_pos,
			regular_rng_values_for_seeds_512_8_shuffled.get(), alg0_values_for_seeds_512_8_shuffled.get(),
			alg6_values_for_seeds_512_8_shuffled.get());
	}

	_store_to_mem(WC_PASS_512);
	fetch_data(data);
}

void tm_avx512bwvl_r512s_map_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_512;
	load_data(data);
	_load_from_mem(WC_PASS_512);

	if (algorithm_id == 0)
	{
		alg0_values_for_seeds_512_8_shuffled = rng->generate_alg0_values_for_seeds(rng_seed, 1, false);
	}
	else if (algorithm_id == 1 || algorithm_id == 2 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 5)
	{
		regular_rng_values_for_seeds_512_8_shuffled = rng->generate_regular_rng_values_for_seeds(rng_seed, 1, false);
	}
	else if (algorithm_id == 6)
	{
		alg6_values_for_seeds_512_8_shuffled = rng->generate_alg6_values_for_seeds(rng_seed, 1, false);
	}

	uint16_t local_pos = 2047;
	for (int i = 0; i < iterations; i++)
	{
		if (local_pos < 128)
			local_pos = 2047;
		_run_alg(WC_PASS_512, algorithm_id, &local_pos,
			regular_rng_values_for_seeds_512_8_shuffled.get(), alg0_values_for_seeds_512_8_shuffled.get(),
			alg6_values_for_seeds_512_8_shuffled.get());
	}

	_store_to_mem(WC_PASS_512);
	fetch_data(data);
}

void tm_avx512bwvl_r512s_map_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_store_to_mem(WC_PASS_512);
	fetch_data(result_out);
}

void tm_avx512bwvl_r512s_map_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_run_all_maps(WC_PASS_512);
	_store_to_mem(WC_PASS_512);
	fetch_data(result_out);
}

bool tm_avx512bwvl_r512s_map_8::test_bruteforce_checksum(uint32_t data, int world)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_run_all_maps(WC_PASS_512);

	if (world == CARNIVAL_WORLD)
		return _decrypt_check<true, CARNIVAL_WORLD>(WC_PASS_512).has_value();
	else
		return _decrypt_check<true, OTHER_WORLD>(WC_PASS_512).has_value();
}


void tm_avx512bwvl_r512s_map_8::set_key(uint32_t new_key)
{
	TM_base::set_key(new_key);
	generate_map_rng();
}
