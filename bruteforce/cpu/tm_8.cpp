#include <stdio.h>
#include <iostream>
#include "data_sizes.h"
#include "tm_8.h"
#include "key_schedule.h"

tm_8::tm_8(RNG * rng_obj) : TM_base(rng_obj)
{
	initialize();
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

void tm_8::expand(uint32 key, uint32 data)
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

	uint16 rng_seed = (key >> 16) & 0xFFFF;
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] += rng->expansion_values_8[rng_seed * 128 + i];
	}
}

void tm_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = new_data[i];
	}
}

void tm_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = working_code_data[i];
	}
}

void tm_8::run_alg(int algorithm_id, uint16 * rng_seed, int iterations)
{
	if (algorithm_id == 0)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_0(*rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 1)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_1(*rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_2(*rng_seed);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 3)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_3(*rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 4)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_4(*rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 5)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_5(*rng_seed);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_6(*rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 7)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg_7();
		}
	}
}

__forceinline void tm_8::alg_0(const uint16 rng_seed)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = (working_code_data[i] << 1) | rng->alg0_values_8[rng_seed * 128 + i];
	}
}

__forceinline void tm_8::alg_1(const uint16 rng_seed)
{
	add_alg(rng->regular_rng_values_8, rng_seed);
}

__forceinline void tm_8::alg_2( const uint16 rng_seed)
{
	uint8 carry = rng->alg2_values_8_8[rng_seed];
	for (int i = 127; i >= 0; i -= 2)
	{
		uint8 next_carry = working_code_data[i - 1] & 0x01;

		working_code_data[i - 1] = (working_code_data[i - 1] >> 1) | (working_code_data[i] & 0x80);
		working_code_data[i] = (working_code_data[i] << 1) | (carry & 0x01);

		carry = next_carry;
	}
}

__forceinline void tm_8::alg_3(const uint16 rng_seed)
{
	xor_alg(working_code_data, rng->regular_rng_values_8 + rng_seed * 128);
}

__forceinline void tm_8::alg_4(const uint16 rng_seed)
{
	add_alg(rng->alg4_values_8, rng_seed);
}

__forceinline void tm_8::alg_5(const uint16 rng_seed)
{
	uint8 carry = rng->alg5_values_8_8[rng_seed];

	for (int i = 127; i >= 0; i -= 2)
	{
		uint8 next_carry = working_code_data[i - 1] & 0x80;

		working_code_data[i - 1] = (working_code_data[i - 1] << 1) | (working_code_data[i] & 0x01);
		working_code_data[i] = (working_code_data[i] >> 1) | carry;

		carry = next_carry;
	}
}

__forceinline void tm_8::alg_6(const uint16 rng_seed)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = (working_code_data[i] >> 1) | rng->alg6_values_8[rng_seed * 128 + i];
	}
}

__forceinline void tm_8::alg_7()
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = working_code_data[i] ^ 0xFF;
	}
}

__forceinline void tm_8::add_alg(uint8* addition_values, const uint16 rng_seed)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = working_code_data[i] + addition_values[rng_seed * 128 + i];
	}
}

__forceinline void tm_8::xor_alg(uint8* working_data, uint8* xor_values)
{
	for (int i = 0; i < 128; i++)
	{
		working_data[i] = working_data[i] ^ xor_values[i];
	}
}

void tm_8::run_one_map(key_schedule_entry schedule_entry)
{
	uint16 rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
	uint16 nibble_selector = schedule_entry.nibble_selector;

	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		// Get the highest bit of the nibble selector to use as a flag
		unsigned char nibble = (nibble_selector >> 15) & 0x01;
		// Shift the nibble selector up one bit
		nibble_selector = nibble_selector << 1;

		// If the flag is a 1, get the high nibble of the current byte
		// Otherwise use the low nibble
		unsigned char current_byte = (uint8)working_code_data[i];

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char alg_id = (current_byte >> 1) & 0x07;

		run_alg(alg_id, &rng_seed, 1);
	}
}

void tm_8::run_all_maps(key_schedule_entry* schedule_entries)
{
	for (int schedule_counter = 0; schedule_counter < 27; schedule_counter++)
	{
		run_one_map(schedule_entries[schedule_counter]);
	}
}

void tm_8::run_bruteforce_data(uint32 key, uint32 start_data, key_schedule_entry* schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	uint32 output_pos = 0;
	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - output_pos) < 5)
		{
			*result_size = result_max_size;
			return;
		}
		uint32 data = start_data + i;

		expand(key, data);
		run_all_maps(schedule_entries);

		uint8 decrypted_data[128];
		for (int i = 0; i < 128; i++)
		{
			decrypted_data[i] = working_code_data[i];
		}
		_decrypt_carnival_world(decrypted_data);

		if (check_carnival_world_checksum(decrypted_data))
		{
			*((uint32*)(&result_data[output_pos])) = i;

			result_data[output_pos + 4] = check_machine_code(decrypted_data, CARNIVAL_WORLD);
			output_pos += 5;
		}

		for (int i = 0; i < 128; i++)
		{
			decrypted_data[i] = working_code_data[i];
		}
		_decrypt_other_world(decrypted_data);

		if (check_other_world_checksum(decrypted_data))
		{
			*((uint32*)(&result_data[output_pos])) = i;

			result_data[output_pos + 4] = check_machine_code(decrypted_data, OTHER_WORLD);
			output_pos += 5;
		}

		report_progress((float)(i + 1) / amount_to_run);
	}

	*result_size = output_pos;
}
__forceinline void tm_8::decrypt_carnival_world()
{
	_decrypt_carnival_world(working_code_data);
}

__forceinline void tm_8::decrypt_other_world()
{
	_decrypt_other_world(working_code_data);
}

__forceinline void tm_8::_decrypt_carnival_world(uint8* working_data)
{
	xor_alg(working_data, carnival_world_data);
}

__forceinline void tm_8::_decrypt_other_world(uint8* working_data)
{
	xor_alg(working_data, other_world_data);
}

__forceinline uint16 tm_8::calculate_masked_checksum(uint8* working_data, uint8* mask)
{
	uint16 sum = 0;
	for (int i = 0; i < 128; i++)
	{
		sum += working_data[i] & mask[i];
	}
	return sum;
}

__forceinline uint16 tm_8::_calculate_carnival_world_checksum(uint8* working_data)
{
	return calculate_masked_checksum(working_data, carnival_world_checksum_mask);
}

__forceinline uint16 tm_8::calculate_carnival_world_checksum()
{
	return _calculate_carnival_world_checksum(working_code_data);
}

__forceinline uint16 tm_8::_calculate_other_world_checksum(uint8* working_data)
{
	return calculate_masked_checksum(working_data, other_world_checksum_mask);
}

__forceinline uint16 tm_8::calculate_other_world_checksum()
{
	return _calculate_other_world_checksum(working_code_data);
}

__forceinline uint16 tm_8::fetch_checksum_value(uint8* working_data, int code_length)
{
	return working_data[reverse_offset(code_length - 1)] << 8 | working_data[reverse_offset(code_length - 2)];
}

__forceinline uint16 tm_8::_fetch_carnival_world_checksum_value(uint8* working_data)
{
	return fetch_checksum_value(working_data, CARNIVAL_WORLD_CODE_LENGTH);
}

__forceinline uint16 tm_8::fetch_carnival_world_checksum_value()
{
	return _fetch_carnival_world_checksum_value(working_code_data);
}

__forceinline uint16 tm_8::_fetch_other_world_checksum_value(uint8* working_data)
{
	return fetch_checksum_value(working_data, OTHER_WORLD_CODE_LENGTH);
}

__forceinline uint16 tm_8::fetch_other_world_checksum_value()
{
	return _fetch_other_world_checksum_value(working_code_data);
}

__forceinline bool tm_8::check_carnival_world_checksum(uint8* working_data)
{
	return _calculate_carnival_world_checksum(working_data) == _fetch_carnival_world_checksum_value(working_data);
}

__forceinline bool tm_8::check_other_world_checksum(uint8* working_data)
{
	return _calculate_other_world_checksum(working_data) == _fetch_other_world_checksum_value(working_data);
}


bool tm_8::initialized = false;