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

#include <iostream>
#include "tm_rev_avx_r128_8.h"

tm_rev_avx_r128_8::tm_rev_avx_r128_8(RNG* rng_obj) : TM_rev_base(rng_obj)
{
	initialize();
}

__forceinline void tm_rev_avx_r128_8::initialize()
{
	if (!initialized)
	{
		rng->generate_seed_forward();

		rng->generate_regular_rng_values_8();

		rng->generate_alg06_values_8();

		rng->generate_alg4_values_8();

		initialized = true;
	}
	obj_name = "tm_rev_avx_r128_8";
}

void tm_rev_avx_r128_8::set_working_code(uint8_t* data)
{
	for (int i = 0; i < 0x80; i++)
	{
		init_working_code_data[i] = data[i];
	}
}

void tm_rev_avx_r128_8::set_trust_mask(uint8_t* data)
{
	for (int i = 0; i < 0x80; i++)
	{
		init_trust_mask[i] = data[i];
	}
}

void tm_rev_avx_r128_8::set_rng_seed(uint16_t seed)
{
	rng_seed_forward = &(rng->seed_forward[seed * 2048]);
}

__forceinline void tm_rev_avx_r128_8::_load_from_mem(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7)
{
	working_code0 = _mm_load_si128((__m128i*)(init_working_code_data));
	working_code1 = _mm_load_si128((__m128i*)(init_working_code_data + 16));
	working_code2 = _mm_load_si128((__m128i*)(init_working_code_data + 32));
	working_code3 = _mm_load_si128((__m128i*)(init_working_code_data + 48));
	working_code4 = _mm_load_si128((__m128i*)(init_working_code_data + 64));
	working_code5 = _mm_load_si128((__m128i*)(init_working_code_data + 80));
	working_code6 = _mm_load_si128((__m128i*)(init_working_code_data + 96));
	working_code7 = _mm_load_si128((__m128i*)(init_working_code_data + 112));

	trust_mask0 = _mm_load_si128((__m128i*)(init_trust_mask));
	trust_mask1 = _mm_load_si128((__m128i*)(init_trust_mask + 16));
	trust_mask2 = _mm_load_si128((__m128i*)(init_trust_mask + 32));
	trust_mask3 = _mm_load_si128((__m128i*)(init_trust_mask + 48));
	trust_mask4 = _mm_load_si128((__m128i*)(init_trust_mask + 64));
	trust_mask5 = _mm_load_si128((__m128i*)(init_trust_mask + 80));
	trust_mask6 = _mm_load_si128((__m128i*)(init_trust_mask + 96));
	trust_mask7 = _mm_load_si128((__m128i*)(init_trust_mask + 112));
}

__forceinline void tm_rev_avx_r128_8::_store_to_mem(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7)
{
	_mm_store_si128((__m128i*)(working_code_data), working_code0);
	_mm_store_si128((__m128i*)(working_code_data + 16), working_code1);
	_mm_store_si128((__m128i*)(working_code_data + 32), working_code2);
	_mm_store_si128((__m128i*)(working_code_data + 48), working_code3);
	_mm_store_si128((__m128i*)(working_code_data + 64), working_code4);
	_mm_store_si128((__m128i*)(working_code_data + 80), working_code5);
	_mm_store_si128((__m128i*)(working_code_data + 96), working_code6);
	_mm_store_si128((__m128i*)(working_code_data + 112), working_code7);

	_mm_store_si128((__m128i*)(trust_mask), trust_mask0);
	_mm_store_si128((__m128i*)(trust_mask + 16), trust_mask1);
	_mm_store_si128((__m128i*)(trust_mask + 32), trust_mask2);
	_mm_store_si128((__m128i*)(trust_mask + 48), trust_mask3);
	_mm_store_si128((__m128i*)(trust_mask + 64), trust_mask4);
	_mm_store_si128((__m128i*)(trust_mask + 80), trust_mask5);
	_mm_store_si128((__m128i*)(trust_mask + 96), trust_mask6);
	_mm_store_si128((__m128i*)(trust_mask + 112), trust_mask7);
}

rev_stats tm_rev_avx_r128_8::run_reverse_process()
{
	__m128i working_code0;
	__m128i working_code1;
	__m128i working_code2;
	__m128i working_code3;
	__m128i working_code4;
	__m128i working_code5;
	__m128i working_code6;
	__m128i working_code7;

	__m128i trust_mask0;
	__m128i trust_mask1;
	__m128i trust_mask2;
	__m128i trust_mask3;
	__m128i trust_mask4;
	__m128i trust_mask5;
	__m128i trust_mask6;
	__m128i trust_mask7;

	_load_from_mem(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, trust_mask0, trust_mask1, trust_mask2, trust_mask3, trust_mask4, trust_mask5, trust_mask6, trust_mask7);

	__m128i mask_FE = _mm_set1_epi16(0xFEFE);
	__m128i mask_FF = _mm_set1_epi16(0xFFFF);
	__m128i mask_01 = _mm_set1_epi16(0x0101);
	__m128i mask_7F = _mm_set1_epi16(0x7F7F);
	__m128i mask_81 = _mm_set1_epi16(0x8181);

	for (int i = 0; i < rev_alg_list_length; i++)
	{
		int alg = rev_alg_list[i];
		uint16_t rng_seed = rng_seed_forward[alg_rng_seed_diff[i]];
		uint8* rng_start;
		switch (alg)
		{
		case 0:
			rev_alg_0(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, trust_mask0, trust_mask1, trust_mask2, trust_mask3, trust_mask4, trust_mask5, trust_mask6, trust_mask7, mask_FE);
			break;
		case 1:
		case 4:
			rng_start = rng->alg4_values_8;

			if (alg == 4)
			{
				rng_start = rng->regular_rng_values_8;
			}

			add_alg(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, trust_mask0, trust_mask1, trust_mask2, trust_mask3, trust_mask4, trust_mask5, trust_mask6, trust_mask7, rng_seed, rng_start, mask_FF, mask_01);

			break;
		case 2:
			rev_alg_2(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, trust_mask0, trust_mask1, trust_mask2, trust_mask3, trust_mask4, trust_mask5, trust_mask6, trust_mask7);
			break;
		case 3:
			rev_alg_3(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, rng_seed);
			break;
		case 5:
			rev_alg_5(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, trust_mask0, trust_mask1, trust_mask2, trust_mask3, trust_mask4, trust_mask5, trust_mask6, trust_mask7);
			break;
		case 6:
			rev_alg_6(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, trust_mask0, trust_mask1, trust_mask2, trust_mask3, trust_mask4, trust_mask5, trust_mask6, trust_mask7, mask_7F);
			break;
		case 7:
			rev_alg_7(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, mask_FF);
			break;
		}
	}

	return check_alg06(working_code0, working_code1, working_code2, working_code3, working_code4, working_code5, working_code6, working_code7, trust_mask0, trust_mask1, trust_mask2, trust_mask3, trust_mask4, trust_mask5, trust_mask6, trust_mask7, mask_81, mask_01, rng_seed_forward[0]);
}

__forceinline void tm_rev_avx_r128_8::add_alg(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, uint16 rng_seed, uint8* rng_start, __m128i& mask_FF, __m128i& mask_01)
{
	rng_start = rng_start + (rng_seed * 128);
	working_code0 = _mm_add_epi8(working_code0, _mm_load_si128((__m128i*)rng_start));
	working_code1 = _mm_add_epi8(working_code1, _mm_load_si128((__m128i*)(rng_start + 16)));
	working_code2 = _mm_add_epi8(working_code2, _mm_load_si128((__m128i*)(rng_start + 32)));
	working_code3 = _mm_add_epi8(working_code3, _mm_load_si128((__m128i*)(rng_start + 48)));
	working_code4 = _mm_add_epi8(working_code4, _mm_load_si128((__m128i*)(rng_start + 64)));
	working_code5 = _mm_add_epi8(working_code5, _mm_load_si128((__m128i*)(rng_start + 80)));
	working_code6 = _mm_add_epi8(working_code6, _mm_load_si128((__m128i*)(rng_start + 96)));
	working_code7 = _mm_add_epi8(working_code7, _mm_load_si128((__m128i*)(rng_start + 112)));

	_add_alg_sub(trust_mask0, mask_FF, mask_01);
	_add_alg_sub(trust_mask1, mask_FF, mask_01);
	_add_alg_sub(trust_mask2, mask_FF, mask_01);
	_add_alg_sub(trust_mask3, mask_FF, mask_01);
	_add_alg_sub(trust_mask4, mask_FF, mask_01);
	_add_alg_sub(trust_mask5, mask_FF, mask_01);
	_add_alg_sub(trust_mask6, mask_FF, mask_01);
	_add_alg_sub(trust_mask7, mask_FF, mask_01);
}

__forceinline void tm_rev_avx_r128_8::_add_alg_sub(__m128i& trust_mask, __m128i& mask_FF, __m128i& mask_01)
{
	trust_mask = _mm_and_si128(trust_mask, _mm_sub_epi8(_mm_xor_si128(trust_mask, mask_FF), mask_01));
}

__forceinline void tm_rev_avx_r128_8::rev_alg_0(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_FE)
{
	alg_0_sub(working_code0, mask_FE);
	alg_0_sub(working_code1, mask_FE);
	alg_0_sub(working_code2, mask_FE);
	alg_0_sub(working_code3, mask_FE);
	alg_0_sub(working_code4, mask_FE);
	alg_0_sub(working_code5, mask_FE);
	alg_0_sub(working_code6, mask_FE);
	alg_0_sub(working_code7, mask_FE);

	alg_0_sub(trust_mask0, mask_FE);
	alg_0_sub(trust_mask1, mask_FE);
	alg_0_sub(trust_mask2, mask_FE);
	alg_0_sub(trust_mask3, mask_FE);
	alg_0_sub(trust_mask4, mask_FE);
	alg_0_sub(trust_mask5, mask_FE);
	alg_0_sub(trust_mask6, mask_FE);
	alg_0_sub(trust_mask7, mask_FE);
}

__forceinline void tm_rev_avx_r128_8::alg_0_sub(__m128i& working_code, __m128i& mask_FE)
{
	working_code = _mm_slli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_FE);
}

__forceinline void tm_rev_avx_r128_8::alg_2_sub(__m128i& working_code, __m128i& carry)
{
	__m128i next_carry = _mm_srli_si128(_mm_and_si128(working_code, _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0)), 15);

	carry = _mm_or_si128(carry, _mm_slli_si128(_mm_and_si128(working_code, _mm_set_epi8(0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0)), 1));

	working_code = _mm_or_si128(
		_mm_or_si128(
			_mm_or_si128(
				_mm_and_si128(
					_mm_slli_epi16(working_code, 1),
					_mm_set_epi8(0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE)),
				_mm_and_si128(
					_mm_srli_epi16(working_code, 1),
					_mm_set_epi8(0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0, 0x7f, 0))),
			_mm_and_si128(
				_mm_slli_si128(working_code, 1),
				_mm_set_epi8(0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0))),
		carry);

	carry = next_carry;
}

__forceinline void tm_rev_avx_r128_8::rev_alg_2(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7)
{
	__m128i carry = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	alg_2_sub(working_code0, carry);
	alg_2_sub(working_code1, carry);
	alg_2_sub(working_code2, carry);
	alg_2_sub(working_code3, carry);
	alg_2_sub(working_code4, carry);
	alg_2_sub(working_code5, carry);
	alg_2_sub(working_code6, carry);
	alg_2_sub(working_code7, carry);

	carry = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	alg_2_sub(trust_mask0, carry);
	alg_2_sub(trust_mask1, carry);
	alg_2_sub(trust_mask2, carry);
	alg_2_sub(trust_mask3, carry);
	alg_2_sub(trust_mask4, carry);
	alg_2_sub(trust_mask5, carry);
	alg_2_sub(trust_mask6, carry);
	alg_2_sub(trust_mask7, carry);
}

__forceinline void tm_rev_avx_r128_8::rev_alg_3(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16 rng_seed)
{
	uint8* rng_start = rng->regular_rng_values_8 + (rng_seed * 128);

	working_code0 = _mm_xor_si128(working_code0, _mm_load_si128((__m128i*)rng_start));
	working_code1 = _mm_xor_si128(working_code1, _mm_load_si128((__m128i*)(rng_start + 16)));
	working_code2 = _mm_xor_si128(working_code2, _mm_load_si128((__m128i*)(rng_start + 32)));
	working_code3 = _mm_xor_si128(working_code3, _mm_load_si128((__m128i*)(rng_start + 48)));
	working_code4 = _mm_xor_si128(working_code4, _mm_load_si128((__m128i*)(rng_start + 64)));
	working_code5 = _mm_xor_si128(working_code5, _mm_load_si128((__m128i*)(rng_start + 80)));
	working_code6 = _mm_xor_si128(working_code6, _mm_load_si128((__m128i*)(rng_start + 96)));
	working_code7 = _mm_xor_si128(working_code7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

__forceinline void tm_rev_avx_r128_8::alg_5_sub(__m128i& working_code, __m128i& carry)
{
	__m128i next_carry = _mm_srli_si128(_mm_and_si128(working_code, _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0)), 15);

	carry = _mm_or_si128(carry, _mm_srli_si128(_mm_and_si128(working_code, _mm_set_epi8(0, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80, 0)), 1));

	working_code = _mm_or_si128(
		_mm_or_si128(
			_mm_or_si128(
				_mm_and_si128(
					_mm_slli_epi16(working_code, 1),
					_mm_set_epi8(0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0, 0xFE, 0)),
				_mm_and_si128(
					_mm_srli_epi16(working_code, 1),
					_mm_set_epi8(0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F, 0, 0x7F))),
			_mm_and_si128(
				_mm_slli_si128(working_code, 1),
				_mm_set_epi8(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0))),
		carry);

	carry = next_carry;
}

__forceinline void tm_rev_avx_r128_8::rev_alg_5(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7)
{
	__m128i carry = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	alg_5_sub(working_code0, carry);
	alg_5_sub(working_code1, carry);
	alg_5_sub(working_code2, carry);
	alg_5_sub(working_code3, carry);
	alg_5_sub(working_code4, carry);
	alg_5_sub(working_code5, carry);
	alg_5_sub(working_code6, carry);
	alg_5_sub(working_code7, carry);

	carry = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	alg_5_sub(trust_mask0, carry);
	alg_5_sub(trust_mask1, carry);
	alg_5_sub(trust_mask2, carry);
	alg_5_sub(trust_mask3, carry);
	alg_5_sub(trust_mask4, carry);
	alg_5_sub(trust_mask5, carry);
	alg_5_sub(trust_mask6, carry);
	alg_5_sub(trust_mask7, carry);
}

__forceinline void tm_rev_avx_r128_8::rev_alg_6(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_7F)
{
	alg_6_sub(working_code0, mask_7F);
	alg_6_sub(working_code1, mask_7F);
	alg_6_sub(working_code2, mask_7F);
	alg_6_sub(working_code3, mask_7F);
	alg_6_sub(working_code4, mask_7F);
	alg_6_sub(working_code5, mask_7F);
	alg_6_sub(working_code6, mask_7F);
	alg_6_sub(working_code7, mask_7F);

	alg_6_sub(trust_mask0, mask_7F);
	alg_6_sub(trust_mask1, mask_7F);
	alg_6_sub(trust_mask2, mask_7F);
	alg_6_sub(trust_mask3, mask_7F);
	alg_6_sub(trust_mask4, mask_7F);
	alg_6_sub(trust_mask5, mask_7F);
	alg_6_sub(trust_mask6, mask_7F);
	alg_6_sub(trust_mask7, mask_7F);
}

__forceinline void tm_rev_avx_r128_8::alg_6_sub(__m128i& working_code, __m128i& mask_7F)
{
	working_code = _mm_srli_epi16(working_code, 1);
	working_code = _mm_and_si128(working_code, mask_7F);
}

__forceinline void tm_rev_avx_r128_8::rev_alg_7(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& mask_FF)
{
	working_code0 = _mm_xor_si128(working_code0, mask_FF);
	working_code1 = _mm_xor_si128(working_code1, mask_FF);
	working_code2 = _mm_xor_si128(working_code2, mask_FF);
	working_code3 = _mm_xor_si128(working_code3, mask_FF);
	working_code4 = _mm_xor_si128(working_code4, mask_FF);
	working_code5 = _mm_xor_si128(working_code5, mask_FF);
	working_code6 = _mm_xor_si128(working_code6, mask_FF);
	working_code7 = _mm_xor_si128(working_code7, mask_FF);
}

__forceinline void tm_rev_avx_r128_8::check_alg06_sub(__m128i& working_code, __m128i& trust_mask, __m128i rng_val, __m128i& alg0_mismatch_sum, __m128i& alg6_mismatch_sum, __m128i& alg0_avail_sum, __m128i& alg6_avail_sum, __m128i& mask_81, __m128i& mask_01)
{
	__m128i cur_mask = _mm_and_si128(trust_mask, mask_81);

	__m128i working_val = _mm_and_si128(working_code, cur_mask);
	__m128i rng_v = _mm_and_si128(rng_val, cur_mask);

	alg0_avail_sum = _mm_add_epi8(alg0_avail_sum, _mm_and_si128(cur_mask, mask_01));
	alg6_avail_sum = _mm_add_epi8(alg6_avail_sum, _mm_and_si128(_mm_srli_epi16(cur_mask, 7), mask_01));

	__m128i mismatch_bits = _mm_xor_si128(working_val, rng_v);

	alg0_mismatch_sum = _mm_add_epi8(alg0_mismatch_sum, _mm_and_si128(mismatch_bits, mask_01));
	alg6_mismatch_sum = _mm_add_epi8(alg6_mismatch_sum, _mm_and_si128(_mm_srli_epi16(mismatch_bits, 7), mask_01));
}

__forceinline uint8_t tm_rev_avx_r128_8::m128_hsum(__m128i& bit_sum)
{
	__m128i r_total = _mm_add_epi8(bit_sum, _mm_srli_si128(bit_sum, 8));
	uint64 total = _mm_cvtsi128_si64(r_total);
	total = total + (total >> 32);
	total = total + (total >> 16);
	total = total + (total >> 8);

	return (total & 0xFF);
}

__forceinline rev_stats tm_rev_avx_r128_8::check_alg06(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_81, __m128i& mask_01, uint16_t rng_seed)
{
	__m128i alg0_mismatch_sum = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m128i alg6_mismatch_sum = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m128i alg0_avail_sum = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m128i alg6_avail_sum = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	uint8_t* rng_table = rng->alg06_values_8 + (rng_seed * 128);

	check_alg06_sub(working_code0, trust_mask0, _mm_load_si128((__m128i*)rng_table), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);
	check_alg06_sub(working_code1, trust_mask1, _mm_load_si128((__m128i*)(rng_table + 16)), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);
	check_alg06_sub(working_code2, trust_mask2, _mm_load_si128((__m128i*)(rng_table + 32)), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);
	check_alg06_sub(working_code3, trust_mask3, _mm_load_si128((__m128i*)(rng_table + 48)), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);
	check_alg06_sub(working_code4, trust_mask4, _mm_load_si128((__m128i*)(rng_table + 64)), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);
	check_alg06_sub(working_code5, trust_mask5, _mm_load_si128((__m128i*)(rng_table + 80)), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);
	check_alg06_sub(working_code6, trust_mask6, _mm_load_si128((__m128i*)(rng_table + 96)), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);
	check_alg06_sub(working_code7, trust_mask7, _mm_load_si128((__m128i*)(rng_table + 112)), alg0_mismatch_sum, alg6_mismatch_sum, alg0_avail_sum, alg6_avail_sum, mask_81, mask_01);

	uint8_t alg0_available_bits = m128_hsum(alg0_avail_sum);
	uint8_t alg0_mismatch_bits = m128_hsum(alg0_mismatch_sum);
	uint8_t alg6_available_bits = m128_hsum(alg6_avail_sum);
	uint8_t alg6_mismatch_bits = m128_hsum(alg6_mismatch_sum);

	return rev_stats(alg0_available_bits, alg0_mismatch_bits, alg6_available_bits, alg6_mismatch_bits);
}

bool tm_rev_avx_r128_8::initialized = false;