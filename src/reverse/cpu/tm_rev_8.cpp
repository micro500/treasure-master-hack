#include <stdio.h>
#include <iostream>
#include "tm_rev_8.h"

tm_rev_8::tm_rev_8(RNG* rng_obj) : TM_rev_base(rng_obj)
{
	initialize();
}

__forceinline void tm_rev_8::initialize()
{
	if (!initialized)
	{
		rng->generate_seed_forward();

		rng->generate_regular_rng_values_8();

		rng->generate_alg06_values_8();

		rng->generate_alg4_values_8();

		initialized = true;
	}
	obj_name = "tm_rev_8";
}

void tm_rev_8::set_working_code(uint8_t* data)
{
	for (int i = 0; i < 0x80; i++)
	{
		init_working_code_data[i] = data[i];
	}
}

void tm_rev_8::set_trust_mask(uint8_t* data)
{
	for (int i = 0; i < 0x80; i++)
	{
		init_trust_mask[i] = data[i];
	}
}

void tm_rev_8::set_rng_seed(uint16_t seed)
{
	rng_seed_forward = &(rng->seed_forward[seed * 2048]);
}

rev_stats tm_rev_8::check_alg06()
{
	int alg0_available_bits = 0;
	int alg0_mismatch_bits = 0;

	int alg6_available_bits = 0;
	int alg6_mismatch_bits = 0;

	uint8_t* rng_table = rng->alg06_values_8 + (rng_seed_forward[0] * 128);

	for (int i = 0; i < 0x80; i++)
	{
		uint8_t cur_mask = trust_mask[i] & 0x81;

		uint8_t working_val = working_code_data[i] & cur_mask;
		uint8_t rng_val = rng_table[127-i] & cur_mask;

		alg0_available_bits += cur_mask & 0x01;
		alg6_available_bits += (cur_mask >> 7) & 0x01;

		uint8_t mismatch = working_val ^ rng_val;

		alg0_mismatch_bits += mismatch & 0x01;
		alg6_mismatch_bits += (mismatch >> 7) & 0x01;
	}

	return rev_stats(alg0_available_bits, alg0_mismatch_bits, alg6_available_bits, alg6_mismatch_bits);
}

void tm_rev_8::_load_from_mem()
{
	for (int i = 0; i < 0x80; i++)
	{
		working_code_data[i] = init_working_code_data[i];
		trust_mask[i] = init_trust_mask[i];
	}
}

rev_stats tm_rev_8::run_reverse_process()
{
	_load_from_mem();

	for (int i = 0; i < rev_alg_list_length; i++)
	{
		int alg = rev_alg_list[i];
		uint16_t rng_seed = rng_seed_forward[alg_rng_seed_diff[i]];
		switch (alg)
		{
		case 0:
			rev_alg_0();
			break;
		case 1:
			rev_alg_1(rng_seed);
			break;
		case 2:
			rev_alg_2();
			break;
		case 3:
			rev_alg_3(rng_seed);
			break;
		case 4:
			rev_alg_4(rng_seed);
			break;
		case 5:
			rev_alg_5();
			break;
		case 6:
			rev_alg_6();
			break;
		case 7:
			rev_alg_7();
			break;
		}
	}

	return check_alg06();
}

__forceinline void tm_rev_8::add_alg(uint8_t* addition_values, const uint16_t rng_seed)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_data[i] = working_code_data[i] + addition_values[(rng_seed * 128) + i];
		trust_mask[i] = trust_mask[i] & ((trust_mask[i] ^ 0xff) - 1);
	}
}

__forceinline void tm_rev_8::xor_alg(uint8_t* working_data, uint8_t* xor_values)
{
	for (int i = 0; i < 128; i++)
	{
		working_data[i] = working_data[i] ^ xor_values[i];
	}
}

void tm_rev_8::rev_alg_0()
{
	for (int i = 0x7F; i >= 0; i--)
	{
		working_code_data[i] = (working_code_data[i] >> 1) & 0x7F;
		trust_mask[i] = (trust_mask[i] >> 1) & 0x7F;
	}
}

void tm_rev_8::rev_alg_1(uint16_t rng_seed)
{
	add_alg(rng->alg4_values_8, rng_seed);
}

void tm_rev_8::rev_alg_2()
{
	unsigned char wc_carry = 0;
	unsigned char tm_carry = 0;

	for (int i = 0; i < 0x80; i += 2)
	{
		// Save bit 0 of byte 1 as the new carry
		unsigned char next_wc_carry = working_code_data[i + 1] & 0x01;
		unsigned char next_tm_carry = trust_mask[i + 1] & 0x01;

		// Shift byte 1 right
		// Set byte 1, bit 7 as byte 0, bit 7
		working_code_data[i + 1] = (working_code_data[i + 1] >> 1) | (working_code_data[i] & 0x80);
		trust_mask[i + 1] = (trust_mask[i + 1] >> 1) | (trust_mask[i] & 0x80);

		// Shift byte 0 left
		// Set byte 0, bit 0 as old carry
		working_code_data[i] = (working_code_data[i] << 1) | wc_carry;
		trust_mask[i] = (trust_mask[i] << 1) | tm_carry;

		wc_carry = next_wc_carry;
		tm_carry = next_tm_carry;
	}
}

void tm_rev_8::rev_alg_3(uint16_t rng_seed)
{
	xor_alg(working_code_data, rng->regular_rng_values_8 + rng_seed * 128);
}

void tm_rev_8::rev_alg_4(uint16_t rng_seed)
{
	add_alg(rng->regular_rng_values_8, rng_seed);
}

void tm_rev_8::rev_alg_5()
{
	unsigned char wc_carry = 0;
	unsigned char tm_carry = 0;

	for (int i = 0; i < 0x80; i += 2)
	{
		// Save bit 7 of byte 1 as the new carry
		unsigned char next_wc_carry = working_code_data[i + 1] & 0x80;
		unsigned char next_tm_carry = trust_mask[i + 1] & 0x80;

		// Shift byte 1 left
		// Set byte 1, bit 0 as byte 0, bit 0
		working_code_data[i + 1] = (working_code_data[i + 1] << 1) | (working_code_data[i] & 0x01);
		trust_mask[i + 1] = (trust_mask[i + 1] << 1) | (trust_mask[i] & 0x01);

		// Shift byte 0 right
		// Set byte 0, bit 7 as old carry
		working_code_data[i] = (working_code_data[i] >> 1) | wc_carry;
		trust_mask[i] = (trust_mask[i] >> 1) | tm_carry;

		wc_carry = next_wc_carry;
		tm_carry = next_tm_carry;
	}
}

void tm_rev_8::rev_alg_6()
{
	for (int i = 0x00; i <= 0x7f; i++)
	{
		working_code_data[i] = (working_code_data[i] << 1) & 0xFE;
		trust_mask[i] = (trust_mask[i] << 1) & 0xFE;
	}
}

void tm_rev_8::rev_alg_7()
{
	for (int i = 0x00; i < 0x80; i++)
	{
		working_code_data[i] = working_code_data[i] ^ 0xFF;
	}
}


bool tm_rev_8::initialized = false;