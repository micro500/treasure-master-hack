#include "tm_avx_m256_8.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx_m256_8::tm_avx_m256_8(RNG* rng_obj) : tm_avx_m256_8(rng_obj, 0) {}

tm_avx_m256_8::tm_avx_m256_8(RNG* rng_obj, const uint32_t key) : tm_avx_m256_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx_m256_8::tm_avx_m256_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj),
	mask_FF(_mm256_set1_epi16(static_cast<int16_t>(0xFFFF))),
	mask_FE(_mm256_set1_epi16(static_cast<int16_t>(0xFEFE))),
	mask_7F(_mm256_set1_epi16(0x7F7F)),
	mask_80(_mm256_set1_epi16(static_cast<int16_t>(0x8080))),
	mask_01(_mm256_set1_epi16(0x0101)),
	mask_top_01(_mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
	mask_top_80(_mm256_set_epi16(static_cast<int16_t>(0x8000), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;
}

__forceinline void tm_avx_m256_8::initialize()
{
	if (!_initialized)
	{
		auto _r0 = rng->generate_expansion_values_8();
		auto _r1 = rng->generate_seed_forward_1();
		auto _r2 = rng->generate_seed_forward_128();
		auto _r3 = rng->generate_regular_rng_values_8();
		auto _r4 = rng->generate_alg0_values_8();
		auto _r5 = rng->generate_alg2_values_256_8();
		auto _r6 = rng->generate_alg4_values_8();
		auto _r7 = rng->generate_alg5_values_256_8();
		auto _r8 = rng->generate_alg6_values_8();

		_expansion_8  = static_cast<uint8_t*>(_r0.get());
		_seed_fwd_1   = static_cast<uint16_t*>(_r1.get());
		_seed_fwd_128 = static_cast<uint16_t*>(_r2.get());
		_regular_8    = static_cast<uint8_t*>(_r3.get());
		_alg0_8       = static_cast<uint8_t*>(_r4.get());
		_alg2_256_8   = static_cast<uint8_t*>(_r5.get());
		_alg4_8       = static_cast<uint8_t*>(_r6.get());
		_alg5_256_8   = static_cast<uint8_t*>(_r7.get());
		_alg6_8       = static_cast<uint8_t*>(_r8.get());

		_table_refs = { _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7, _r8 };
		_initialized = true;
	}
	obj_name = "tm_avx_m256_8";
}

void tm_avx_m256_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		working_code_data[i] = new_data[i];
}

void tm_avx_m256_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		new_data[i] = working_code_data[i];
}

__forceinline void tm_avx_m256_8::_expand_code(uint32_t data)
{
	uint8_t* x = working_code_data;
	for (int i = 0; i < 128; i += 8)
	{
		x[i]   = (key >> 24) & 0xFF;
		x[i+1] = (key >> 16) & 0xFF;
		x[i+2] = (key >> 8)  & 0xFF;
		x[i+3] = key & 0xFF;
		x[i+4] = (data >> 24) & 0xFF;
		x[i+5] = (data >> 16) & 0xFF;
		x[i+6] = (data >> 8)  & 0xFF;
		x[i+7] = data & 0xFF;
	}
	uint16_t rng_seed = static_cast<uint16_t>((key >> 16) & 0xFFFF);
	add_alg(&rng_seed, _expansion_8);
}

__forceinline void tm_avx_m256_8::add_alg(uint16_t* rng_seed, uint8_t* rng_start)
{
	rng_start = rng_start + (*rng_seed) * 128;
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start + i * 32));
		__m128i sum_lo = _mm_add_epi8(_mm256_castsi256_si128(cur_val), _mm256_castsi256_si128(rng_val));
		__m128i sum_hi = _mm_add_epi8(_mm256_extractf128_si256(cur_val, 1), _mm256_extractf128_si256(rng_val, 1));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), _mm256_set_m128i(sum_hi, sum_lo));
	}
}

__forceinline void tm_avx_m256_8::xor_alg(uint8_t* values)
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_load_si256((__m256i*)(values + i * 32));
		cur_val = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_8::alg_0(uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg0_8 + (*rng_seed) * 128;
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m128i lo = _mm256_castsi256_si128(cur_val);
		__m128i hi = _mm256_extractf128_si256(cur_val, 1);
		lo = _mm_slli_epi16(lo, 1);
		hi = _mm_slli_epi16(hi, 1);
		cur_val = _mm256_set_m128i(hi, lo);
		cur_val = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask_FE)));
		__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start + i * 32));
		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_8::alg_2_sub(__m256i& cur_val, __m256i& carry)
{
	__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
	__m128i cur_val_hi = _mm256_extractf128_si256(cur_val, 1);

	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)),
		_mm256_castsi256_pd(mask_top_01)));

	__m128i temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo, 1), _mm_slli_si128(cur_val_hi, 15));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 1);
	__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

	carry = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(carry),
		_mm256_castsi256_pd(_mm256_castpd_si256(_mm256_and_pd(
			_mm256_castsi256_pd(cur_val_srl),
			_mm256_castsi256_pd(_mm256_set_epi16(
				0, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
				0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100)))))));

	__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_srli_epi16(cur_val_hi, 1), _mm_srli_epi16(cur_val_lo, 1))),
		_mm256_castsi256_pd(_mm256_set1_epi16(0x007F))));

	__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_epi16(cur_val_hi, 1), _mm_slli_epi16(cur_val_lo, 1))),
		_mm256_castsi256_pd(_mm256_set1_epi16(static_cast<int16_t>(0xFE00)))));

	__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(cur_val_srl),
		_mm256_castsi256_pd(_mm256_set1_epi16(0x0080))));

	cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(part3)));
	cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(carry)));
	carry = next_carry;
}

__forceinline void tm_avx_m256_8::alg_2(uint16_t* rng_seed)
{
	__m256i carry = _mm256_load_si256((__m256i*)(_alg2_256_8 + (*rng_seed) * 32));
	for (int i = 3; i >= 0; i--)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		alg_2_sub(cur_val, carry);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_8::alg_3(uint16_t* rng_seed)
{
	xor_alg(_regular_8 + (*rng_seed) * 128);
}

__forceinline void tm_avx_m256_8::alg_5_sub(__m256i& cur_val, __m256i& carry)
{
	__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
	__m128i cur_val_hi = _mm256_extractf128_si256(cur_val, 1);

	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)),
		_mm256_castsi256_pd(mask_top_80)));

	__m128i temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo, 1), _mm_slli_si128(cur_val_hi, 15));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 1);
	__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

	carry = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(carry),
		_mm256_castsi256_pd(_mm256_castpd_si256(_mm256_and_pd(
			_mm256_castsi256_pd(cur_val_srl),
			_mm256_castsi256_pd(_mm256_set_epi16(
				0, static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000),
				static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000),
				static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000),
				static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000))))))));

	__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_srli_epi16(cur_val_hi, 1), _mm_srli_epi16(cur_val_lo, 1))),
		_mm256_castsi256_pd(_mm256_set1_epi16(0x7F00))));

	__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_epi16(cur_val_hi, 1), _mm_slli_epi16(cur_val_lo, 1))),
		_mm256_castsi256_pd(_mm256_set1_epi16(0x00FE))));

	__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(cur_val_srl),
		_mm256_castsi256_pd(_mm256_set1_epi16(0x0001))));

	cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(part3)));
	cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(carry)));
	carry = next_carry;
}

__forceinline void tm_avx_m256_8::alg_5(uint16_t* rng_seed)
{
	__m256i carry = _mm256_load_si256((__m256i*)(_alg5_256_8 + (*rng_seed) * 32));
	for (int i = 3; i >= 0; i--)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		alg_5_sub(cur_val, carry);
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_8::alg_6(uint16_t* rng_seed)
{
	uint8_t* rng_start = _alg6_8 + (*rng_seed) * 128;
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m128i lo = _mm256_castsi256_si128(cur_val);
		__m128i hi = _mm256_extractf128_si256(cur_val, 1);
		lo = _mm_srli_epi16(lo, 1);
		hi = _mm_srli_epi16(hi, 1);
		cur_val = _mm256_set_m128i(hi, lo);
		cur_val = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask_7F)));
		__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start + i * 32));
		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_8::alg_7()
{
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		cur_val = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask_FF)));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_8::_run_alg(int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		uint8_t* rng_start = (algorithm_id == 4) ? _alg4_8 : _regular_8;
		add_alg(rng_seed, rng_start);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7();
	}
}

__forceinline void tm_avx_m256_8::_run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16_t rng_seed = static_cast<uint16_t>((schedule_entry.rng1 << 8) | schedule_entry.rng2);
	uint16_t nibble_selector = schedule_entry.nibble_selector;

	for (int i = 0; i < 16; i++)
	{
		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);

		uint8_t current_byte = working_code_data[i];
		if (nibble == 1)
			current_byte = static_cast<uint8_t>(current_byte >> 4);

		uint8_t algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);
		_run_alg(algorithm_id, &rng_seed);
	}
}

__forceinline void tm_avx_m256_8::_run_all_maps()
{
	for (auto it = schedule_entries->entries.begin(); it != schedule_entries->entries.end(); it++)
		_run_one_map(*it);
}

__forceinline void tm_avx_m256_8::_decrypt_carnival_world()
{
	xor_alg(carnival_world_data);
}

__forceinline void tm_avx_m256_8::_decrypt_other_world()
{
	xor_alg(other_world_data);
}

__forceinline void tm_avx_m256_8::mid_sum(__m128i& sum, __m256i& working_code, __m256i& sum_mask, __m128i& lo_mask)
{
	__m256i temp_masked = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(working_code), _mm256_castsi256_pd(sum_mask)));
	__m128i temp_lo = _mm256_castsi256_si128(temp_masked);
	__m128i temp_hi = _mm256_extractf128_si256(temp_masked, 1);
	sum = _mm_add_epi16(sum, _mm_and_si128(temp_lo, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(temp_lo, 8));
	sum = _mm_add_epi16(sum, _mm_and_si128(temp_hi, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(temp_hi, 8));
}

__forceinline uint16_t tm_avx_m256_8::masked_checksum(uint8_t* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);
	for (int i = 0; i < 4; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i sum_mask = _mm256_load_si256((__m256i*)(mask + i * 32));
		mid_sum(sum, cur_val, sum_mask, lo_mask);
	}
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

__forceinline uint16_t tm_avx_m256_8::_calculate_carnival_world_checksum()
{
	return masked_checksum(carnival_world_checksum_mask);
}

__forceinline uint16_t tm_avx_m256_8::_calculate_other_world_checksum()
{
	return masked_checksum(other_world_checksum_mask);
}

__forceinline uint16_t tm_avx_m256_8::fetch_checksum_value(uint8_t code_length)
{
	uint8_t checksum_low = working_code_data[127 - code_length];
	uint8_t checksum_hi = working_code_data[127 - (code_length + 1)];
	return static_cast<uint16_t>((checksum_hi << 8) | checksum_low);
}

__forceinline uint16_t tm_avx_m256_8::_fetch_carnival_world_checksum_value()
{
	return fetch_checksum_value(CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx_m256_8::_fetch_other_world_checksum_value()
{
	return fetch_checksum_value(OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx_m256_8::check_carnival_world_checksum()
{
	return _calculate_carnival_world_checksum() == _fetch_carnival_world_checksum_value();
}

__forceinline bool tm_avx_m256_8::check_other_world_checksum()
{
	return _calculate_other_world_checksum() == _fetch_other_world_checksum_value();
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx_m256_8::_decrypt_check()
{
	ALIGNED(32) uint8_t saved[128];
	_mm256_store_si256((__m256i*)saved,       _mm256_load_si256((__m256i*)working_code_data));
	_mm256_store_si256((__m256i*)(saved+32),  _mm256_load_si256((__m256i*)(working_code_data+32)));
	_mm256_store_si256((__m256i*)(saved+64),  _mm256_load_si256((__m256i*)(working_code_data+64)));
	_mm256_store_si256((__m256i*)(saved+96),  _mm256_load_si256((__m256i*)(working_code_data+96)));

	if constexpr (WORLD == CARNIVAL_WORLD)
		_decrypt_carnival_world();
	else
		_decrypt_other_world();

	if constexpr (CHECK_CHECKSUM)
	{
		bool ok = (WORLD == CARNIVAL_WORLD) ? check_carnival_world_checksum() : check_other_world_checksum();
		if (!ok)
		{
			_mm256_store_si256((__m256i*)working_code_data,      _mm256_load_si256((__m256i*)saved));
			_mm256_store_si256((__m256i*)(working_code_data+32), _mm256_load_si256((__m256i*)(saved+32)));
			_mm256_store_si256((__m256i*)(working_code_data+64), _mm256_load_si256((__m256i*)(saved+64)));
			_mm256_store_si256((__m256i*)(working_code_data+96), _mm256_load_si256((__m256i*)(saved+96)));
			return std::nullopt;
		}
	}

	auto result = check_machine_code(working_code_data, WORLD);
	_mm256_store_si256((__m256i*)working_code_data,      _mm256_load_si256((__m256i*)saved));
	_mm256_store_si256((__m256i*)(working_code_data+32), _mm256_load_si256((__m256i*)(saved+32)));
	_mm256_store_si256((__m256i*)(working_code_data+64), _mm256_load_si256((__m256i*)(saved+64)));
	_mm256_store_si256((__m256i*)(working_code_data+96), _mm256_load_si256((__m256i*)(saved+96)));
	return result;
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx_m256_8::_run_bruteforce(uint32_t data, uint8_t* result_data, uint32_t* result_size)
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

void tm_avx_m256_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
{
	for (uint32_t i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
			return;
		_run_bruteforce<true>(start_data + i, result_data, result_size);
		report_progress(static_cast<double>(i + 1) / static_cast<double>(amount_to_run));
	}
}

void tm_avx_m256_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	uint8_t result_data[2];
	uint32_t result_pos = 0;
	_run_bruteforce<false>(data, result_data, &result_pos);
	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_avx_m256_8::test_algorithm_chain(const uint8_t* algorithm_ids, int chain_length,
                                         uint8_t* data, uint16_t* rng_seed)
{
	load_data(data);
	for (int i = 0; i < chain_length; ++i)
	{
		_run_alg(algorithm_ids[i], rng_seed);
	}
	fetch_data(data);
}

void tm_avx_m256_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	load_data(data);
	for (int i = 0; i < iterations; i++)
		_run_alg(algorithm_id, rng_seed);
	fetch_data(data);
}

void tm_avx_m256_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	fetch_data(result_out);
}

void tm_avx_m256_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	_run_all_maps();
	fetch_data(result_out);
}

bool tm_avx_m256_8::test_bruteforce_checksum(uint32_t data, int world)
{
	_expand_code(data);
	_run_all_maps();
	if (world == CARNIVAL_WORLD)
		return _decrypt_check<true, CARNIVAL_WORLD>().has_value();
	else
		return _decrypt_check<true, OTHER_WORLD>().has_value();
}


