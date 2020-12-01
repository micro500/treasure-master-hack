#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2

#include "data_sizes.h"
#include "tm_128_8.h"

tm_128_8::tm_128_8(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_128_8::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();
		rng->generate_regular_rng_values_8_lo();
		rng->generate_regular_rng_values_8_hi();

		rng->generate_alg0_values_8();
		rng->generate_alg2_values_128_8();
		rng->generate_alg4_values_8();
		rng->generate_alg4_values_8_lo();
		rng->generate_alg4_values_8_hi();
		rng->generate_alg5_values_128_8();
		rng->generate_alg6_values_8();

		initialized = true;
	}
	obj_name = "tm_128_8";
}

void tm_128_8::expand(uint32 key, uint32 data)
{
	uint8* x = (uint8*)working_code_data;
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

void tm_128_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[i] = new_data[i];
	}
}

void tm_128_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8*)working_code_data)[i];
	}
}

void tm_128_8::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
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


__forceinline void tm_128_8::alg_0(const uint16 rng_seed)
{
	__m128i mask_FE = _mm_set1_epi16(0xFEFE);
	for (int i = 0; i < 8; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i*)(working_code_data + i * 16));
		__m128i rng_val = _mm_loadu_si128((__m128i*)(rng->alg0_values_8 + (rng_seed * 128) + (i * 16)));
		cur_val = _mm_slli_epi16(cur_val, 1);
		cur_val = _mm_and_si128(cur_val, mask_FE);
		cur_val = _mm_or_si128(cur_val, rng_val);

		_mm_store_si128((__m128i*)(working_code_data + i * 16), cur_val);
	}
}

__forceinline void tm_128_8::alg_1(const uint16 rng_seed)
{
	add_alg(rng->regular_rng_values_8_lo, rng->regular_rng_values_8_hi, rng_seed);
}

__forceinline void tm_128_8::alg_2(const uint16 rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg2_values_128_8 + (rng_seed * 16)));
	for (int i = 7; i >= 0; i--)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i*)(working_code_data + i * 16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val, _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 1)), 15);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val, _mm_set_epi8(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0)), 1));

		//_mm_and_si128(_mm_slli_si128(cur_val,15),_mm_set_epi16(0,0x80,0,0x80,0,0x80,0,0x80));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val, 1),
						_mm_set_epi8(0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val, 1),
						_mm_set_epi8(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0))),
				_mm_and_si128(
					_mm_srli_si128(cur_val, 1),
					_mm_set_epi8(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80))),
			carry);

		_mm_store_si128((__m128i*)(working_code_data + i * 16), cur_val);

		carry = next_carry;
	}
}

__forceinline void tm_128_8::alg_3(const uint16 rng_seed)
{
	for (int i = 0; i < 8; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i*)(working_code_data + i * 16));
		__m128i rng_val = _mm_loadu_si128((__m128i*)(rng->regular_rng_values_8 + (rng_seed * 128) + (i * 16)));
		cur_val = _mm_xor_si128(cur_val, rng_val);

		_mm_store_si128((__m128i*)(working_code_data + i * 16), cur_val);
	}
}

__forceinline void tm_128_8::alg_4(const uint16 rng_seed)
{
	add_alg(rng->alg4_values_8_lo, rng->alg4_values_8_hi, rng_seed);
}

__forceinline void tm_128_8::alg_5(const uint16 rng_seed)
{
	__m128i carry = _mm_loadu_si128((__m128i*)(rng->alg5_values_128_8 + (rng_seed * 16)));
	for (int i = 7; i >= 0; i--)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i*)(working_code_data + i * 16));
		__m128i next_carry = _mm_slli_si128(_mm_and_si128(cur_val, _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 0x80)), 15);

		carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(cur_val, _mm_set_epi8(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0)), 1));

		cur_val = _mm_or_si128(
			_mm_or_si128(
				_mm_or_si128(
					_mm_and_si128(
						_mm_srli_epi16(cur_val, 1),
						_mm_set_epi8(0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0)),
					_mm_and_si128(
						_mm_slli_epi16(cur_val, 1),
						_mm_set_epi8(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE))),
				_mm_and_si128(
					_mm_srli_si128(cur_val, 1),
					_mm_set_epi8(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1))),
			carry);

		_mm_store_si128((__m128i*)(working_code_data + i * 16), cur_val);

		carry = next_carry;
	}
}

__forceinline void tm_128_8::alg_6(const uint16 rng_seed)
{
	__m128i mask_7F = _mm_set1_epi16(0x7F7F);
	for (int i = 0; i < 8; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i*)(working_code_data + i * 16));
		__m128i rng_val = _mm_loadu_si128((__m128i*)(rng->alg6_values_8 + (rng_seed * 128) + (i * 16)));
		cur_val = _mm_srli_epi16(cur_val, 1);
		cur_val = _mm_and_si128(cur_val, mask_7F);
		cur_val = _mm_or_si128(cur_val, rng_val);

		_mm_store_si128((__m128i*)(working_code_data + i * 16), cur_val);
	}
}

__forceinline void tm_128_8::alg_7()
{
	__m128i mask = _mm_set1_epi16(0xFFFF);
	/*for (int i = 0; i < 8; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i *)(working_code + i*16));
		cur_val = _mm_xor_si128 (cur_val, mask);

		_mm_store_si128 ((__m128i *)(working_code + i*16), cur_val);
	}
	*/

	_mm_store_si128((__m128i*)(working_code_data), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data)), mask));
	_mm_store_si128((__m128i*)(working_code_data + 16), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data + 16)), mask));
	_mm_store_si128((__m128i*)(working_code_data + 32), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data + 32)), mask));
	_mm_store_si128((__m128i*)(working_code_data + 48), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data + 48)), mask));
	_mm_store_si128((__m128i*)(working_code_data + 64), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data + 64)), mask));
	_mm_store_si128((__m128i*)(working_code_data + 80), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data + 80)), mask));
	_mm_store_si128((__m128i*)(working_code_data + 96), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data + 96)), mask));
	_mm_store_si128((__m128i*)(working_code_data + 112), _mm_xor_si128(_mm_loadu_si128((__m128i*)(working_code_data + 112)), mask));
}

__forceinline void tm_128_8::add_alg(uint8* addition_values_lo, uint8* addition_values_hi, const uint16 rng_seed)
{
	__m128i mask_lo = _mm_set_epi8(0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0);
	__m128i mask_hi = _mm_set_epi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);

	for (int i = 0; i < 8; i++)
	{
		__m128i cur_val = _mm_loadu_si128((__m128i*)(working_code_data + i * 16));
		__m128i rng_val_lo = _mm_loadu_si128((__m128i*)(addition_values_lo + (rng_seed * 128) + (i * 16)));
		__m128i rng_val_hi = _mm_loadu_si128((__m128i*)(addition_values_hi + (rng_seed * 128) + (i * 16)));
		cur_val = _mm_or_si128(_mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val, mask_lo), rng_val_lo), mask_lo), _mm_and_si128(_mm_add_epi16(_mm_and_si128(cur_val, mask_hi), rng_val_hi), mask_hi));

		_mm_store_si128((__m128i*)(working_code_data + i * 16), cur_val);
	}
}

void tm_128_8::run_one_map(key_schedule_entry schedule_entry)
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
		unsigned char current_byte = ((uint8*)working_code_data)[i];

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char alg_id = (current_byte >> 1) & 0x07;

		run_alg(alg_id, &rng_seed, 1);
	}
}

void tm_128_8::run_all_maps(key_schedule_entry* schedule_entries)
{
	for (int schedule_counter = 0; schedule_counter < 27; schedule_counter++)
	{
		run_one_map(schedule_entries[schedule_counter]);
	}
}


bool tm_128_8::initialized = false;