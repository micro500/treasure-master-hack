#include "tm_8.h"
#include "key_schedule.h"

tm_8::tm_8(RNG* rng_obj) : tm_8(rng_obj, 0) {}

tm_8::tm_8(RNG* rng_obj, const uint32_t key) : tm_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_8::tm_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj)
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;
}

__forceinline void tm_8::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();

		rng->generate_alg0_values_8();
		rng->generate_alg2_values_8_8();
		rng->generate_alg4_values_8();
		rng->generate_alg5_values_8_8();
		rng->generate_alg6_values_8();

		initialized = true;
	}
	obj_name = "tm_8";
}

__forceinline void tm_8::_expand_code(uint32_t data)
{
	for (int i = 0; i < 128; i += 8)
	{
		working_code_data[i] = (key >> 24) & 0xFF;
		working_code_data[i + 1] = (key >> 16) & 0xFF;
		working_code_data[i + 2] = (key >> 8) & 0xFF;
		working_code_data[i + 3] = key & 0xFF;

		working_code_data[i + 4] = (data >> 24) & 0xFF;
		working_code_data[i + 5] = (data >> 16) & 0xFF;
		working_code_data[i + 6] = (data >> 8) & 0xFF;
		working_code_data[i + 7] = data & 0xFF;
	}

	uint16_t rng_seed = (key >> 16) & 0xFFFF;
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] += rng->expansion_values_8[rng_seed * 128 + i];
	}
}

__forceinline void tm_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = new_data[i];
	}
}

__forceinline void tm_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = working_code_data[i];
	}
}

__forceinline void tm_8::_run_alg(int algorithm_id, uint16_t* rng_seed)
{
	if (algorithm_id == 0)
	{
		alg_0(*rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 1)
	{
		add_alg(rng->regular_rng_values_8, *rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 2)
	{
		alg_2(*rng_seed);
		*rng_seed = rng->seed_forward_1[*rng_seed];
	}
	else if (algorithm_id == 3)
	{
		alg_3(*rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 4)
	{
		add_alg(rng->alg4_values_8, *rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 5)
	{
		alg_5(*rng_seed);
		*rng_seed = rng->seed_forward_1[*rng_seed];
	}
	else if (algorithm_id == 6)
	{
		alg_6(*rng_seed);
		*rng_seed = rng->seed_forward_128[*rng_seed];
	}
	else if (algorithm_id == 7)
	{
		alg_7();
	}
}

__forceinline void tm_8::alg_0(uint16_t rng_seed)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = static_cast<uint8_t>((working_code_data[i] << 1) | rng->alg0_values_8[rng_seed * 128 + i]);
	}
}

__forceinline void tm_8::alg_2(uint16_t rng_seed)
{
	uint8_t carry = rng->alg2_values_8_8[rng_seed];
	for (int i = 127; i >= 0; i -= 2)
	{
		uint8_t next_carry = static_cast<uint8_t>(working_code_data[i - 1] & 0x01);

		working_code_data[i - 1] = static_cast<uint8_t>((working_code_data[i - 1] >> 1) | (working_code_data[i] & 0x80));
		working_code_data[i] = static_cast<uint8_t>((working_code_data[i] << 1) | (carry & 0x01));

		carry = next_carry;
	}
}

__forceinline void tm_8::alg_3(uint16_t rng_seed)
{
	xor_alg(working_code_data, rng->regular_rng_values_8 + rng_seed * 128);
}

__forceinline void tm_8::alg_5(uint16_t rng_seed)
{
	uint8_t carry = rng->alg5_values_8_8[rng_seed];

	for (int i = 127; i >= 0; i -= 2)
	{
		uint8_t next_carry = static_cast<uint8_t>(working_code_data[i - 1] & 0x80);

		working_code_data[i - 1] = static_cast<uint8_t>((working_code_data[i - 1] << 1) | (working_code_data[i] & 0x01));
		working_code_data[i] = static_cast<uint8_t>((working_code_data[i] >> 1) | carry);

		carry = next_carry;
	}
}

__forceinline void tm_8::alg_6(uint16_t rng_seed)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = static_cast<uint8_t>((working_code_data[i] >> 1) | rng->alg6_values_8[rng_seed * 128 + i]);
	}
}

__forceinline void tm_8::alg_7()
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = static_cast<uint8_t>(working_code_data[i] ^ 0xFF);
	}
}

__forceinline void tm_8::add_alg(uint8_t* addition_values, uint16_t rng_seed)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = static_cast<uint8_t>(working_code_data[i] + addition_values[rng_seed * 128 + i]);
	}
}

__forceinline void tm_8::xor_alg(uint8_t* working_data, uint8_t* xor_values)
{
	for (int i = 0; i < 128; i++)
	{
		working_data[i] = static_cast<uint8_t>(working_data[i] ^ xor_values[i]);
	}
}

__forceinline void tm_8::_run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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

		// If the flag is a 1, get the high nibble of the current byte
		// Otherwise use the low nibble
		uint8_t current_byte = static_cast<uint8_t>(working_code_data[i]);
		if (nibble == 1)
		{
			current_byte = static_cast<uint8_t>(current_byte >> 4);
		}

		// Mask off only 3 bits
		uint8_t alg_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);
		_run_alg(alg_id, &rng_seed);
	}
}

__forceinline void tm_8::_run_all_maps()
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries->entries.begin(); it != schedule_entries->entries.end(); it++)
	{
		_run_one_map(*it);
	}
}

__forceinline void tm_8::_decrypt_carnival_world(uint8_t* out)
{
	fetch_data(out);
	xor_alg(out, carnival_world_data);
}

__forceinline void tm_8::_decrypt_other_world(uint8_t* out)
{
	fetch_data(out);
	xor_alg(out, other_world_data);
}

__forceinline uint16_t tm_8::masked_checksum(uint8_t* working_data, uint8_t* mask)
{
	uint16_t sum = 0;
	for (int i = 0; i < 128; i++)
	{
		sum += working_data[i] & mask[i];
	}
	return sum;
}

__forceinline uint16_t tm_8::_calculate_carnival_world_checksum(uint8_t* working_data)
{
	return masked_checksum(working_data, carnival_world_checksum_mask);
}

__forceinline uint16_t tm_8::_calculate_other_world_checksum(uint8_t* working_data)
{
	return masked_checksum(working_data, other_world_checksum_mask);
}

__forceinline uint16_t tm_8::fetch_checksum_value(uint8_t* working_data, uint8_t code_length)
{
	return static_cast<uint16_t>(working_data[reverse_offset(code_length - 1)] << 8 | working_data[reverse_offset(code_length - 2)]);
}

__forceinline uint16_t tm_8::_fetch_carnival_world_checksum_value(uint8_t* working_data)
{
	return fetch_checksum_value(working_data, CARNIVAL_WORLD_CODE_LENGTH);
}

__forceinline uint16_t tm_8::_fetch_other_world_checksum_value(uint8_t* working_data)
{
	return fetch_checksum_value(working_data, OTHER_WORLD_CODE_LENGTH);
}

__forceinline bool tm_8::check_carnival_world_checksum(uint8_t* data)
{
	return _calculate_carnival_world_checksum(data) == _fetch_carnival_world_checksum_value(data);
}

__forceinline bool tm_8::check_other_world_checksum(uint8_t* data)
{
	return _calculate_other_world_checksum(data) == _fetch_other_world_checksum_value(data);
}

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_8::_decrypt_check()
{
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(decrypted_data);

		if constexpr (CHECK_CHECKSUM) {
			if (!check_carnival_world_checksum(decrypted_data))
			{
				return std::nullopt;
			}
		}
	}
	else
	{
		_decrypt_other_world(decrypted_data);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum(decrypted_data))
			{
				return std::nullopt;
			}
		}
	}
	return check_machine_code(decrypted_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_8::_run_bruteforce(uint32_t data, uint8_t* result_data, uint32_t* result_size)
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
		result_data[*result_size+1] = other_flags.value();
		*result_size += 2;
	}
}

void tm_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
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

void tm_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	uint8_t result_data[2];
	uint32_t result_size = 0;
	_run_bruteforce<false>(data, result_data, &result_size);
	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

void tm_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	load_data(data);
	_run_alg(algorithm_id, rng_seed);
	fetch_data(data);
}

void tm_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	load_data(data);
	for (int i = 0; i < iterations; ++i)
	{
		_run_alg(algorithm_id, rng_seed);
	}
	fetch_data(data);
}

void tm_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	fetch_data(result_out);
}

void tm_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	_expand_code(data);
	_run_all_maps();
	fetch_data(result_out);
}

bool tm_8::test_bruteforce_checksum(uint32_t data, int world)
{
	_expand_code(data);
	_run_all_maps();
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

bool tm_8::initialized = false;