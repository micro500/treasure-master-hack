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
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = working_code_data[i] ^ rng->regular_rng_values_8[rng_seed * 128 + i];
	}
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

uint16 tm_8::generate_stats(uint32 key, uint32 data, key_schedule_entry* schedule_entries, bool use_hashing)
{
	expand(key, data);
	run_all_maps(schedule_entries);

	
	uint16 stats_result = 0;

	uint8 decrypted_data[128];
	decrypt_data(working_code_data, carnival_world_data, decrypted_data, CARNIVAL_WORLD_CODE_LENGTH);
	if (check_checksum(decrypted_data, CARNIVAL_WORLD_CODE_LENGTH))
	{
		stats_result |= 0x8000;
		stats_result |= (check_machine_code(decrypted_data, CARNIVAL_WORLD) << 8);
	}

	decrypt_data(working_code_data, other_world_data, decrypted_data, OTHER_WORLD_CODE_LENGTH);
	if (check_checksum(decrypted_data, OTHER_WORLD_CODE_LENGTH))
	{
		stats_result |= 0x0080;
		stats_result |= check_machine_code(decrypted_data, OTHER_WORLD);
	}

	return stats_result;
}

void tm_8::decrypt_data(uint8* data_in, uint8* data_to_decrypt, uint8* data_out, int length)
{
	for (int i = 128 - length; i < 128; i++)
	{
		data_out[i] = data_to_decrypt[i] ^ data_in[i];
	}
}

bool tm_8::check_checksum(uint8* data, int length)
{
	uint16 checksum_total = (data[reverse_offset(length - 1)] << 8) | data[reverse_offset(length - 2)];
	if (checksum_total > ((length - 2) * 0xFF))
	{
		return false;
	}

	uint16 sum = 0;
	for (int i = 0; i < length - 2; i++)
	{
		sum += data[reverse_offset(i)];
	}

	return sum == checksum_total;
}

bool tm_8::initialized = false;