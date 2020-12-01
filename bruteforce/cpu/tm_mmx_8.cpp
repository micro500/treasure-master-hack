#ifndef _WIN64
#include <stdio.h>
#include <mmintrin.h>  //MMX

#include "data_sizes.h"
#include "tm_mmx_8.h"

tm_mmx_8::tm_mmx_8(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_mmx_8::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();
		rng->generate_regular_rng_values_256_8_shuffled();

		rng->generate_alg0_values_8();
		rng->generate_alg2_values_64_8();
		rng->generate_alg4_values_8();
		rng->generate_alg5_values_64_8();
		rng->generate_alg6_values_8();

		initialized = true;
	}
	obj_name = "tm_mmx_8";
}

void tm_mmx_8::expand(uint32 key, uint32 data)
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
	}
}

void tm_mmx_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint8*)working_code_data)[i] = new_data[i];
	}

}

void tm_mmx_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint8*)working_code_data)[i];
	}
}

void tm_mmx_8::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	__m64 mask_FF = _mm_set1_pi8(0xFF);
	__m64 mask_FE = _mm_set1_pi8(0xFE);
	__m64 mask_7F = _mm_set1_pi8(0x7F);
	__m64 mask_hi = _mm_set_pi8(0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0);
	__m64 mask_lo = _mm_set_pi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);
	__m64 mask_007F = _mm_set_pi8(0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F);
	__m64 mask_0080 = _mm_set_pi8(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80);
	__m64 mask_FE00 = _mm_set_pi8(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0);
	__m64 mask_0100 = _mm_set_pi8(0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0);
	__m64 mask_00FE = _mm_set_pi8(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE);
	__m64 mask_0001 = _mm_set_pi8(0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01);
	__m64 mask_7F00 = _mm_set_pi8(0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0);
	__m64 mask_8000 = _mm_set_pi8(0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0);

	__m64 mask_top_01 = _mm_set_pi8(0x01, 0, 0, 0, 0, 0, 0, 0);
	__m64 mask_top_80 = _mm_set_pi8(0x80, 0, 0, 0, 0, 0, 0, 0);

	if (algorithm_id == 0)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_0(rng_seed, mask_FE);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 1 || algorithm_id == 4)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8* rng_start = rng->regular_rng_values_8;

			if (algorithm_id == 4)
			{
				rng_start = rng->alg4_values_8;
			}

			add_alg(rng_seed, rng_start);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_2(rng_seed, mask_hi, mask_lo, mask_007F, mask_0080, mask_FE00, mask_0100, mask_top_01);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 3)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_3(rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 5)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_5(rng_seed, mask_hi, mask_lo, mask_00FE, mask_0001, mask_7F00, mask_8000, mask_top_80);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_6(rng_seed, mask_7F);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 7)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_7(mask_FF);
		}
	}
}

__forceinline void tm_mmx_8::alg_0(uint16* rng_seed, __m64& mask_FE)
{
	uint8* rng_start = rng->alg0_values_8 + ((*rng_seed) * 128);

	for (int i = 0; i < 16; i++)
	{
		__m64 cur_val = *(__m64*)(working_code_data + (i * 8));
		__m64 rng_val = *(__m64*)(rng_start + (i * 8));
		cur_val = _m_psllwi(cur_val, 1);
		cur_val = _m_pand(cur_val, mask_FE);
		cur_val = _m_por(cur_val, rng_val);

		*(__m64*)(working_code_data + (i * 8)) = cur_val;
	}
}


__forceinline void tm_mmx_8::alg_2(uint16* rng_seed, __m64& mask_hi, __m64& mask_lo, __m64& mask_007F, __m64& mask_0080, __m64& mask_FE00, __m64& mask_0100, __m64& mask_top_01)
{
	__m64 carry = *(__m64*)(rng->alg2_values_64_8 + *rng_seed * 8);
	for (int i = 15; i >= 0; i--)
	{
		__m64 cur_val = *(__m64*)(working_code_data + (i * 8));

		__m64 cur_val_lo = _m_pand(cur_val, mask_lo);
		__m64 temp1 = _m_psrlwi(cur_val_lo, 1);
		__m64 temp2 = _m_psrlqi(cur_val, 8);
		temp1 = _m_pand(temp1, mask_007F);
		temp1 = _m_por(temp1, _m_pand(temp2, mask_0080));

		__m64 cur_val_hi = _m_pand(cur_val, mask_hi);
		__m64 temp3 = _m_psllwi(cur_val_hi, 1);
		__m64 temp4 = _m_psrlqi(cur_val, 8);
		temp3 = _m_pand(temp3, mask_FE00);
		temp3 = _m_por(temp3, _m_pand(temp4, mask_0100));
		temp3 = _m_por(temp3, carry);

		__m64 next_carry = _m_pand(_m_psllqi(cur_val, 56), mask_top_01);

		*(__m64*)(working_code_data + (i * 8)) = _m_por(temp1, temp3);

		carry = next_carry;
	}
}


__forceinline void tm_mmx_8::alg_3(uint16* rng_seed)
{
	uint8* rng_start = rng->regular_rng_values_8 + ((*rng_seed) * 128);

	for (int i = 0; i < 16; i++)
	{
		*(__m64*)(working_code_data + (i * 8)) = _m_pxor(*(__m64*)(working_code_data + (i * 8)), *(__m64*)(rng_start + (i * 8)));
	}
}

/*
__forceinline void tm_mmx_8::alg_5_sub(__m256i& working_a, __m256i& working_b, __m256i& carry, __m256i& mask_top_80, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m128i cur_val1_lo = _mm256_castsi256_si128(working_a);
	__m128i cur_val1_hi = _mm256_extractf128_si256(working_a, 1);
	__m128i cur_val2_lo = _mm256_castsi256_si128(working_b);
	__m128i cur_val2_hi = _mm256_extractf128_si256(working_b, 1);

	// bitwise left shift
	__m128i temp1_lo = _mm_slli_epi16(cur_val1_lo, 1);
	__m128i temp1_hi = _mm_slli_epi16(cur_val1_hi, 1);
	// Mask off low bits
	__m256i cur_val1_most = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(temp1_hi, temp1_lo)), _mm256_castsi256_pd(mask_FE)));

	// Mask off the low bits
	__m256i cur_val2_masked = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(working_b), _mm256_castsi256_pd(mask_01)));

	// bytewise right shift
	temp1_lo = _mm_srli_si128(cur_val1_lo, 1);
	// carry the lowest byte from the high half into the lowest half
	temp1_lo = _mm_or_si128(temp1_lo, _mm_slli_si128(cur_val1_hi, 15));
	temp1_hi = _mm_srli_si128(cur_val1_hi, 1);
	__m256i cur_val1_srl = _mm256_set_m128i(temp1_hi, temp1_lo);
	// mask off only the relevant high bit
	__m256i cur_val1_bit = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val1_srl), _mm256_castsi256_pd(mask_80)));
	// add the carry to the top
	cur_val1_bit = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_bit), _mm256_castsi256_pd(carry)));

	// bitwise right shift
	__m128i temp2_lo = _mm_srli_epi16(cur_val2_lo, 1);
	__m128i temp2_hi = _mm_srli_epi16(cur_val2_hi, 1);
	// mask off high bit
	__m256i cur_val2_most = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(temp2_hi, temp2_lo)), _mm256_castsi256_pd(mask_7F)));

	// Save the next carry
	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val1_lo, 15), cur_val1_lo)), _mm256_castsi256_pd(mask_top_80)));

	working_a = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val1_most), _mm256_castsi256_pd(cur_val2_masked)));
	working_b = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val2_most), _mm256_castsi256_pd(cur_val1_bit)));

	carry = next_carry;
}

__forceinline void tm_mmx_8::alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_alg2, __m256i& mask_007F, __m256i& mask_FE00, __m256i& mask_0080, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg5_values_256_8 + ((*rng_seed) * 32)));

	alg_5_sub(working_code2, working_code3, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
	alg_5_sub(working_code0, working_code1, carry, mask_top_80, mask_80, mask_7F, mask_FE, mask_01);
}
*/

__forceinline void tm_mmx_8::alg_5(uint16* rng_seed, __m64& mask_hi, __m64& mask_lo, __m64& mask_00FE, __m64& mask_0001, __m64& mask_7F00, __m64& mask_8000, __m64& mask_top_80)
{
	__m64 carry = *(__m64*)(rng->alg5_values_64_8 + *rng_seed * 8);
	for (int i = 15; i >= 0; i--)
	{
		__m64 cur_val = *(__m64*)(working_code_data + (i * 8));

		__m64 cur_val_lo = _m_pand(cur_val, mask_lo);
		__m64 temp1 = _m_psllwi(cur_val_lo, 1);
		__m64 temp2 = _m_psrlqi(cur_val, 8);
		temp1 = _m_pand(temp1, mask_00FE);
		temp1 = _m_por(temp1, _m_pand(temp2, mask_0001));

		__m64 cur_val_hi = _m_pand(cur_val, mask_hi);
		__m64 temp3 = _m_psrlwi(cur_val_hi, 1);
		__m64 temp4 = _m_psrlqi(cur_val, 8);
		temp3 = _m_pand(temp3, mask_7F00);
		temp3 = _m_por(temp3, _m_pand(temp4, mask_8000));
		temp3 = _m_por(temp3, carry);

		__m64 next_carry = _m_pand(_m_psllqi(cur_val, 56), mask_top_80);

		*(__m64*)(working_code_data + (i * 8)) = _m_por(temp1, temp3);

		carry = next_carry;
	}
}


__forceinline void tm_mmx_8::alg_6(uint16* rng_seed, __m64& mask_7F)
{
	uint8* rng_start = rng->alg6_values_8 + ((*rng_seed) * 128);

	for (int i = 0; i < 16; i++)
	{
		__m64 cur_val = *(__m64*)(working_code_data + (i * 8));
		__m64 rng_val = *(__m64*)(rng_start + (i * 8));
		cur_val = _m_psrlwi(cur_val, 1);
		cur_val = _m_pand(cur_val, mask_7F);
		cur_val = _m_por(cur_val, rng_val);

		*(__m64*)(working_code_data + (i * 8)) = cur_val;
	}
}

__forceinline void tm_mmx_8::alg_7(__m64& mask_FF)
{
	for (int i = 0; i < 16; i++)
	{
		*(__m64*)(working_code_data + (i * 8)) = _m_pxor(*(__m64*)(working_code_data + (i * 8)), mask_FF);
	}
}

__forceinline void tm_mmx_8::add_alg(uint16* rng_seed, uint8* rng_start)
{
	rng_start = rng_start + ((*rng_seed) * 128);

	for (int i = 0; i < 16; i++)
	{
		*(__m64*)(working_code_data + (i * 8)) = _m_paddb(*(__m64*)(working_code_data + (i * 8)), *(__m64*)(rng_start + (i * 8)));
	}
}


void tm_mmx_8::run_one_map(key_schedule_entry schedule_entry)
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

void tm_mmx_8::run_all_maps(key_schedule_entry* schedule_entries)
{
	__m64 mask_FF = _mm_set1_pi8(0xFF);
	__m64 mask_FE = _mm_set1_pi8(0xFE);
	__m64 mask_7F = _mm_set1_pi8(0x7F);
	__m64 mask_hi = _mm_set_pi8(0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0);
	__m64 mask_lo = _mm_set_pi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);
	__m64 mask_007F = _mm_set_pi8(0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F);
	__m64 mask_0080 = _mm_set_pi8(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80);
	__m64 mask_FE00 = _mm_set_pi8(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0);
	__m64 mask_0100 = _mm_set_pi8(0x01, 0, 0x01, 0, 0x01, 0, 0x01, 0);
	__m64 mask_00FE = _mm_set_pi8(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE);
	__m64 mask_0001 = _mm_set_pi8(0, 0x01, 0, 0x01, 0, 0x01, 0, 0x01);
	__m64 mask_7F00 = _mm_set_pi8(0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0);
	__m64 mask_8000 = _mm_set_pi8(0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0);

	__m64 mask_top_01 = _mm_set_pi8(0x01, 0, 0, 0, 0, 0, 0, 0);
	__m64 mask_top_80 = _mm_set_pi8(0x80, 0, 0, 0, 0, 0, 0, 0);

	for (int schedule_counter = 0; schedule_counter < 27; schedule_counter++)
	{
		key_schedule_entry schedule_entry = schedule_entries[schedule_counter];

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
			unsigned char current_byte = (uint8)((uint8*)working_code_data)[i];

			if (nibble == 1)
			{
				current_byte = current_byte >> 4;
			}

			// Mask off only 3 bits
			unsigned char algorithm_id = (current_byte >> 1) & 0x07;
			/*
			printf("%i ", algorithm_id);
			printf("%04X ", rng_seed);

			// store back to memory
			_mm256_store_si256((__m256i*)(working_code_data), working_code0);
			_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
			_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
			_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);

			print_working_code();
			*/

			if (algorithm_id == 0)
			{
				alg_0(&rng_seed, mask_FE);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 1 || algorithm_id == 4)
			{
				uint8* rng_start = rng->regular_rng_values_8;

				if (algorithm_id == 4)
				{
					rng_start = rng->alg4_values_8;
				}

				add_alg(&rng_seed, rng_start);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 2)
			{
				alg_2(&rng_seed, mask_hi, mask_lo, mask_007F, mask_0080, mask_FE00, mask_0100, mask_top_01);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 3)
			{
				alg_3(&rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 5)
			{
				alg_5(&rng_seed, mask_hi, mask_lo, mask_00FE, mask_0001, mask_7F00, mask_8000, mask_top_80);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 6)
			{
				alg_6(&rng_seed, mask_7F);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 7)
			{
				alg_7(mask_FF);
			}
		}
		/*
		printf("\n");
		if (schedule_counter == 6)
		{
			// store back to memory
			_mm256_store_si256((__m256i*)(working_code_data), working_code0);
			_mm256_store_si256((__m256i*)(working_code_data + 32), working_code1);
			_mm256_store_si256((__m256i*)(working_code_data + 64), working_code2);
			_mm256_store_si256((__m256i*)(working_code_data + 96), working_code3);

			print_working_code();
		}
		*/
	}
}

bool tm_mmx_8::initialized = false;
#endif