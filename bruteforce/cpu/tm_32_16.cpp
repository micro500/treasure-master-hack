#include <iostream>
#include "data_sizes.h"
#include "tm_32_16.h"
#include "key_schedule.h"

tm_32_16::tm_32_16(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_32_16::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_16();

		rng->generate_alg0_values_16();
		rng->generate_alg2_values_32_16();
		rng->generate_alg4_values_16();
		rng->generate_alg5_values_32_16();
		rng->generate_alg6_values_16();

		initialized = true;
	}
	obj_name = "tm_32_16";
}

void tm_32_16::expand(uint32 key, uint32 data)
{
	uint16* x = (uint16*)working_code_data;
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

	uint16 rng_seed = (key >> 16) & 0xFFFF;
	for (int i = 0; i < 128; i++)
	{
		x[i] += rng->expansion_values_8[rng_seed * 128 + i];
		x[i] = x[i] & 0xFF;
	}
}

void tm_32_16::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint16*)working_code_data)[i] = new_data[i];
	}
}

void tm_32_16::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint16*)working_code_data)[i];
	}
}

void tm_32_16::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
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

__forceinline void tm_32_16::alg_0(const uint16 rng_seed)
{
	for (int i = 0; i < 64; i++)
	{
		working_code_data[i] = ((working_code_data[i] << 1) | ((uint32*)rng->alg0_values_16)[(rng_seed * 128) / 2 + i]) & 0xFF00FF;
	}
}

__forceinline void tm_32_16::alg_1(const uint16 rng_seed)
{
	add_alg((uint32*)rng->regular_rng_values_16, rng_seed);
}

__forceinline void tm_32_16::alg_2(const uint16 rng_seed)
{
	uint32 carry = rng->alg2_values_32_16[rng_seed];
	for (int i = 63; i >= 0; i--)
	{
		uint32 next_carry = (working_code_data[i] & 0x00000001) << 16;

		working_code_data[i] = (((working_code_data[i] & 0x000000FF) >> 1) & 0x7F) | (((working_code_data[i] & 0x00FF0000) << 1) & 0x00FE0000) | ((working_code_data[i] & 0x00800000) >> 16) | carry;

		carry = next_carry;
	}
}

__forceinline void tm_32_16::alg_3(const uint16 rng_seed)
{
	for (int i = 0; i < 64; i++)
	{
		working_code_data[i] = working_code_data[i] ^ ((uint32*)rng->regular_rng_values_16)[(rng_seed * 128) / 2 + i];
	}
}

__forceinline void tm_32_16::alg_4(const uint16 rng_seed)
{
	add_alg((uint32*)rng->alg4_values_16, rng_seed);
}

__forceinline void tm_32_16::alg_5(const uint16 rng_seed)
{
	uint32 carry = rng->alg5_values_32_16[rng_seed];
	for (int i = 63; i >= 0; i--)
	{
		uint32 next_carry = (working_code_data[i] & 0x00000080) << 16;

		working_code_data[i] = (((working_code_data[i] & 0x000000FF) << 1) & 0xFE) | (((working_code_data[i] & 0x00FF0000) >> 1) & 0x007F0000) | ((working_code_data[i] & 0x00010000) >> 16) | carry;

		carry = next_carry;
	}
}

__forceinline void tm_32_16::alg_6(const uint16 rng_seed)
{
	for (int i = 0; i < 64; i++)
	{
		working_code_data[i] = ((working_code_data[i] >> 1) | ((uint32*)rng->alg6_values_16)[(rng_seed * 128) / 2 + i]) & 0xFF00FF;
	}
}

__forceinline void tm_32_16::alg_7()
{
	for (int i = 0; i < 64; i++)
	{
		working_code_data[i] = working_code_data[i] ^ 0xFF00FF;
	}
}

__forceinline void tm_32_16::add_alg(uint32* addition_values, const uint16 rng_seed)
{
	for (int i = 0; i < 64; i++)
	{
		working_code_data[i] = (working_code_data[i] + addition_values[(rng_seed * 128) / 2 + i]) & 0xFF00FF;
	}
}

void tm_32_16::run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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
		unsigned char current_byte = ((uint16*)working_code_data)[i];

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char alg_id = (current_byte >> 1) & 0x07;

		run_alg(alg_id, &rng_seed, 1);
	}
}

void tm_32_16::run_all_maps(const key_schedule& schedule_entries)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries.entries.begin(); it != schedule_entries.entries.end(); it++)
	{
		run_one_map(*it);
	}
}

bool tm_32_16::initialized = false;
