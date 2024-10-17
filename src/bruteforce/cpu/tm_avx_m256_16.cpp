#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A
#include <immintrin.h> //AVX
//#include <zmmintrin.h> //AVX512

#include "data_sizes.h"
#include "tm_avx_m256_16.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx_m256_16::tm_avx_m256_16(RNG* rng_obj) : TM_base(rng_obj)
{
	initialize();
}

__forceinline void tm_avx_m256_16::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_16();

		rng->generate_alg0_values_16();
		rng->generate_alg2_values_256_16();
		rng->generate_alg4_values_16();
		rng->generate_alg5_values_256_16();
		rng->generate_alg6_values_16();

		initialized = true;
	}
	obj_name = "tm_avx_m256_16";
}

void tm_avx_m256_16::expand(uint32 key, uint32 data)
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


void tm_avx_m256_16::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		((uint16*)working_code_data)[i] = new_data[i];
	}
}

void tm_avx_m256_16::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = ((uint16*)working_code_data)[i] & 0xFF;
	}
}

void tm_avx_m256_16::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	if (algorithm_id == 0)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_0(rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 1)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_1(rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_2(rng_seed);
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
	else if (algorithm_id == 4)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_4(rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 5)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_5(rng_seed);
			*rng_seed = rng->seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm_id == 6)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_6(rng_seed);
			*rng_seed = rng->seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm_id == 7)
	{
		for (int j = 0; j < iterations; j++)
		{
			alg_7();
		}
	}
}

__forceinline void tm_avx_m256_16::alg_0(uint16* rng_seed)
{
	for (int i = 0; i < 8; i++)
	{
		__m128i cur_val_lo = _mm_loadu_si128((__m128i*)(working_code_data + i * 32));
		cur_val_lo = _mm_slli_epi16(cur_val_lo, 1);

		__m128i cur_val_hi = _mm_loadu_si128((__m128i*)(working_code_data + i * 32 + 16));
		cur_val_hi = _mm_slli_epi16(cur_val_hi, 1);

		__m256i cur_val = _mm256_set_m128i(cur_val_hi, cur_val_lo);

		__m256i rng_val = _mm256_load_si256((__m256i*)((uint8*)rng->alg0_values_16 + (*rng_seed * 128 * 2) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));

		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_16::alg_1(uint16* rng_seed)
{
	add_alg((uint8*)rng->regular_rng_values_16, rng_seed);
}

__forceinline void tm_avx_m256_16::alg_2(uint16* rng_seed)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg2_values_256_16 + (*rng_seed * 32)));
	for (int i = 7; i >= 0; i--)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		__m128i cur_val_hi = _mm256_castsi256_si128(cur_val);

		// Maybe 128-bit mask it, then reinterpret-cast it to 256 and permute it?
		__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(_mm256_set_epi16(0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))));

		// Fetch extra carry and apply it to carry
		__m128i temp_lo = _mm_srli_si128(cur_val_lo, 2);
		temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
		__m128i temp_hi = _mm_srli_si128(cur_val_hi, 2);
		__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0))));
		carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));


		temp_lo = _mm_srli_epi16(cur_val_lo, 1);
		temp_hi = _mm_srli_epi16(cur_val_hi, 1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F))));

		temp_lo = _mm_slli_epi16(cur_val_lo, 1);
		temp_hi = _mm_slli_epi16(cur_val_hi, 1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0))));

		__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80))));

		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(part3)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(carry)));

		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);

		carry = next_carry;
	}
}

__forceinline void tm_avx_m256_16::alg_3(uint16* rng_seed)
{
	uint8* rng_start = (uint8*)rng->regular_rng_values_16 + (*rng_seed * 128 * 2);
	for (int i = 0; i < 8; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_load_si256((__m256i*)(rng_start + (i * 32)));
		cur_val = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_16::alg_4(uint16* rng_seed)
{
	add_alg((uint8*)rng->alg4_values_16, rng_seed);
}

__forceinline void tm_avx_m256_16::alg_5(uint16* rng_seed)
{
	__m256i carry = _mm256_load_si256((__m256i*)(rng->alg5_values_256_16 + (*rng_seed * 32)));
	for (int i = 7; i >= 0; i--)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m128i cur_val_lo = _mm256_castsi256_si128(cur_val);
		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		__m128i cur_val_hi = _mm256_castsi256_si128(cur_val);

		// Maybe 128-bit mask it, then reinterpret-cast it to 256 and permute it?
		__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 14), cur_val_lo)), _mm256_castsi256_pd(_mm256_set_epi16(0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))));

		// Fetch extra carry and apply it to carry
		__m128i temp_lo = _mm_srli_si128(cur_val_lo, 2);
		temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 14));
		__m128i temp_hi = _mm_srli_si128(cur_val_hi, 2);
		__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0))));
		carry = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));


		temp_lo = _mm_srli_epi16(cur_val_lo, 1);
		temp_hi = _mm_srli_epi16(cur_val_hi, 1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0))));

		temp_lo = _mm_slli_epi16(cur_val_lo, 1);
		temp_hi = _mm_slli_epi16(cur_val_hi, 1);
		temp256 = _mm256_set_m128i(temp_hi, temp_lo);
		__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(_mm256_set_epi16(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE))));

		__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(_mm256_set_epi16(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1))));

		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(part3)));
		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(carry)));

		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);

		carry = next_carry;
	}
}

__forceinline void tm_avx_m256_16::alg_6(uint16* rng_seed)
{
	for (int i = 0; i < 8; i++)
	{
		__m128i cur_val_lo = _mm_loadu_si128((__m128i*)(working_code_data + i * 32));
		cur_val_lo = _mm_srli_epi16(cur_val_lo, 1);

		__m128i cur_val_hi = _mm_loadu_si128((__m128i*)(working_code_data + i * 32 + 16));
		cur_val_hi = _mm_srli_epi16(cur_val_hi, 1);

		__m256i cur_val = _mm256_set_m128i(cur_val_hi, cur_val_lo);

		__m256i rng_val = _mm256_load_si256((__m256i*)((uint8*)rng->alg6_values_16 + (*rng_seed * 128 * 2) + (i * 32)));

		cur_val = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(rng_val)));

		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));

		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_16::alg_7()
{
	for (int i = 0; i < 8; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));
		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}

__forceinline void tm_avx_m256_16::add_alg(const uint8* addition_values, uint16* rng_seed)
{
	for (int i = 0; i < 8; i++)
	{
		__m256i cur_val = _mm256_load_si256((__m256i*)(working_code_data + i * 32));
		__m256i rng_val = _mm256_load_si256((__m256i*)(addition_values + (*rng_seed * 128 * 2) + (i * 32)));

		__m128i sum_lo = _mm_add_epi16(_mm256_castsi256_si128(cur_val), _mm256_castsi256_si128(rng_val));

		cur_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(cur_val), 1));
		rng_val = _mm256_castpd_si256(_mm256_permute2f128_pd(_mm256_castsi256_pd(rng_val), _mm256_castsi256_pd(rng_val), 1));

		__m128i sum_hi = _mm_add_epi16(_mm256_castsi256_si128(cur_val), _mm256_castsi256_si128(rng_val));

		cur_val = _mm256_set_m128i(sum_hi, sum_lo);

		__m256i mask = _mm256_set1_epi16(0x00FF);
		cur_val = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(cur_val), _mm256_castsi256_pd(mask)));

		_mm256_store_si256((__m256i*)(working_code_data + i * 32), cur_val);
	}
}


void tm_avx_m256_16::run_one_map(const key_schedule::key_schedule_entry& schedule_entry)
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

void tm_avx_m256_16::run_all_maps(const key_schedule& schedule_entries)
{
	for (std::vector<key_schedule::key_schedule_entry>::const_iterator it = schedule_entries.entries.begin(); it != schedule_entries.entries.end(); it++)
	{
		key_schedule::key_schedule_entry schedule_entry = *it;

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
			unsigned char algorithm_id = (current_byte >> 1) & 0x07;

			if (algorithm_id == 0)
			{
				alg_0(&rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 1)
			{
				alg_1(&rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 2)
			{
				alg_2(&rng_seed);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 3)
			{
				alg_3(&rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 4)
			{
				alg_4(&rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 5)
			{
				alg_5(&rng_seed);
				rng_seed = rng->seed_forward_1[rng_seed];
			}
			else if (algorithm_id == 6)
			{
				alg_6(&rng_seed);
				rng_seed = rng->seed_forward_128[rng_seed];
			}
			else if (algorithm_id == 7)
			{
				alg_7();
			}
		}
	}
}

bool tm_avx_m256_16::initialized = false;