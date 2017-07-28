#include <stdio.h>
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

#include <chrono>
#include <iostream>

#include "alignment.h"
#include "tm_avx_full.h"
#include "key_schedule.h"
#include "data_sizes.h"
#include "rng.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

void main()
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;

	uint16 rng_seed;
	uint16 * rng_table = new uint16[256*256];
	generate_rng_table(rng_table);

	uint8 * regular_rng_values = (uint8*)aligned_malloc(0x10000 * 128 * 2, 32);
	generate_regular_rng_values_16((uint16*)regular_rng_values, rng_table);

	uint8 * alg0_values = (uint8*)aligned_malloc(0x10000 * 128 * 2, 32);
	generate_alg0_values_16((uint16*)alg0_values, rng_table);

	uint8 * alg6_values = (uint8*)aligned_malloc(0x10000 * 128 * 2, 32);
	generate_alg6_values_16((uint16*)alg6_values, rng_table);

	uint8 *alg2_values = (uint8*)aligned_malloc(0x10000 * 32, 32);
	generate_alg2_values_256_16(alg2_values, rng_table);

	uint8 *alg5_values = (uint8*)aligned_malloc(0x10000 * 32, 32);
	generate_alg5_values_256_16(alg5_values, rng_table);

	uint16 * rng_forward_1 = new uint16[256*256];
	generate_seed_forward_1(rng_forward_1, rng_table);

	uint16 * rng_forward_128 = new uint16[256*256];
	generate_seed_forward_128(rng_forward_128, rng_table);

	int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11};

	uint8 IV[4] = {0x2C,0xA5,0xB4,0x2D};

	ALIGNED(32) uint16 working_code[128];

	key_schedule_data schedule_data;
	schedule_data.as_uint8[0] = IV[0];
	schedule_data.as_uint8[1] = IV[1];
	schedule_data.as_uint8[2] = IV[2];
	schedule_data.as_uint8[3] = IV[3];

	key_schedule_entry schedule_entries[27];

	int schedule_counter = 0;
	for (int i = 0; i < 26; i++)
	{
		schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data);

		if (map_list[i] == 0x22)
		{
			schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data,4);
		}
	}

	__m256i mask_FF = _mm256_set1_epi16(0x00FF);
	__m256i mask_01 = _mm256_set1_epi16(0x0001);
	__m256i mask_top_01 = _mm256_set_epi16(0x01,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
	__m256i mask_alg2 = _mm256_set_epi16(0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0);
	__m256i mask_007F = _mm256_set_epi16(0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F);
	__m256i mask_FE00 = _mm256_set_epi16(0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0);
	__m256i mask_0080 = _mm256_set_epi16(0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80);

	__m256i mask_top_80 = _mm256_set_epi16(0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
	__m256i mask_alg5 = _mm256_set_epi16(0,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0,0x80,0);
	__m256i mask_7F00 = _mm256_set_epi16(0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0,0x7F,0);
	__m256i mask_00FE = _mm256_set_epi16(0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE,0,0xFE);
	__m256i mask_0001 = _mm256_set_epi16(0,0x01,0,0x01,0,0x01,0,0x01,0,0x01,0,0x01,0,0x01,0,0x01);

	auto start = clock::now();
	for (int byte5 = 0; byte5 < 256; byte5++)
	{
		for (int byte6 = 0; byte6 < 256; byte6++)
		{
			for (int byte7 = 0; byte7 < 256; byte7++)
			{
				working_code[0] = IV[0];
				working_code[1] = IV[1];
				working_code[2] = IV[2];
				working_code[3] = IV[3];
				working_code[4] = 0;
				working_code[5] = byte5;
				working_code[6] = byte6;
				working_code[7] = byte7;

				rng_seed = (working_code[0] << 8) | working_code[1];

				// Expand the code to 128 bytes
				for (int i = 8; i < 0x80; i++)
				{
					working_code[i] = (working_code[i-8] + run_rng(&rng_seed,rng_table)) & 0xFF;
				}
	
				/*
				for (int i = 127; i >= 0; i--)
				{

					printf("%02X",working_code[i]);
				}
				printf("\n\n");
				*/

				__m256i working_code0 = _mm256_load_si256((__m256i *)(((uint8*)working_code)));
				__m256i working_code1 = _mm256_load_si256((__m256i *)(((uint8*)working_code) + 32));
				__m256i working_code2 = _mm256_load_si256((__m256i *)(((uint8*)working_code) + 64));
				__m256i working_code3 = _mm256_load_si256((__m256i *)(((uint8*)working_code) + 96));
				__m256i working_code4 = _mm256_load_si256((__m256i *)(((uint8*)working_code) + 128));
				__m256i working_code5 = _mm256_load_si256((__m256i *)(((uint8*)working_code) + 160));
				__m256i working_code6 = _mm256_load_si256((__m256i *)(((uint8*)working_code) + 192));
				__m256i working_code7 = _mm256_load_si256((__m256i *)(((uint8*)working_code) + 224));

				for (schedule_counter = 0; schedule_counter < 27; schedule_counter++)
				{
					key_schedule_entry schedule_entry = schedule_entries[schedule_counter];

					rng_seed = (schedule_entry.rng1 << 8) | schedule_entry.rng2;
					uint16 nibble_selector = schedule_entry.nibble_selector;

					// Next, the working code is processed with the same steps 16 times:
					for (int i = 0; i < 16; i++)
					{
						_mm256_store_si256 ((__m256i *)(((uint8*)working_code)), working_code0);
			
						// Get the highest bit of the nibble selector to use as a flag
						unsigned char nibble = (nibble_selector >> 15) & 0x01;
						// Shift the nibble selector up one bit
						nibble_selector = nibble_selector << 1;

						// If the flag is a 1, get the high nibble of the current byte
						// Otherwise use the low nibble
						unsigned char current_byte = working_code[i];
			
						if (nibble == 1)
						{
							current_byte = current_byte >> 4;
						}

						// Mask off only 3 bits
						unsigned char alg_id = (current_byte >> 1) & 0x07;
		
						// Run the selected algorithm
						if (alg_id == 0)
						{
							uint8 * rng_start = alg0_values + ((rng_seed) * 128 * 2);

							__m128i cur_val_lo = _mm256_castsi256_si128(working_code0);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							__m128i cur_val_hi = _mm256_extractf128_si256(working_code0,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code0 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));
							working_code0 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code1);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code1,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code1 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));
							working_code1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code2);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code2,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code2 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));
							working_code2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code3);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code3,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code3 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
							working_code3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code4);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code4,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code4 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(rng_val)));
							working_code4 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code5);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code5,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code5 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(rng_val)));
							working_code5 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code6);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code6,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code6 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(rng_val)));
							working_code6 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code7);
							cur_val_lo = _mm_slli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code7,1);
							cur_val_hi = _mm_slli_epi16(cur_val_hi,1);
							working_code7 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(rng_val)));
							working_code7 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));

							rng_seed = rng_forward_128[rng_seed];
						}
						else if (alg_id == 1)
						{
							uint8 * rng_start = regular_rng_values + ((rng_seed) * 128 * 2);

							__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
							__m128i sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code0), _mm256_castsi256_si128(rng_val));
							__m128i sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code0,1), _mm256_extractf128_si256(rng_val,1));
							working_code0 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code0 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
							sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code1), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code1,1), _mm256_extractf128_si256(rng_val,1));
							working_code1 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
							sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code2), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code2,1), _mm256_extractf128_si256(rng_val,1));
							working_code2 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
							sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code3), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code3,1), _mm256_extractf128_si256(rng_val,1));
							working_code3 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
							sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code4), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code4,1), _mm256_extractf128_si256(rng_val,1));
							working_code4 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code4 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
							sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code5), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code5,1), _mm256_extractf128_si256(rng_val,1));
							working_code5 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code5 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
							sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code6), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code6,1), _mm256_extractf128_si256(rng_val,1));
							working_code6 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code6 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
							sum_lo = _mm_add_epi16(_mm256_castsi256_si128(working_code7), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_add_epi16(_mm256_extractf128_si256(working_code7,1), _mm256_extractf128_si256(rng_val,1));
							working_code7 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code7 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));

							rng_seed = rng_forward_128[rng_seed];
						}
						else if (alg_id == 2)
						{
							__m256i carry = _mm256_load_si256((__m256i *)(alg2_values + ((rng_seed) * 32)));

							__m128i cur_val_lo = _mm256_castsi256_si128(working_code7);
							__m128i cur_val_hi = _mm256_extractf128_si256(working_code7,1);

							__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));

							__m128i temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							__m128i temp_hi = _mm_srli_si128(cur_val_hi,2);
							__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

							__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							__m256i part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));

							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							__m256i part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));

							__m256i part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));

							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code7),_mm256_castsi256_pd(part3)));
							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code7),_mm256_castsi256_pd(carry)));

							carry = next_carry;



							cur_val_lo = _mm256_castsi256_si128(working_code6);
							cur_val_hi = _mm256_extractf128_si256(working_code6,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code6),_mm256_castsi256_pd(part3)));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code6),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code5);
							cur_val_hi = _mm256_extractf128_si256(working_code5,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code5),_mm256_castsi256_pd(part3)));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code5),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code4);
							cur_val_hi = _mm256_extractf128_si256(working_code4,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code4),_mm256_castsi256_pd(part3)));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code4),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code3);
							cur_val_hi = _mm256_extractf128_si256(working_code3,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code3),_mm256_castsi256_pd(part3)));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code3),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code2);
							cur_val_hi = _mm256_extractf128_si256(working_code2,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code2),_mm256_castsi256_pd(part3)));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code2),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code1);
							cur_val_hi = _mm256_extractf128_si256(working_code1,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_01)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code1),_mm256_castsi256_pd(part3)));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code1),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code0);
							cur_val_hi = _mm256_extractf128_si256(working_code0,1);
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code0),_mm256_castsi256_pd(part3)));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code0),_mm256_castsi256_pd(carry)));

							rng_seed = rng_forward_1[rng_seed];
						}
						else if (alg_id == 3)
						{
							uint8 * rng_start = regular_rng_values + ((rng_seed) * 128 * 2);

							__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
							working_code0 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
							working_code1 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
							working_code2 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
							working_code3 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
							working_code4 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(rng_val)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
							working_code5 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(rng_val)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
							working_code6 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(rng_val)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
							working_code7 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(rng_val)));

							rng_seed = rng_forward_128[rng_seed];
						}
						else if (alg_id == 4)
						{
							uint8 * rng_start = regular_rng_values + ((rng_seed) * 128 * 2);

							__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
							__m128i sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code0), _mm256_castsi256_si128(rng_val));
							__m128i sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code0,1), _mm256_extractf128_si256(rng_val,1));
							working_code0 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code0 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
							sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code1), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code1,1), _mm256_extractf128_si256(rng_val,1));
							working_code1 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
							sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code2), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code2,1), _mm256_extractf128_si256(rng_val,1));
							working_code2 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
							sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code3), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code3,1), _mm256_extractf128_si256(rng_val,1));
							working_code3 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
							sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code4), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code4,1), _mm256_extractf128_si256(rng_val,1));
							working_code4 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code4 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
							sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code5), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code5,1), _mm256_extractf128_si256(rng_val,1));
							working_code5 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code5 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
							sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code6), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code6,1), _mm256_extractf128_si256(rng_val,1));
							working_code6 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code6 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

							rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
							sum_lo = _mm_sub_epi16(_mm256_castsi256_si128(working_code7), _mm256_castsi256_si128(rng_val));
							sum_hi = _mm_sub_epi16(_mm256_extractf128_si256(working_code7,1), _mm256_extractf128_si256(rng_val,1));
							working_code7 = _mm256_set_m128i(sum_hi, sum_lo);
							working_code7 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));

							rng_seed = rng_forward_128[rng_seed];
						}
						else if (alg_id == 5)
						{
							__m256i carry = _mm256_load_si256((__m256i *)(alg5_values + ((rng_seed) * 32)));

							__m128i cur_val_lo = _mm256_castsi256_si128(working_code7);
							__m128i cur_val_hi = _mm256_extractf128_si256(working_code7,1);

							__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));

							__m128i temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							__m128i temp_hi = _mm_srli_si128(cur_val_hi,2);
							__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

							__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							__m256i part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));

							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							__m256i part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));

							__m256i part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));

							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code7),_mm256_castsi256_pd(part3)));
							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code7),_mm256_castsi256_pd(carry)));

							carry = next_carry;



							cur_val_lo = _mm256_castsi256_si128(working_code6);
							cur_val_hi = _mm256_extractf128_si256(working_code6,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code6),_mm256_castsi256_pd(part3)));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code6),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code5);
							cur_val_hi = _mm256_extractf128_si256(working_code5,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code5),_mm256_castsi256_pd(part3)));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code5),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code4);
							cur_val_hi = _mm256_extractf128_si256(working_code4,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code4),_mm256_castsi256_pd(part3)));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code4),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code3);
							cur_val_hi = _mm256_extractf128_si256(working_code3,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code3),_mm256_castsi256_pd(part3)));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code3),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code2);
							cur_val_hi = _mm256_extractf128_si256(working_code2,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code2),_mm256_castsi256_pd(part3)));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code2),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code1);
							cur_val_hi = _mm256_extractf128_si256(working_code1,1);
							next_carry = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo)), _mm256_castsi256_pd(mask_top_80)));
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code1),_mm256_castsi256_pd(part3)));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code1),_mm256_castsi256_pd(carry)));
							carry = next_carry;

							cur_val_lo = _mm256_castsi256_si128(working_code0);
							cur_val_hi = _mm256_extractf128_si256(working_code0,1);
							temp_lo = _mm_srli_si128(cur_val_lo,2);
							temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi,14));
							temp_hi = _mm_srli_si128(cur_val_hi,2);
							cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
							temp256 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
							carry = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(carry),_mm256_castsi256_pd(temp256)));
							temp_lo = _mm_srli_epi16(cur_val_lo,1);
							temp_hi = _mm_srli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));
							temp_lo = _mm_slli_epi16(cur_val_lo,1);
							temp_hi = _mm_slli_epi16(cur_val_hi,1);
							temp256 = _mm256_set_m128i(temp_hi, temp_lo);
							part2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));
							part3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(part1),_mm256_castsi256_pd(part2)));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code0),_mm256_castsi256_pd(part3)));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code0),_mm256_castsi256_pd(carry)));

							rng_seed = rng_forward_1[rng_seed];
						}
						else if (alg_id == 6)
						{
							uint8 * rng_start = alg6_values + ((rng_seed) * 128 * 2);

							__m128i cur_val_lo = _mm256_castsi256_si128(working_code0);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							__m128i cur_val_hi = _mm256_extractf128_si256(working_code0,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code0 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
							working_code0 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(rng_val)));
							working_code0 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code1);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code1,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code1 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
							working_code1 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(rng_val)));
							working_code1 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code2);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code2,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code2 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
							working_code2 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(rng_val)));
							working_code2 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code3);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code3,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code3 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
							working_code3 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(rng_val)));
							working_code3 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code4);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code4,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code4 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
							working_code4 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(rng_val)));
							working_code4 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code5);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code5,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code5 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
							working_code5 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(rng_val)));
							working_code5 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code6);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code6,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code6 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
							working_code6 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(rng_val)));
							working_code6 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));

							cur_val_lo = _mm256_castsi256_si128(working_code7);
							cur_val_lo = _mm_srli_epi16(cur_val_lo,1);
							cur_val_hi = _mm256_extractf128_si256(working_code7,1);
							cur_val_hi = _mm_srli_epi16(cur_val_hi,1);
							working_code7 = _mm256_set_m128i(cur_val_hi, cur_val_lo);
							rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
							working_code7 = _mm256_castpd_si256(_mm256_or_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(rng_val)));
							working_code7 = _mm256_castpd_si256(_mm256_and_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));

							rng_seed = rng_forward_128[rng_seed];
						}
						else if (alg_id == 7)
						{
							working_code0 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code0), _mm256_castsi256_pd(mask_FF)));
							working_code1 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code1), _mm256_castsi256_pd(mask_FF)));
							working_code2 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code2), _mm256_castsi256_pd(mask_FF)));
							working_code3 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code3), _mm256_castsi256_pd(mask_FF)));
							working_code4 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code4), _mm256_castsi256_pd(mask_FF)));
							working_code5 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code5), _mm256_castsi256_pd(mask_FF)));
							working_code6 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code6), _mm256_castsi256_pd(mask_FF)));
							working_code7 = _mm256_castpd_si256(_mm256_xor_pd (_mm256_castsi256_pd(working_code7), _mm256_castsi256_pd(mask_FF)));
						} 
					}
				}

				_mm256_store_si256 ((__m256i *)(((uint8*)working_code)), working_code0);
				_mm256_store_si256 ((__m256i *)(((uint8*)working_code) + 32), working_code1);
				_mm256_store_si256 ((__m256i *)(((uint8*)working_code) + 64), working_code2);
				_mm256_store_si256 ((__m256i *)(((uint8*)working_code) + 96), working_code3);
				_mm256_store_si256 ((__m256i *)(((uint8*)working_code) + 128), working_code4);
				_mm256_store_si256 ((__m256i *)(((uint8*)working_code) + 160), working_code5);
				_mm256_store_si256 ((__m256i *)(((uint8*)working_code) + 192), working_code6);
				_mm256_store_si256 ((__m256i *)(((uint8*)working_code) + 224), working_code7);

				// fake sum
				uint16 sum = 0;
				for (int i = 0; i < 128; i++)
				{
					sum += working_code[i];
				}
	
				if (sum == 0x41D7)
				{
					printf("got it, %i %i %i\n", byte5, byte6, byte7);
				}
			}
		}
	}
	auto end = clock::now();
	std::cout << duration_cast<microseconds>(end-start).count() << "us\n";

}