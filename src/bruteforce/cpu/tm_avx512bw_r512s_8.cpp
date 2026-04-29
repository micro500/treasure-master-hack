#include "tm_avx512bw_r512s_8.h"


tm_avx512bw_r512s_8::tm_avx512bw_r512s_8(RNG* rng_obj) : tm_avx512bw_r512s_8(rng_obj, 0) {}

tm_avx512bw_r512s_8::tm_avx512bw_r512s_8(RNG* rng_obj, const uint32_t key) : tm_avx512bw_r512s_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx512bw_r512s_8::tm_avx512bw_r512s_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
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

	shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 512, false);
	shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 512, false);
	shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 512, false);
	shuffle_mem(other_world_data, other_world_data_shuffled, 512, false);
}

__forceinline void tm_avx512bw_r512s_8::initialize()
{
	if (!_initialized)
	{
		auto _r0 = rng->generate_expansion_values_512_8_shuffled();
		auto _r1 = rng->generate_seed_forward_1();
		auto _r2 = rng->generate_seed_forward_128();
		auto _r3 = rng->generate_regular_rng_values_8();
		auto _r4 = rng->generate_regular_rng_values_512_8_shuffled();
		auto _r5 = rng->generate_alg0_values_512_8_shuffled();
		auto _r6 = rng->generate_alg2_values_8_8();
		auto _r7 = rng->generate_alg4_values_512_8_shuffled();
		auto _r8 = rng->generate_alg5_values_8_8();
		auto _r9 = rng->generate_alg6_values_512_8_shuffled();

		_expansion_512s = static_cast<uint8_t*>(_r0.get());
		_seed_fwd_1     = static_cast<uint16_t*>(_r1.get());
		_seed_fwd_128   = static_cast<uint16_t*>(_r2.get());
		_regular_8      = static_cast<uint8_t*>(_r3.get());
		_regular_512s   = static_cast<uint8_t*>(_r4.get());
		_alg0_512s      = static_cast<uint8_t*>(_r5.get());
		_alg2_8_8       = static_cast<uint8_t*>(_r6.get());
		_alg4_512s      = static_cast<uint8_t*>(_r7.get());
		_alg5_8_8       = static_cast<uint8_t*>(_r8.get());
		_alg6_512s      = static_cast<uint8_t*>(_r9.get());

		_table_refs = { _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7, _r8, _r9 };
		_initialized = true;
	}
	obj_name = "tm_avx512bw_r512s_8";
}

__forceinline void tm_avx512bw_r512s_8::_expand_code(uint32_t data, WC_ARGS_512)
{
	uint64_t x = ((uint64_t)key << 32) | data;

	__m512i a = _mm512_set1_epi64(static_cast<int64_t>(x));
	__m512i lo_mask = _mm512_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m512i hi_mask = _mm512_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);

	__m512i lo = _mm512_shuffle_epi8(a, lo_mask);
	__m512i hi = _mm512_shuffle_epi8(a, hi_mask);

	wc0 = lo;
	wc1 = hi;

	uint8_t* rng_start = _expansion_512s;
	uint16_t rng_seed = (key >> 16) & 0xFFFF;

	add_alg(WC_PASS_512, &rng_seed, rng_start);
}

void tm_avx512bw_r512s_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8_t*)working_code_data)[shuffle_8(i, 512)] = new_data[i];
	}
}

void tm_avx512bw_r512s_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8_t*)working_code_data)[shuffle_8(i, 512)];
	}
}

__forceinline void tm_avx512bw_r512s_8::_load_from_mem(WC_ARGS_512)
{
	wc0 = _mm512_loadu_si512(working_code_data);
	wc1 = _mm512_loadu_si512(working_code_data + 64);
}

__forceinline void tm_avx512bw_r512s_8::_store_to_mem(WC_ARGS_512)
{
	_mm512_storeu_si512(working_code_data, wc0);
	_mm512_storeu_si512(working_code_data + 64, wc1);
}


__forceinline void tm_avx512bw_r512s_8::_run_alg(WC_ARGS_512, int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_512, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		uint8_t* rng_start = _regular_512s;

		if (algorithm_id == 4)
		{
			rng_start = _alg4_512s;
		}

		add_alg(WC_PASS_512, rng_seed, rng_start);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS_512, rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS_512, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS_512, rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS_512, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS_512);
	}
}

__forceinline void tm_avx512bw_r512s_8::alg_0(WC_ARGS_512, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg0_512s + ((*rng_seed) * 128);

	wc0 = _mm512_slli_epi64(wc0, 1);
	__m512i rng_val = _mm512_load_si512((__m512i*)(rng_start));
	wc0 = _mm512_and_si512(wc0, mask_FE);
	wc0 = _mm512_or_si512(wc0, rng_val);

	wc1 = _mm512_slli_epi64(wc1, 1);
	rng_val = _mm512_load_si512((__m512i*)(rng_start + 64));
	wc1 = _mm512_and_si512(wc1, mask_FE);
	wc1 = _mm512_or_si512(wc1, rng_val);
}

__forceinline void tm_avx512bw_r512s_8::alg_2_sub(__m512i& working_a, __m512i& working_b, __m512i& carry)
{
	// Shift bytes right 1 bit
	__m512i cur_val1_most = _mm512_and_si512(_mm512_srli_epi64(working_a, 1), mask_7F);
	// Shift bytes left 1 bit
	__m512i cur_val2_most = _mm512_and_si512(_mm512_slli_epi64(working_b, 1), mask_FE);
	
	// Mask off the top bits
	__m512i cur_val2_masked = _mm512_and_si512(working_b, mask_80);

	__m512i cur_val1_bit = _mm512_and_si512(working_a, mask_01);

	// Shift right 1 byte
	__m512i mask = _mm512_maskz_permutexvar_epi32(_cvtu32_mask16(0x0FFF), _mm512_set_epi32(0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4), cur_val1_bit);
	__m512i cur_val1_srl = _mm512_alignr_epi8(mask, cur_val1_bit, 1);
	//__m512i cur_val1_srl = _mm512_maskz_permutexvar_epi8(_cvtu64_mask64(0x7FFFFFFFFFFFFFFFull), _mm512_set_epi8(0,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1), cur_val1_bit);
	__m512i cur_val1_srl_w_carry = _mm512_or_si512(cur_val1_srl, carry);

	working_a = _mm512_or_si512(cur_val1_most, cur_val2_masked);
	working_b = _mm512_or_si512(cur_val2_most, cur_val1_srl_w_carry);
}
__forceinline void tm_avx512bw_r512s_8::alg_2(WC_ARGS_512, uint16_t* rng_seed)
{
	__m512i carry = _mm512_and_si512(
		_mm512_set1_epi8(static_cast<int8_t>(_alg2_8_8[*rng_seed])),
		mask_top_01);

	alg_2_sub(WC_PASS_512, carry);
}

__forceinline void tm_avx512bw_r512s_8::alg_3(WC_ARGS_512, uint16_t* rng_seed)
{
	uint8_t* rng_start = _regular_512s + ((*rng_seed) * 128);

	__m512i rng_val = _mm512_load_si512((__m512i*)(rng_start));
	wc0 = _mm512_xor_si512(wc0, rng_val);

	rng_val = _mm512_load_si512((__m512i*)(rng_start + 64));
	wc1 = _mm512_xor_si512(wc1, rng_val);
}

__forceinline void tm_avx512bw_r512s_8::xor_alg(WC_ARGS_512, uint8_t* values)
{
	wc0 = _mm512_xor_si512(wc0, _mm512_load_si512(values));
	wc1 = _mm512_xor_si512(wc1, _mm512_load_si512(values + 64));
}

__forceinline void tm_avx512bw_r512s_8::alg_5_sub(__m512i& working_a, __m512i& working_b, __m512i& carry)
{
	// Shift bytes right 1 bit
	__m512i cur_val1_most = _mm512_and_si512(_mm512_slli_epi64(working_a, 1), mask_FE);
	// Shift bytes left 1 bit
	__m512i cur_val2_most = _mm512_and_si512(_mm512_srli_epi64(working_b, 1), mask_7F);

	// Mask off the top bits
	__m512i cur_val2_masked = _mm512_and_si512(working_b, mask_01);

	__m512i cur_val1_bit = _mm512_and_si512(working_a, mask_80);

	// Shift right 1 byte
	__m512i mask = _mm512_maskz_permutexvar_epi32(_cvtu32_mask16(0x0FFF), _mm512_set_epi32(0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4), cur_val1_bit);
	__m512i cur_val1_srl = _mm512_alignr_epi8(mask, cur_val1_bit, 1);
	//__m512i cur_val1_srl = _mm512_maskz_permutexvar_epi8(_cvtu64_mask64(0x7FFFFFFFFFFFFFFFull), _mm512_set_epi8(0,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1), cur_val1_bit);
	__m512i cur_val1_srl_w_carry = _mm512_or_si512(cur_val1_srl, carry);

	working_a = _mm512_or_si512(cur_val1_most, cur_val2_masked);
	working_b = _mm512_or_si512(cur_val2_most, cur_val1_srl_w_carry);
}

__forceinline void tm_avx512bw_r512s_8::alg_5(WC_ARGS_512, uint16_t* rng_seed)
{
	__m512i carry = _mm512_and_si512(
		_mm512_set1_epi8(static_cast<int8_t>(_alg5_8_8[*rng_seed])),
		mask_top_80);

	alg_5_sub(WC_PASS_512, carry);
}

__forceinline void tm_avx512bw_r512s_8::alg_6(WC_ARGS_512, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg6_512s + ((*rng_seed) * 128);

	wc0 = _mm512_srli_epi64(wc0, 1);
	__m512i rng_val = _mm512_load_si512((__m512i*)(rng_start));
	wc0 = _mm512_and_si512(wc0, mask_7F);
	wc0 = _mm512_or_si512(wc0, rng_val);

	wc1 = _mm512_srli_epi64(wc1, 1);
	rng_val = _mm512_load_si512((__m512i*)(rng_start + 64));
	wc1 = _mm512_and_si512(wc1, mask_7F);
	wc1 = _mm512_or_si512(wc1, rng_val);
}

__forceinline void tm_avx512bw_r512s_8::alg_7(WC_ARGS_512)
{
	wc0 = _mm512_xor_si512(wc0, mask_FF);
	wc1 = _mm512_xor_si512(wc1, mask_FF);
}

__forceinline void tm_avx512bw_r512s_8::add_alg(WC_ARGS_512, uint16_t* rng_seed, uint8_t* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);

	__m512i rng_val = _mm512_load_si512((__m512i*)(rng_start));
	wc0 = _mm512_add_epi8(wc0, rng_val);

	rng_val = _mm512_load_si512((__m512i*)(rng_start + 64));
	wc1 = _mm512_add_epi8(wc1, rng_val);
}


void tm_avx512bw_r512s_8::_run_one_map(WC_ARGS_512, const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16_t rng_seed = static_cast<uint16_t>((schedule_entry.rng1 << 8) | schedule_entry.rng2);
	uint16_t nibble_selector = schedule_entry.nibble_selector;

	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		// Get the highest bit of the nibble selector to use as a flag
		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		// Shift the nibble selector up one bit
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);

		_store_to_mem(WC_PASS_512);

		// If the flag is a 1, get the high nibble of the current byte
		// Otherwise use the low nibble
		uint8_t current_byte = (uint8_t)working_code_data[shuffle_8(i, 512)];

		if (nibble == 1)
		{
			current_byte = static_cast<uint8_t>(current_byte >> 4);
		}

		// Mask off only 3 bits
		uint8_t alg_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(WC_PASS_512, alg_id, &rng_seed);
	}
}

void tm_avx512bw_r512s_8::_run_all_maps(WC_ARGS_512)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = this->schedule_entries->entries.begin(); it != this->schedule_entries->entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

		_run_one_map(WC_PASS_512, schedule_entry);
	}
}

__forceinline uint16_t tm_avx512bw_r512s_8::masked_checksum(WC_ARGS_512, uint8_t* mask)
{
	__m512i z = _mm512_setzero_si512();

	__m512i sum = _mm512_sad_epu8(_mm512_and_si512(wc0, _mm512_loadu_si512(mask)), z);
	sum = _mm512_add_epi64(sum, _mm512_sad_epu8(_mm512_and_si512(wc1, _mm512_loadu_si512(mask + 64)), z));

	return (uint16_t)_mm512_reduce_add_epi64(sum);
}


__forceinline uint16_t tm_avx512bw_r512s_8::_calculate_carnival_world_checksum(WC_ARGS_512)
{
	return masked_checksum(WC_PASS_512, carnival_world_checksum_mask_shuffled);
}

__forceinline uint16_t tm_avx512bw_r512s_8::_calculate_other_world_checksum(WC_ARGS_512)
{
	return masked_checksum(WC_PASS_512, other_world_checksum_mask_shuffled);
}

__forceinline bool tm_avx512bw_r512s_8::check_carnival_world_checksum(WC_ARGS_512)
{
	return _calculate_carnival_world_checksum(WC_PASS_512) == _fetch_carnival_world_checksum_value(WC_PASS_512);
}

__forceinline bool tm_avx512bw_r512s_8::check_other_world_checksum(WC_ARGS_512)
{
	return _calculate_other_world_checksum(WC_PASS_512) == _fetch_other_world_checksum_value(WC_PASS_512);
}

__forceinline uint16_t tm_avx512bw_r512s_8::_fetch_carnival_world_checksum_value(WC_ARGS_512)
{
	return fetch_checksum_value(WC_PASS_512, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx512bw_r512s_8::_fetch_other_world_checksum_value(WC_ARGS_512)
{
	return fetch_checksum_value(WC_PASS_512, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx512bw_r512s_8::fetch_checksum_value(WC_ARGS_512, uint8_t code_length)
{
	_store_to_mem(WC_PASS_512);

	uint8_t checksum_low = static_cast<uint8_t>((reinterpret_cast<uint8_t*>(working_code_data))[shuffle_8((127 - code_length), 512)]);
	uint8_t checksum_hi = static_cast<uint8_t>((reinterpret_cast<uint8_t*>(working_code_data))[shuffle_8((127 - (code_length + 1)), 512)]);
	uint16_t checksum = static_cast<uint16_t>((checksum_hi << 8) | checksum_low);

	return checksum;
}



__forceinline void tm_avx512bw_r512s_8::_decrypt_carnival_world(WC_ARGS_512)
{
	xor_alg(WC_PASS_512, carnival_world_data_shuffled);
}

__forceinline void tm_avx512bw_r512s_8::_decrypt_other_world(WC_ARGS_512)
{
	xor_alg(WC_PASS_512, other_world_data_shuffled);
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx512bw_r512s_8::_decrypt_check(WC_ARGS_512)
{
	WC_XOR_VARS_512;
	WC_COPY_XOR_512;
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(WC_XOR_PASS_512);

		if constexpr (CHECK_CHECKSUM) {
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
__forceinline void tm_avx512bw_r512s_8::_run_bruteforce(WC_ARGS_512, uint32_t data, uint8_t* result_data, uint32_t* result_size)
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

void tm_avx512bw_r512s_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
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

void tm_avx512bw_r512s_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_512;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_512, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}


void tm_avx512bw_r512s_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	WC_VARS_512;
	load_data(data);
	_load_from_mem(WC_PASS_512);
	_run_alg(WC_PASS_512, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_512);
	fetch_data(data);
}
void tm_avx512bw_r512s_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_512;
	load_data(data);
	_load_from_mem(WC_PASS_512);
	for (int i = 0; i < iterations; i++)
		_run_alg(WC_PASS_512, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_512);
	fetch_data(data);
}


void tm_avx512bw_r512s_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_store_to_mem(WC_PASS_512);
	fetch_data(result_out);
}

void tm_avx512bw_r512s_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_run_all_maps(WC_PASS_512);
	_store_to_mem(WC_PASS_512);
	fetch_data(result_out);
}

bool tm_avx512bw_r512s_8::test_bruteforce_checksum(uint32_t data, int world)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_run_all_maps(WC_PASS_512);

	if (world == CARNIVAL_WORLD)
		return _decrypt_check<true, CARNIVAL_WORLD>(WC_PASS_512).has_value();
	else
		return _decrypt_check<true, OTHER_WORLD>(WC_PASS_512).has_value();
}

