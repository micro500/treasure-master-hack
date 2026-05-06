#include "tm_avx512bwvl_r512_8.h"


tm_avx512bwvl_r512_8::tm_avx512bwvl_r512_8(RNG* rng_obj) : tm_avx512bwvl_r512_8(rng_obj, 0) {}

tm_avx512bwvl_r512_8::tm_avx512bwvl_r512_8(RNG* rng_obj, const uint32_t key) : tm_avx512bwvl_r512_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx512bwvl_r512_8::tm_avx512bwvl_r512_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries)
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
}

__forceinline void tm_avx512bwvl_r512_8::initialize()
{
	if (!_initialized)
	{
		auto _r0 = rng->generate_expansion_values_8();
		auto _r1 = rng->generate_seed_forward_1();
		auto _r2 = rng->generate_seed_forward_128();
		auto _r3 = rng->generate_regular_rng_values_8();
		auto _r4 = rng->generate_alg0_values_8();
		auto _r5 = rng->generate_alg2_values_8_8();
		auto _r6 = rng->generate_alg4_values_8();
		auto _r7 = rng->generate_alg5_values_8_8();
		auto _r8 = rng->generate_alg6_values_8();

		_expansion_8  = static_cast<uint8_t*>(_r0.get());
		_seed_fwd_1   = static_cast<uint16_t*>(_r1.get());
		_seed_fwd_128 = static_cast<uint16_t*>(_r2.get());
		_regular_8    = static_cast<uint8_t*>(_r3.get());
		_alg0_8       = static_cast<uint8_t*>(_r4.get());
		_alg2_8_8     = static_cast<uint8_t*>(_r5.get());
		_alg4_8       = static_cast<uint8_t*>(_r6.get());
		_alg5_8_8     = static_cast<uint8_t*>(_r7.get());
		_alg6_8       = static_cast<uint8_t*>(_r8.get());

		_table_refs = { _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7, _r8 };
		_initialized = true;
	}
	obj_name = "tm_avx512bwvl_r512_8";
}

__forceinline void tm_avx512bwvl_r512_8::_load_from_mem(WC_ARGS_512)
{
	wc0 = _mm512_loadu_si512(working_code_data);
	wc1 = _mm512_loadu_si512(working_code_data + 64);
}

__forceinline void tm_avx512bwvl_r512_8::_store_to_mem(WC_ARGS_512)
{
	_mm512_storeu_si512(working_code_data, wc0);
	_mm512_storeu_si512(working_code_data + 64, wc1);
}

__forceinline void tm_avx512bwvl_r512_8::_expand_code(uint32_t data, WC_ARGS_512)
{
	uint64_t x = ((uint64_t)key << 32) | data;

	__m512i a = _mm512_set1_epi64(static_cast<int64_t>(x));
	__m512i nat_mask = _mm512_set_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7);
	__m512i pattern = _mm512_shuffle_epi8(a, nat_mask);

	wc0 = pattern;
	wc1 = pattern;

	uint8_t* rng_start = _expansion_8;
	uint16_t rng_seed = (key >> 16) & 0xFFFF;

	add_alg(WC_PASS_512, &rng_seed, rng_start);
}

void tm_avx512bwvl_r512_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = new_data[i];
	}
}

void tm_avx512bwvl_r512_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = working_code_data[i];
	}
}

__forceinline void tm_avx512bwvl_r512_8::_run_alg(WC_ARGS_512, int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_512, rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		uint8_t* rng_start = _regular_8;

		if (algorithm_id == 4)
		{
			rng_start = _alg4_8;
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

__forceinline void tm_avx512bwvl_r512_8::alg_0(WC_ARGS_512, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg0_8 + (*rng_seed) * 128;

	wc0 = _mm512_slli_epi16(wc0, 1);
	__m512i rng_val = _mm512_load_si512(rng_start);
	wc0 = _mm512_ternarylogic_epi32(wc0, mask_FE, rng_val, 0xEA);

	wc1 = _mm512_slli_epi16(wc1, 1);
	rng_val = _mm512_load_si512(rng_start + 64);
	wc1 = _mm512_ternarylogic_epi32(wc1, mask_FE, rng_val, 0xEA);
}

__forceinline void tm_avx512bwvl_r512_8::alg_2_sub(__m512i& wc, __m512i& carry)
{
	// new even byte (low of each word): (b[2k] >> 1) | (b[2k+1] & 0x80)
	__m512i part_lo = _mm512_and_si512(_mm512_srli_epi16(wc, 1), _mm512_set1_epi16(0x007F));
	__m512i part_hi_bit = _mm512_and_si512(_mm512_srli_epi16(wc, 8), _mm512_set1_epi16(0x0080));
	__m512i new_lo = _mm512_or_si512(part_lo, part_hi_bit);

	// carry_k = bit0(b[2k+2]) in the HIGH byte of word k
	// Move bit0 of each low byte to high-byte position, then right-shift 2 bytes across full register
	__m512i low_bit0 = _mm512_and_si512(wc, _mm512_set1_epi16(0x0001));
	__m512i low_bit0_hi = _mm512_slli_epi16(low_bit0, 8);
	__m512i lane_shifted = _mm512_maskz_permutexvar_epi32(
		_cvtu32_mask16(0x0FFF),
		_mm512_set_epi32(0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4),
		low_bit0_hi);
	__m512i carry_in = _mm512_or_si512(_mm512_alignr_epi8(lane_shifted, low_bit0_hi, 2), carry);

	// new odd byte (high of each word): (b[2k+1] << 1) | carry_k
	__m512i new_hi = _mm512_or_si512(
		_mm512_and_si512(_mm512_slli_epi16(wc, 1), _mm512_set1_epi16(static_cast<int16_t>(0xFE00))),
		carry_in);

	// carry_out: bit0(b[0]) placed at byte 63 bit 0
	uint8_t bit0_val = static_cast<uint8_t>(_mm_extract_epi8(_mm512_castsi512_si128(wc), 0)) & 0x01;
	carry = _mm512_and_si512(_mm512_set1_epi8(static_cast<int8_t>(bit0_val)), mask_top_01);

	wc = _mm512_ternarylogic_epi32(new_lo, _mm512_set1_epi16(0x00FF), new_hi, 0xCA);
}

__forceinline void tm_avx512bwvl_r512_8::alg_2(WC_ARGS_512, uint16_t* rng_seed)
{
	__m512i carry = _mm512_and_si512(
		_mm512_set1_epi8(static_cast<int8_t>(_alg2_8_8[*rng_seed])),
		mask_top_01);

	alg_2_sub(wc1, carry);
	alg_2_sub(wc0, carry);
}

__forceinline void tm_avx512bwvl_r512_8::alg_3(WC_ARGS_512, uint16_t* rng_seed)
{
	uint8_t* rng_start = _regular_8 + (*rng_seed) * 128;
	xor_alg(WC_PASS_512, rng_start);
}

__forceinline void tm_avx512bwvl_r512_8::xor_alg(WC_ARGS_512, uint8_t* values)
{
	wc0 = _mm512_xor_si512(wc0, _mm512_load_si512(values));
	wc1 = _mm512_xor_si512(wc1, _mm512_load_si512(values + 64));
}

__forceinline void tm_avx512bwvl_r512_8::alg_5_sub(__m512i& wc, __m512i& carry)
{
	// new even byte (low of each word): (b[2k] << 1) | (b[2k+1] & 0x01)
	__m512i part_lo = _mm512_and_si512(_mm512_slli_epi16(wc, 1), _mm512_set1_epi16(0x00FE));
	__m512i part_lo_bit = _mm512_and_si512(_mm512_srli_epi16(wc, 8), _mm512_set1_epi16(0x0001));
	__m512i new_lo = _mm512_or_si512(part_lo, part_lo_bit);

	// carry_k = bit7(b[2k+2]) in the HIGH byte of word k
	__m512i low_bit7 = _mm512_and_si512(wc, _mm512_set1_epi16(0x0080));
	__m512i low_bit7_hi = _mm512_slli_epi16(low_bit7, 8);
	__m512i lane_shifted = _mm512_maskz_permutexvar_epi32(
		_cvtu32_mask16(0x0FFF),
		_mm512_set_epi32(0, 0, 0, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4),
		low_bit7_hi);
	__m512i carry_in = _mm512_or_si512(_mm512_alignr_epi8(lane_shifted, low_bit7_hi, 2), carry);

	// new odd byte (high of each word): (b[2k+1] >> 1) | carry_k
	__m512i new_hi = _mm512_or_si512(
		_mm512_and_si512(_mm512_srli_epi16(wc, 1), _mm512_set1_epi16(0x7F00)),
		carry_in);

	// carry_out: bit7(b[0]) placed at byte 63 bit 7
	uint8_t bit7_val = static_cast<uint8_t>(_mm_extract_epi8(_mm512_castsi512_si128(wc), 0)) & 0x80;
	carry = _mm512_and_si512(_mm512_set1_epi8(static_cast<int8_t>(bit7_val)), mask_top_80);

	wc = _mm512_ternarylogic_epi32(new_lo, _mm512_set1_epi16(0x00FF), new_hi, 0xCA);
}

__forceinline void tm_avx512bwvl_r512_8::alg_5(WC_ARGS_512, uint16_t* rng_seed)
{
	__m512i carry = _mm512_and_si512(
		_mm512_set1_epi8(static_cast<int8_t>(_alg5_8_8[*rng_seed])),
		mask_top_80);

	alg_5_sub(wc1, carry);
	alg_5_sub(wc0, carry);
}

__forceinline void tm_avx512bwvl_r512_8::alg_6(WC_ARGS_512, uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg6_8 + (*rng_seed) * 128;

	wc0 = _mm512_srli_epi16(wc0, 1);
	__m512i rng_val = _mm512_load_si512(rng_start);
	wc0 = _mm512_ternarylogic_epi32(wc0, mask_7F, rng_val, 0xEA);

	wc1 = _mm512_srli_epi16(wc1, 1);
	rng_val = _mm512_load_si512(rng_start + 64);
	wc1 = _mm512_ternarylogic_epi32(wc1, mask_7F, rng_val, 0xEA);
}

__forceinline void tm_avx512bwvl_r512_8::alg_7(WC_ARGS_512)
{
	wc0 = _mm512_xor_si512(wc0, mask_FF);
	wc1 = _mm512_xor_si512(wc1, mask_FF);
}

__forceinline void tm_avx512bwvl_r512_8::add_alg(WC_ARGS_512, uint16_t* rng_seed, uint8_t* rng_start)
{
	rng_start = rng_start + (*rng_seed) * 128;

	__m512i rng_val = _mm512_load_si512(rng_start);
	wc0 = _mm512_add_epi8(wc0, rng_val);

	rng_val = _mm512_load_si512(rng_start + 64);
	wc1 = _mm512_add_epi8(wc1, rng_val);
}

void tm_avx512bwvl_r512_8::_run_one_map(WC_ARGS_512, const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16_t rng_seed = static_cast<uint16_t>((schedule_entry.rng1 << 8) | schedule_entry.rng2);
	uint16_t nibble_selector = schedule_entry.nibble_selector;

	for (int i = 0; i < 16; i++)
	{
		// Natural layout: bytes 0-15 are in wc0; store only wc0 to read byte i
		_mm512_storeu_si512(working_code_data, wc0);

		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);

		uint8_t current_byte = working_code_data[i];

		if (nibble == 1)
		{
			current_byte = static_cast<uint8_t>(current_byte >> 4);
		}

		uint8_t alg_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(WC_PASS_512, alg_id, &rng_seed);
	}
}

void tm_avx512bwvl_r512_8::_run_all_maps(WC_ARGS_512)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = this->schedule_entries->entries.begin(); it != this->schedule_entries->entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;
		_run_one_map(WC_PASS_512, schedule_entry);
	}
}

__forceinline uint16_t tm_avx512bwvl_r512_8::masked_checksum(WC_ARGS_512, uint8_t* mask)
{
	__m512i z = _mm512_setzero_si512();

	__m512i sum = _mm512_sad_epu8(_mm512_and_si512(wc0, _mm512_loadu_si512(mask)), z);
	sum = _mm512_add_epi64(sum, _mm512_sad_epu8(_mm512_and_si512(wc1, _mm512_loadu_si512(mask + 64)), z));

	return static_cast<uint16_t>(_mm512_reduce_add_epi64(sum));
}

__forceinline uint16_t tm_avx512bwvl_r512_8::_calculate_carnival_world_checksum(WC_ARGS_512)
{
	return masked_checksum(WC_PASS_512, carnival_world_checksum_mask);
}

__forceinline uint16_t tm_avx512bwvl_r512_8::_calculate_other_world_checksum(WC_ARGS_512)
{
	return masked_checksum(WC_PASS_512, other_world_checksum_mask);
}

__forceinline bool tm_avx512bwvl_r512_8::check_carnival_world_checksum(WC_ARGS_512)
{
	return _calculate_carnival_world_checksum(WC_PASS_512) == _fetch_carnival_world_checksum_value(WC_PASS_512);
}

__forceinline bool tm_avx512bwvl_r512_8::check_other_world_checksum(WC_ARGS_512)
{
	return _calculate_other_world_checksum(WC_PASS_512) == _fetch_other_world_checksum_value(WC_PASS_512);
}

__forceinline uint16_t tm_avx512bwvl_r512_8::fetch_checksum_value(WC_ARGS_512, uint8_t code_length)
{
	// Both checksum byte positions fall within bytes 0-63 (wc0), so only store wc0
	_mm512_storeu_si512(working_code_data, wc0);

	uint8_t lo = working_code_data[127 - code_length];
	uint8_t hi = working_code_data[127 - (code_length + 1)];
	return static_cast<uint16_t>((hi << 8) | lo);
}

__forceinline uint16_t tm_avx512bwvl_r512_8::_fetch_carnival_world_checksum_value(WC_ARGS_512)
{
	return fetch_checksum_value(WC_PASS_512, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx512bwvl_r512_8::_fetch_other_world_checksum_value(WC_ARGS_512)
{
	return fetch_checksum_value(WC_PASS_512, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline void tm_avx512bwvl_r512_8::_decrypt_carnival_world(WC_ARGS_512)
{
	xor_alg(WC_PASS_512, carnival_world_data);
}

__forceinline void tm_avx512bwvl_r512_8::_decrypt_other_world(WC_ARGS_512)
{
	xor_alg(WC_PASS_512, other_world_data);
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx512bwvl_r512_8::_decrypt_check(WC_ARGS_512)
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
	return check_machine_code(working_code_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx512bwvl_r512_8::_run_bruteforce(WC_ARGS_512, uint32_t data, uint8_t* result_data, uint32_t* result_size)
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

void tm_avx512bwvl_r512_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
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

void tm_avx512bwvl_r512_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_512;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_512, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_avx512bwvl_r512_8::test_algorithm_chain(const uint8_t* algorithm_ids, int chain_length,
                                                uint8_t* data, uint16_t* rng_seed)
{
	WC_VARS_512;
	load_data(data);
	_load_from_mem(WC_PASS_512);
	for (int i = 0; i < chain_length; ++i)
	{
		_run_alg(WC_PASS_512, algorithm_ids[i], rng_seed);
	}
	_store_to_mem(WC_PASS_512);
	fetch_data(data);
}

void tm_avx512bwvl_r512_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_512;
	load_data(data);
	_load_from_mem(WC_PASS_512);
	for (int i = 0; i < iterations; i++)
		_run_alg(WC_PASS_512, algorithm_id, rng_seed);
	_store_to_mem(WC_PASS_512);
	fetch_data(data);
}

void tm_avx512bwvl_r512_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_store_to_mem(WC_PASS_512);
	fetch_data(result_out);
}

void tm_avx512bwvl_r512_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_run_all_maps(WC_PASS_512);
	_store_to_mem(WC_PASS_512);
	fetch_data(result_out);
}

bool tm_avx512bwvl_r512_8::test_bruteforce_checksum(uint32_t data, int world)
{
	WC_VARS_512;
	_expand_code(data, WC_PASS_512);
	_run_all_maps(WC_PASS_512);

	if (world == CARNIVAL_WORLD)
		return _decrypt_check<true, CARNIVAL_WORLD>(WC_PASS_512).has_value();
	else
		return _decrypt_check<true, OTHER_WORLD>(WC_PASS_512).has_value();
}


