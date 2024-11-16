#pragma once
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

#include <cstdint>

#include "alignment2.h"
#include "rng_obj.h"
#include "tm_rev_base.h"

class tm_rev_avx_r128s_8 : public TM_rev_base
{
public:
	tm_rev_avx_r128s_8(RNG* rng);

	virtual void set_working_code(uint8_t* data);
	virtual void set_trust_mask(uint8_t* data);

	virtual void set_rng_seed(uint16_t seed);

	virtual rev_stats run_reverse_process();
	uint8_t m128_hsum(__m128i& bit_sum);
	void check_alg06_sub(__m128i& working_code, __m128i& trust_mask, __m128i rng_val, __m128i& alg0_mismatch_sum, __m128i& alg6_mismatch_sum, __m128i& alg0_avail_sum, __m128i& alg6_avail_sum, __m128i& mask_81, __m128i& mask_01);
	rev_stats check_alg06(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_81, __m128i& mask_01, uint16_t rng_seed);

	void rev_alg_0(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_FE);
	void alg_0_sub(__m128i& working_code, __m128i& mask_FE);
	void rev_alg_2(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void rev_alg_3(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16 rng_seed);
	void rev_alg_5(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void rev_alg_6(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, __m128i& mask_7F);
	void alg_6_sub(__m128i& working_code, __m128i& mask_7F);
	void rev_alg_7(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& mask_FF);

	void _load_from_mem(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7);
	void _store_to_mem(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7);
	void _store_to_mem_unshuffled(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7);

	void print_working_code();

private:
	void initialize();

	void add_alg(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& trust_mask0, __m128i& trust_mask1, __m128i& trust_mask2, __m128i& trust_mask3, __m128i& trust_mask4, __m128i& trust_mask5, __m128i& trust_mask6, __m128i& trust_mask7, uint16 rng_seed, uint8* rng_start, __m128i& mask_FF, __m128i& mask_01);
	void _add_alg_sub(__m128i& working_code, __m128i& mask_FF, __m128i& mask_01);

	uint8_t init_working_code_data[128];
	uint8_t init_trust_mask[128];

	uint8_t working_code_data[128];
	uint8_t trust_mask[128];

	uint16_t* rng_seed_forward;

	static bool initialized;
};