#include "tm_32_8.h"
#include "key_schedule.h"

tm_32_8::tm_32_8(RNG* rng_obj) : tm_32_8(rng_obj, 0) {}

tm_32_8::tm_32_8(RNG* rng_obj, const uint32_t key) : tm_32_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_32_8::tm_32_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule) : TM_base(rng_obj)
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule;
}

__forceinline void tm_32_8::initialize()
{
	if (!_initialized)
	{
		auto _r0 = rng->generate_expansion_values_8();
		auto _r1 = rng->generate_seed_forward_1();
		auto _r2 = rng->generate_seed_forward_128();
		auto _r3 = rng->generate_regular_rng_values_8();
		auto _r4 = rng->generate_regular_rng_values_8_lo();
		auto _r5 = rng->generate_regular_rng_values_8_hi();
		auto _r6 = rng->generate_alg0_values_8();
		auto _r7 = rng->generate_alg2_values_32_8();
		auto _r8 = rng->generate_alg4_values_8();
		auto _r9 = rng->generate_alg4_values_8_lo();
		auto _r10 = rng->generate_alg4_values_8_hi();
		auto _r11 = rng->generate_alg5_values_32_8();
		auto _r12 = rng->generate_alg6_values_8();

		_expansion_8 = static_cast<uint8_t*>(_r0.get());
		_seed_fwd_1 = static_cast<uint16_t*>(_r1.get());
		_seed_fwd_128 = static_cast<uint16_t*>(_r2.get());
		_regular_8 = static_cast<uint8_t*>(_r3.get());
		_regular_8_lo = static_cast<uint8_t*>(_r4.get());
		_regular_8_hi = static_cast<uint8_t*>(_r5.get());
		_alg0_8 = static_cast<uint8_t*>(_r6.get());
		_alg2_32_8 = static_cast<uint32_t*>(_r7.get());
		_alg4_8 = static_cast<uint8_t*>(_r8.get());
		_alg4_8_lo = static_cast<uint8_t*>(_r9.get());
		_alg4_8_hi = static_cast<uint8_t*>(_r10.get());
		_alg5_32_8 = static_cast<uint32_t*>(_r11.get());
		_alg6_8 = static_cast<uint8_t*>(_r12.get());

		_table_refs = { _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7, _r8, _r9, _r10, _r11, _r12 };
		_initialized = true;
	}
	obj_name = "tm_32_8";
}

__forceinline void tm_32_8::_expand_code(uint32_t data)
{
	uint8_t* x = (uint8_t*)working_code_data;
	for (int i = 0; i < 128; i += 8)
	{
		x[i] = (key >> 24) & 0xFF;
		x[i + 1] = (key >> 16) & 0xFF;
		x[i + 2] = (key >> 8) & 0xFF;
		x[i + 3] = key & 0xFF;

		x[i + 4] = (data >> 24) & 0xFF;
		x[i + 5] = (data >> 16) & 0xFF;
		x[i + 6] = (data >> 8) & 0xFF;
		x[i + 7] = data & 0xFF;
	}

	uint16_t rng_seed = (key >> 16) & 0xFFFF;
	for (int i = 0; i < 128; i++)
	{
		x[i] += _expansion_8[rng_seed * 128 + i];
	}
}

__forceinline void tm_32_8::load_data(uint8_t* src)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8_t*)working_code_data)[i] = src[i];
	}
}

__forceinline void tm_32_8::fetch_data(uint8_t* dst)
{
	for (int i = 0; i < 128; i++)
	{
		dst[i] = ((uint8_t*)working_code_data)[i];
	}
}

__forceinline void tm_32_8::_run_alg(int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(*rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 1)
	{
		add_alg((uint32_t*)_regular_8_lo, (uint32_t*)_regular_8_hi, *rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(*rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(*rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 4)
	{
		add_alg((uint32_t*)_alg4_8_lo, (uint32_t*)_alg4_8_hi, *rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(*rng_seed);
		*rng_seed = _seed_fwd_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(*rng_seed);
		*rng_seed = _seed_fwd_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7();
	}
}

__forceinline void tm_32_8::alg_0(const uint16_t rng_seed)
{
	for (int i = 0; i < 32; i++)
	{
		working_code_data[i] = ((working_code_data[i] << 1) & 0xFEFEFEFE) | *(uint32_t*)(_alg0_8 + (rng_seed * 128) + i * 4);
	}
}

__forceinline void tm_32_8::alg_2(const uint16_t rng_seed)
{
	uint32_t carry = _alg2_32_8[rng_seed];
	for (int i = 31; i >= 0; i--)
	{
		uint32_t next_carry = (working_code_data[i] & 0x00000001) << 24;

		working_code_data[i] = ((working_code_data[i] & 0x00010000) >> 8) | ((working_code_data[i] >> 1) & 0x007F007F) | ((working_code_data[i] << 1) & 0xFE00FE00) | ((working_code_data[i] >> 8) & 0x00800080) | carry;

		carry = next_carry;
	}
}

__forceinline void tm_32_8::xor_alg(uint32_t* working_data, const uint8_t* values)
{
	for (int i = 0; i < 32; i++)
	{
		working_data[i] = working_data[i] ^ ((uint32_t*)values)[i];
	}
}

__forceinline void tm_32_8::alg_3(const uint16_t rng_seed)
{
	xor_alg(working_code_data, &_regular_8[rng_seed * 128]);
}

__forceinline void tm_32_8::alg_5(const uint16_t rng_seed)
{
	uint32_t carry = _alg5_32_8[rng_seed];
	for (int i = 31; i >= 0; i--)
	{
		uint32_t next_carry = (working_code_data[i] & 0x00000080) << 24;

		working_code_data[i] = ((working_code_data[i] & 0x00800000) >> 8) | ((working_code_data[i] << 1) & 0x00FE00FE) | ((working_code_data[i] >> 1) & 0x7F007F00) | ((working_code_data[i] >> 8) & 0x00010001) | carry;

		carry = next_carry;
	}
}

__forceinline void tm_32_8::alg_6(const uint16_t rng_seed)
{
	for (int i = 0; i < 32; i++)
	{
		working_code_data[i] = ((working_code_data[i] >> 1) & 0x7F7F7F7F) | ((uint32_t*)_alg6_8)[(rng_seed * 128) / 4 + i];
	}
}

__forceinline void tm_32_8::alg_7()
{
	for (int i = 0; i < 32; i++)
	{
		working_code_data[i] = working_code_data[i] ^ 0xFFFFFFFF;
	}
}

__forceinline void tm_32_8::add_alg(uint32_t* addition_values_lo, uint32_t* addition_values_hi, const uint16_t rng_seed)
{
	for (int i = 0; i < 32; i++)
	{
		working_code_data[i] = (((working_code_data[i] & 0x00FF00FF) + addition_values_hi[(rng_seed * 128) / 4 + i]) & 0x00FF00FF) | (((working_code_data[i] & 0xFF00FF00) + addition_values_lo[(rng_seed * 128) / 4 + i]) & 0xFF00FF00);
	}
}

__forceinline void tm_32_8::_run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
{
	uint16_t rng_seed = static_cast<uint16_t>((schedule_entry.rng1 << 8) | schedule_entry.rng2);
	uint16_t nibble_selector = schedule_entry.nibble_selector;

	for (int i = 0; i < 16; i++)
	{
		uint8_t nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector = static_cast<uint16_t>(nibble_selector << 1);

		uint8_t current_byte = ((uint8_t*)working_code_data)[i];
		if (nibble == 1)
		{
			current_byte = static_cast<uint8_t>(current_byte >> 4);
		}

		uint8_t alg_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);
		_run_alg(alg_id, &rng_seed);
	}
}

__forceinline void tm_32_8::_run_all_maps()
{
	for (auto it = schedule_entries->entries.begin(); it != schedule_entries->entries.end(); it++)
	{
		_run_one_map(*it);
	}
}

__forceinline void tm_32_8::_decrypt_carnival_world(uint8_t* working_data)
{
	xor_alg((uint32_t*)working_data, carnival_world_data);
}

__forceinline void tm_32_8::_decrypt_other_world(uint8_t* working_data)
{
	xor_alg((uint32_t*)working_data, other_world_data);
}

__forceinline bool tm_32_8::check_carnival_world_checksum(uint8_t* working_data)
{
	return _calculate_carnival_world_checksum(working_data) == _fetch_carnival_world_checksum_value(working_data);
}

__forceinline bool tm_32_8::check_other_world_checksum(uint8_t* working_data)
{
	return _calculate_other_world_checksum(working_data) == _fetch_other_world_checksum_value(working_data);
}

__forceinline uint16_t tm_32_8::_calculate_carnival_world_checksum(uint8_t* working_data)
{
	return masked_checksum(working_data, carnival_world_checksum_mask);
}

__forceinline uint16_t tm_32_8::_calculate_other_world_checksum(uint8_t* working_data)
{
	return masked_checksum(working_data, other_world_checksum_mask);
}

__forceinline uint16_t tm_32_8::fetch_checksum_value(uint8_t* working_data, uint8_t code_length)
{
	return static_cast<uint16_t>(working_data[reverse_offset(code_length - 1)] << 8 | working_data[reverse_offset(code_length - 2)]);
}

__forceinline uint16_t tm_32_8::_fetch_carnival_world_checksum_value(uint8_t* working_data)
{
	return fetch_checksum_value(working_data, CARNIVAL_WORLD_CODE_LENGTH);
}

__forceinline uint16_t tm_32_8::_fetch_other_world_checksum_value(uint8_t* working_data)
{
	return fetch_checksum_value(working_data, OTHER_WORLD_CODE_LENGTH);
}

__forceinline uint16_t tm_32_8::masked_checksum(uint8_t* working_data, uint8_t* mask)
{
	uint16_t sum = 0;
	for (int i = 0; i < 128; i++)
	{
		sum += working_data[i] & mask[i];
	}
	return sum;
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_32_8::_decrypt_check()
{
	fetch_data(decrypted_data);

	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(decrypted_data);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_carnival_world_checksum(decrypted_data))
				return std::nullopt;
		}
	}
	else
	{
		_decrypt_other_world(decrypted_data);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum(decrypted_data))
				return std::nullopt;
		}
	}

	return check_machine_code(decrypted_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_32_8::_run_bruteforce(uint32_t data, uint8_t* result_data, uint32_t* result_size)
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

void tm_32_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
{
	uint32_t output_pos = 0;
	for (uint32_t i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - output_pos) < 5)
		{
			*result_size = result_max_size;
			return;
		}
		uint32_t data = start_data + i;
		_run_bruteforce<true>(data, result_data, &output_pos);
		report_progress(static_cast<double>(i + 1) / static_cast<double>(amount_to_run));
	}
	*result_size = output_pos;
}

void tm_32_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	uint8_t result_data[2];
	uint32_t result_size = 0;
	_run_bruteforce<false>(data, result_data, &result_size);
	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_32_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	load_data(data);
	_run_alg(algorithm_id, rng_seed);
	fetch_data(data);
}

void tm_32_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	load_data(data);
	for (int i = 0; i < iterations; ++i)
	{
		_run_alg(algorithm_id, rng_seed);
	}
	fetch_data(data);
}

void tm_32_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	fetch_data(result_out);
}

void tm_32_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	_run_all_maps();
	fetch_data(result_out);
}

bool tm_32_8::test_bruteforce_checksum(uint32_t data, int world)
{
	_expand_code(data);
	_run_all_maps();
	fetch_data(decrypted_data);
	if (world == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(decrypted_data);
		return check_carnival_world_checksum(decrypted_data);
	}
	else
	{
		_decrypt_other_world(decrypted_data);
		return check_other_world_checksum(decrypted_data);
	}
}

