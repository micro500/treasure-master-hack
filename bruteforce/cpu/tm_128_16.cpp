#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2

#include "data_sizes.h"
#include "tm_128_16.h"

tm_128_16::tm_128_16(RNG *rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_128_16::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_16();

		rng->generate_alg0_values_16();
		rng->generate_alg2_values_128_16();
		rng->generate_alg4_values_16();
		rng->generate_alg5_values_128_16();
		rng->generate_alg6_values_16();

		initialized = true;
	}
	obj_name = "tm_128_16";
}

void tm_128_16::expand(uint32 key, uint32 data)
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

void tm_128_16::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint16*)working_code_data)[i] = new_data[i];
	}
}

void tm_128_16::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint16*)working_code_data)[i];
	}
}

void tm_128_16::run_alg(int algorithm_id, uint16 * rng_seed, int iterations)
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


void tm_128_16::alg_0(const uint16 rng_seed)
{
	for (int i = 0; i < 16; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code_data + i * 16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(((uint8*)rng->alg0_values_16) + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_slli_epi16(cur_val, 1);
		cur_val = _mm_or_si128(cur_val, rng_val);

		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128(cur_val, mask);

		_mm_store_si128((__m128i *)(working_code_data + i * 16), cur_val);
	}
}

void tm_128_16::alg_1(const uint16 rng_seed)
{
	add_alg(((uint8*)rng->regular_rng_values_16), rng_seed);
}

void tm_128_16::alg_2(const uint16 rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i *)(((uint8*)rng->alg2_values_128_16) + (rng_seed * 16)));
	for (int i = 15; i >= 0; i--)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code_data + i * 16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val, _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 1)), 14);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val, _mm_set_epi16(0, 1, 0, 1, 0, 1, 0, 0)), 2));

		//_mm_and_si128(_mm_slli_si128(cur_val,15),_mm_set_epi16(0,0x80,0,0x80,0,0x80,0,0x80));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val, 1),
						_mm_set_epi16(0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val, 1),
						_mm_set_epi16(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0))),
				_mm_and_si128(
					_mm_srli_si128(cur_val, 2),
					_mm_set_epi16(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80))),
			carry);

		_mm_store_si128((__m128i *)(working_code_data + i * 16), cur_val);

		carry = next_carry;
	}
}

void tm_128_16::alg_3(const uint16 rng_seed)
{
	for (int i = 0; i < 16; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code_data + i * 16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(((uint8*)rng->regular_rng_values_16) + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_xor_si128(cur_val, rng_val);

		_mm_store_si128((__m128i *)(working_code_data + i * 16), cur_val);
	}
}

void tm_128_16::alg_4(const uint16 rng_seed)
{
	add_alg(((uint8*)rng->alg4_values_16), rng_seed);
}

void tm_128_16::alg_5(const uint16 rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i *)(((uint8*)rng->alg5_values_128_16) + (rng_seed * 16)));
	for (int i = 15; i >= 0; i--)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code_data + i * 16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val, _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 0x80)), 14);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val, _mm_set_epi16(0, 0x80, 0, 0x80, 0, 0x80, 0, 0)), 2));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val, 1),
						_mm_set_epi16(0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val, 1),
						_mm_set_epi16(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE))),
				_mm_and_si128(
					_mm_srli_si128(cur_val, 2),
					_mm_set_epi16(0, 1, 0, 1, 0, 1, 0, 1))),
			carry);

		_mm_store_si128((__m128i *)(working_code_data + i * 16), cur_val);

		carry = next_carry;
	}
}


void tm_128_16::alg_6(const uint16 rng_seed)
{
	for (int i = 0; i < 16; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code_data + i * 16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(((uint8*)rng->alg6_values_16) + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_srli_epi16(cur_val, 1);
		cur_val = _mm_or_si128(cur_val, rng_val);

		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128(cur_val, mask);

		_mm_store_si128((__m128i *)(working_code_data + i * 16), cur_val);
	}
}


void tm_128_16::alg_7()
{
	__m128i mask = _mm_set1_epi16(0x00FF);
	for (int i = 0; i < 16; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code_data + i * 16));
		cur_val = _mm_xor_si128(cur_val, mask);

		_mm_store_si128((__m128i *)(working_code_data + i * 16), cur_val);
	}
}

void tm_128_16::add_alg(uint8* addition_values, const uint16 rng_seed)
{
	for (int i = 0; i < 16; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code_data + i * 16));
		__m128i rng_val = _mm_loadu_si128((__m128i *)(addition_values + (rng_seed * 128 * 2) + (i * 16)));
		cur_val = _mm_add_epi16(cur_val, rng_val);
		__m128i mask = _mm_set1_epi16(0x00FF);
		cur_val = _mm_and_si128(cur_val, mask);

		_mm_store_si128((__m128i *)(working_code_data + i * 16), cur_val);
	}
}

void tm_128_16::run_one_map(key_schedule_entry schedule_entry)
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

void tm_128_16::run_all_maps(key_schedule_entry* schedule_entries)
{
	for (int schedule_counter = 0; schedule_counter < 27; schedule_counter++)
	{
		run_one_map(schedule_entries[schedule_counter]);
	}
}


bool tm_128_16::initialized = false;