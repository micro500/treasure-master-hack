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

#include "../tm_8bit/data_sizes.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

void generate_rng_table(uint16 * rng_table)
{
    for (int i = 0; i <= 0xFF; i++)
    {
        for (int j = 0; j <= 0xFF; j++)
        {
            unsigned int rngA = i;
            unsigned int rngB = j;
            
            uint8 carry = 0;
            
            rngB = (rngB + rngA) & 0xFF;
            
            rngA = rngA + 0x89;
            carry = rngA > 0xFF ? 1 : 0;
            rngA = rngA & 0xFF;
            
            rngB = rngB + 0x2A + carry;
            carry = rngB > 0xFF ? 1 : 0;
            rngB = rngB & 0xFF;
            
            rngA = rngA + 0x21 + carry;
            carry = rngA > 0xFF ? 1 : 0;
            rngA = rngA & 0xFF;
            
            rngB = rngB + 0x43 + carry;
            carry = rngB > 0xFF ? 1 : 0;
            rngB = rngB & 0xFF;
            
            rng_table[(i * 0x100) + j] = (rngA << 8) | rngB;
        }
    }
}

uint8 run_rng(uint16 * rng_seed, uint16 * rng_table)
{
	uint16 result = rng_table[*rng_seed];
	*rng_seed = result;

	return ((result >> 8) ^ (result)) & 0xFF;
}


void generate_regular_rng_values(uint16 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
		*rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            regular_rng_values[i*128 + (127 - j)] = run_rng(rng_seed, rng_table);
        }
    }
}

void generate_alg0_values(uint16 * alg0_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg0_values[i*128 + (127 - j)] = (run_rng(rng_seed, rng_table) >> 7) & 0x01;
        }
    }
}

void generate_alg6_values(uint16 * alg6_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
        *rng_seed = i;
        for (int j = 0; j < 128; j++)
        {
            alg6_values[i*128 + j] = run_rng(rng_seed, rng_table) & 0x80;
        }
    }
}

void generate_alg2_values(uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 132; j++)
		{
			alg2_values[i*32 + j] = 0;
		}
        *rng_seed = i;
        alg2_values[i*32+30] = (run_rng(rng_seed, rng_table) & 0x80) >> 7;
    }
}

void generate_alg5_values(uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table)
{
    for (int i = 0; i < 0x10000; i++)
    {
		for (int j = 0; j < 32; j++)
		{
			alg5_values[i*32 + j] = 0;
		}
        *rng_seed = i;
		alg5_values[i*32+30] = run_rng(rng_seed, rng_table) & 0x80;
    }
}

void generate_seed_forward_1(uint16 * values, uint16 * rng_table)
{
	for (int i = 0; i < 0x10000; i++)
    {
		values[i] = rng_table[i];
    }
}

void generate_seed_forward_128(uint16 * values, uint16 * rng_seed, uint16 * rng_table)
{
	for (int i = 0; i < 0x10000; i++)
    {
		*rng_seed = i;
		for (int j = 0; j < 128; j++)
		{
			*rng_seed = rng_table[*rng_seed];
		}
		values[i] = *rng_seed;
    }
}


void run_alg(int alg_id, int iterations, uint8 * working_code, uint8 * regular_rng_values, uint8* alg0_values, uint8* alg2_values, uint8* alg5_values, uint8* alg6_values, uint16 * rng_seed, uint16 * rng_forward_1, uint16 * rng_forward_128)
{
	__m256i working_code0 = _mm256_load_si256((__m256i *)(working_code));
	__m256i working_code1 = _mm256_load_si256((__m256i *)(working_code + 32));
	__m256i working_code2 = _mm256_load_si256((__m256i *)(working_code + 64));
	__m256i working_code3 = _mm256_load_si256((__m256i *)(working_code + 96));
	__m256i working_code4 = _mm256_load_si256((__m256i *)(working_code + 128));
	__m256i working_code5 = _mm256_load_si256((__m256i *)(working_code + 160));
	__m256i working_code6 = _mm256_load_si256((__m256i *)(working_code + 192));
	__m256i working_code7 = _mm256_load_si256((__m256i *)(working_code + 224));

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
	if (alg_id == 0)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8 * rng_start = alg0_values + ((*rng_seed) * 128 * 2);

			__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
			working_code0 = _mm256_slli_epi16(working_code0,1);
			working_code0 = _mm256_or_si256 (working_code0, rng_val);
			working_code0 = _mm256_and_si256 (working_code0, mask_FF);
			
			rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
			working_code1 = _mm256_slli_epi16(working_code1,1);
			working_code1 = _mm256_or_si256 (working_code1, rng_val);
			working_code1 = _mm256_and_si256 (working_code1, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
			working_code2 = _mm256_slli_epi16(working_code2,1);
			working_code2 = _mm256_or_si256 (working_code2, rng_val);
			working_code2 = _mm256_and_si256 (working_code2, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
			working_code3 = _mm256_slli_epi16(working_code3,1);
			working_code3 = _mm256_or_si256 (working_code3, rng_val);
			working_code3 = _mm256_and_si256 (working_code3, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
			working_code4 = _mm256_slli_epi16(working_code4,1);
			working_code4 = _mm256_or_si256 (working_code4, rng_val);
			working_code4 = _mm256_and_si256 (working_code4, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
			working_code5 = _mm256_slli_epi16(working_code5,1);
			working_code5 = _mm256_or_si256 (working_code5, rng_val);
			working_code5 = _mm256_and_si256 (working_code5, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
			working_code6 = _mm256_slli_epi16(working_code6,1);
			working_code6 = _mm256_or_si256 (working_code6, rng_val);
			working_code6 = _mm256_and_si256 (working_code6, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
			working_code7 = _mm256_slli_epi16(working_code7,1);
			working_code7 = _mm256_or_si256 (working_code7, rng_val);
			working_code7 = _mm256_and_si256 (working_code7, mask_FF);

			*rng_seed = rng_forward_128[*rng_seed];
		}
	}
	else if (alg_id == 1)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8 * rng_start = regular_rng_values + ((*rng_seed) * 128 * 2);

			__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
			working_code0 = _mm256_add_epi16(working_code0, rng_val);
			working_code0 = _mm256_and_si256 (working_code0, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
			working_code1 = _mm256_add_epi16(working_code1, rng_val);
			working_code1 = _mm256_and_si256 (working_code1, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
			working_code2 = _mm256_add_epi16(working_code2, rng_val);
			working_code2 = _mm256_and_si256 (working_code2, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
			working_code3 = _mm256_add_epi16(working_code3, rng_val);
			working_code3 = _mm256_and_si256 (working_code3, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
			working_code4 = _mm256_add_epi16(working_code4, rng_val);
			working_code4 = _mm256_and_si256 (working_code4, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
			working_code5 = _mm256_add_epi16(working_code5, rng_val);
			working_code5 = _mm256_and_si256 (working_code5, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
			working_code6 = _mm256_add_epi16(working_code6, rng_val);
			working_code6 = _mm256_and_si256 (working_code6, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
			working_code7 = _mm256_add_epi16(working_code7, rng_val);
			working_code7 = _mm256_and_si256 (working_code7, mask_FF);

			*rng_seed = rng_forward_128[*rng_seed];
		}
	}
	else if (alg_id == 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			__m256i carry = _mm256_load_si256((__m256i *)(alg2_values + ((*rng_seed) * 32)));
			
			__m128i cur_val_lo = _mm256_castsi256_si128(working_code7);
			__m128i cur_val_hi = _mm256_extractf128_si256(working_code7,1);
			__m256i next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_01);
			__m128i temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			__m128i temp_hi = _mm_srli_si128(cur_val_hi,2);
			__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			__m256i temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);

			temp256 = _mm256_srli_epi16(working_code7,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);


			temp256 = _mm256_slli_epi16(working_code7,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);

			working_code7 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));
			carry = next_carry;
			

			cur_val_lo = _mm256_castsi256_si128(working_code6);
			cur_val_hi = _mm256_extractf128_si256(working_code6,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_01);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code6,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code6,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);
			working_code6 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code5);
			cur_val_hi = _mm256_extractf128_si256(working_code5,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_01);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code5,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code5,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);
			working_code5 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code4);
			cur_val_hi = _mm256_extractf128_si256(working_code4,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_01);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code4,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code4,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);
			working_code4 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code3);
			cur_val_hi = _mm256_extractf128_si256(working_code3,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_01);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code3,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code3,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);
			working_code3 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code2);
			cur_val_hi = _mm256_extractf128_si256(working_code2,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_01);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code2,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code2,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);
			working_code2 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code1);
			cur_val_hi = _mm256_extractf128_si256(working_code1,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_01);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code1,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code1,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);
			working_code1 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code0);
			cur_val_hi = _mm256_extractf128_si256(working_code0,1);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg2);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code0,1);
			temp256 = _mm256_and_si256 (temp256, mask_007F);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code0,1);
			temp256 = _mm256_and_si256 (temp256, mask_FE00);
			carry = _mm256_or_si256 (carry, temp256);
			working_code0 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0080));

			*rng_seed = rng_forward_1[*rng_seed];
		}
	}
	else if (alg_id == 3)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8 * rng_start = regular_rng_values + ((*rng_seed) * 128 * 2);

			__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
			working_code0 = _mm256_xor_si256 (working_code0, rng_val);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
			working_code1 = _mm256_xor_si256 (working_code1, rng_val);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
			working_code2 = _mm256_xor_si256 (working_code0, rng_val);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
			working_code3 = _mm256_xor_si256 (working_code0, rng_val);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
			working_code4 = _mm256_xor_si256 (working_code0, rng_val);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
			working_code5 = _mm256_xor_si256 (working_code0, rng_val);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
			working_code6 = _mm256_xor_si256 (working_code0, rng_val);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
			working_code7 = _mm256_xor_si256 (working_code0, rng_val);

			*rng_seed = rng_forward_128[*rng_seed];
		}
	}
	else if (alg_id == 4)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8 * rng_start = regular_rng_values + ((*rng_seed) * 128 * 2);

			__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code0 = _mm256_add_epi16(working_code0, rng_val);
			working_code0 = _mm256_add_epi16(working_code0, mask_01);
			working_code0 = _mm256_and_si256 (working_code0, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code1 = _mm256_add_epi16(working_code1, rng_val);
			working_code1 = _mm256_add_epi16(working_code1, mask_01);
			working_code1 = _mm256_and_si256 (working_code1, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code2 = _mm256_add_epi16(working_code2, rng_val);
			working_code2 = _mm256_add_epi16(working_code2, mask_01);
			working_code2 = _mm256_and_si256 (working_code2, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code3 = _mm256_add_epi16(working_code3, rng_val);
			working_code3 = _mm256_add_epi16(working_code3, mask_01);
			working_code3 = _mm256_and_si256 (working_code3, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code4 = _mm256_add_epi16(working_code4, rng_val);
			working_code4 = _mm256_add_epi16(working_code4, mask_01);
			working_code4 = _mm256_and_si256 (working_code4, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code5 = _mm256_add_epi16(working_code5, rng_val);
			working_code5 = _mm256_add_epi16(working_code5, mask_01);
			working_code5 = _mm256_and_si256 (working_code5, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code6 = _mm256_add_epi16(working_code6, rng_val);
			working_code6 = _mm256_add_epi16(working_code6, mask_01);
			working_code6 = _mm256_and_si256 (working_code6, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
			rng_val = _mm256_xor_si256 (rng_val, mask_FF);
			working_code7 = _mm256_add_epi16(working_code7, rng_val);
			working_code7 = _mm256_add_epi16(working_code7, mask_01);
			working_code7 = _mm256_and_si256 (working_code7, mask_FF);

			*rng_seed = rng_forward_128[*rng_seed];
		}
	}
	else if (alg_id == 5)
	{
		for (int j = 0; j < iterations; j++)
		{
			__m256i carry = _mm256_load_si256((__m256i *)(alg5_values + ((*rng_seed) * 32)));
			
			__m128i cur_val_lo = _mm256_castsi256_si128(working_code7);
			__m128i cur_val_hi = _mm256_extractf128_si256(working_code7,1);
			__m256i next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_80);
			__m128i temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			__m128i temp_hi = _mm_srli_si128(cur_val_hi,2);
			__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			__m256i temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);

			temp256 = _mm256_srli_epi16(working_code7,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);


			temp256 = _mm256_slli_epi16(working_code7,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);

			working_code7 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));
			carry = next_carry;
			

			cur_val_lo = _mm256_castsi256_si128(working_code6);
			cur_val_hi = _mm256_extractf128_si256(working_code6,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_80);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code6,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code6,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);
			working_code6 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code5);
			cur_val_hi = _mm256_extractf128_si256(working_code5,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_80);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code5,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code5,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);
			working_code5 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code4);
			cur_val_hi = _mm256_extractf128_si256(working_code4,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_80);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code4,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code4,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);
			working_code4 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code3);
			cur_val_hi = _mm256_extractf128_si256(working_code3,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_80);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code3,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code3,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);
			working_code3 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code2);
			cur_val_hi = _mm256_extractf128_si256(working_code2,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_80);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code2,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code2,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);
			working_code2 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code1);
			cur_val_hi = _mm256_extractf128_si256(working_code1,1);
			next_carry = _mm256_and_si256(_mm256_set_m128i(_mm_slli_si128(cur_val_lo,14), cur_val_lo), mask_top_80);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code1,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code1,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);
			working_code1 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));
			carry = next_carry;

			cur_val_lo = _mm256_castsi256_si128(working_code0);
			cur_val_hi = _mm256_extractf128_si256(working_code0,1);
			temp_lo = _mm_or_si128(_mm_srli_si128(cur_val_lo,2), _mm_slli_si128(cur_val_hi,14));
			temp_hi = _mm_srli_si128(cur_val_hi,2);
			cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);
			temp256 = _mm256_and_si256 (cur_val_srl, mask_alg5);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_srli_epi16(working_code0,1);
			temp256 = _mm256_and_si256 (temp256, mask_7F00);
			carry = _mm256_or_si256 (carry, temp256);
			temp256 = _mm256_slli_epi16(working_code0,1);
			temp256 = _mm256_and_si256 (temp256, mask_00FE);
			carry = _mm256_or_si256 (carry, temp256);
			working_code0 =  _mm256_or_si256(carry, _mm256_and_si256 (cur_val_srl, mask_0001));

			*rng_seed = rng_forward_1[*rng_seed];
		}
	}
	else if (alg_id == 6)
	{
		for (int j = 0; j < iterations; j++)
		{
			uint8 * rng_start = alg6_values + ((*rng_seed) * 128 * 2);

			__m256i rng_val = _mm256_load_si256((__m256i *)(rng_start));
			working_code0 = _mm256_srli_epi16(working_code0,1);
			working_code0 = _mm256_or_si256 (working_code0, rng_val);
			working_code0 = _mm256_and_si256 (working_code0, mask_FF);
			
			rng_val = _mm256_load_si256((__m256i *)(rng_start + 32));
			working_code1 = _mm256_srli_epi16(working_code1,1);
			working_code1 = _mm256_or_si256 (working_code1, rng_val);
			working_code1 = _mm256_and_si256 (working_code1, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 64));
			working_code2 = _mm256_srli_epi16(working_code2,1);
			working_code2 = _mm256_or_si256 (working_code2, rng_val);
			working_code2 = _mm256_and_si256 (working_code2, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 96));
			working_code3 = _mm256_srli_epi16(working_code3,1);
			working_code3 = _mm256_or_si256 (working_code3, rng_val);
			working_code3 = _mm256_and_si256 (working_code3, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 128));
			working_code4 = _mm256_srli_epi16(working_code4,1);
			working_code4 = _mm256_or_si256 (working_code4, rng_val);
			working_code4 = _mm256_and_si256 (working_code4, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 160));
			working_code5 = _mm256_srli_epi16(working_code5,1);
			working_code5 = _mm256_or_si256 (working_code5, rng_val);
			working_code5 = _mm256_and_si256 (working_code5, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 192));
			working_code6 = _mm256_srli_epi16(working_code6,1);
			working_code6 = _mm256_or_si256 (working_code6, rng_val);
			working_code6 = _mm256_and_si256 (working_code6, mask_FF);

			rng_val = _mm256_load_si256((__m256i *)(rng_start + 224));
			working_code7 = _mm256_srli_epi16(working_code7,1);
			working_code7 = _mm256_or_si256 (working_code7, rng_val);
			working_code7 = _mm256_and_si256 (working_code7, mask_FF);

			*rng_seed = rng_forward_128[*rng_seed];
		}
	}
	else if (alg_id == 7)
	{
		for (int j = 0; j < iterations; j++)
		{
			working_code0 = _mm256_xor_si256 (working_code0, mask_FF);
			working_code1 = _mm256_xor_si256 (working_code1, mask_FF);
			working_code2 = _mm256_xor_si256 (working_code2, mask_FF);
			working_code3 = _mm256_xor_si256 (working_code3, mask_FF);
			working_code4 = _mm256_xor_si256 (working_code4, mask_FF);
			working_code5 = _mm256_xor_si256 (working_code5, mask_FF);
			working_code6 = _mm256_xor_si256 (working_code6, mask_FF);
			working_code7 = _mm256_xor_si256 (working_code7, mask_FF);
		}
	}

	_mm256_store_si256 ((__m256i *)(working_code), working_code0);
	_mm256_store_si256 ((__m256i *)(working_code + 32), working_code1);
	_mm256_store_si256 ((__m256i *)(working_code + 64), working_code2);
	_mm256_store_si256 ((__m256i *)(working_code + 96), working_code3);
	_mm256_store_si256 ((__m256i *)(working_code + 128), working_code4);
	_mm256_store_si256 ((__m256i *)(working_code + 160), working_code5);
	_mm256_store_si256 ((__m256i *)(working_code + 192), working_code6);
	_mm256_store_si256 ((__m256i *)(working_code + 224), working_code7);
}


void * aligned_malloc(int byte_count, int align_size)
{
	uint8 * new_array = new uint8[byte_count + align_size];
	int mod = ((uint64)new_array) % align_size;
	int remainder = align_size - mod;
	uint64 result = (uint64)new_array + (uint64)remainder;
	return (void*)(result);
}