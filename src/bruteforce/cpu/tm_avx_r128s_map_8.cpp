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

#include "data_sizes.h"
#include "tm_avx_r128s_map_8.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

tm_avx_r128s_map_8::tm_avx_r128s_map_8(RNG* rng_obj) : tm_avx_r128s_map_8(rng_obj, 0) {}

tm_avx_r128s_map_8::tm_avx_r128s_map_8(RNG* rng_obj, const uint32_t key) : tm_avx_r128s_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx_r128s_map_8::tm_avx_r128s_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj)
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;

	expansion_values_for_seed_128_8_shuffled = nullptr;
	regular_rng_values_for_seeds_8 = nullptr;
	alg0_values_for_seeds_8 = nullptr;
	alg2_values_for_seeds_128_8 = nullptr;
	alg5_values_for_seeds_128_8 = nullptr;
	alg6_values_for_seeds_8 = nullptr;

	generate_map_rng();
}

tm_avx_r128s_map_8::~tm_avx_r128s_map_8()
{
}

void tm_avx_r128s_map_8::initialize()
{
	if (!initialized)
	{
		shuffle_mem(carnival_world_checksum_mask, carnival_world_checksum_mask_shuffled, 128, false);
		shuffle_mem(carnival_world_data, carnival_world_data_shuffled, 128, false);
		shuffle_mem(other_world_checksum_mask, other_world_checksum_mask_shuffled, 128, false);
		shuffle_mem(other_world_data, other_world_data_shuffled, 128, false);

		rng->generate_expansion_values_128_8_shuffled();

		initialized = true;
	}
	obj_name = "tm_avx_r128s_map_8";
}

// ---------------------------------------------------------------------------
// Map RNG table generation
// ---------------------------------------------------------------------------

void tm_avx_r128s_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(&expansion_values_for_seed_128_8_shuffled, (key >> 16) & 0xFFFF, true, 128);
	rng->generate_regular_rng_values_for_seeds_8(&regular_rng_values_for_seeds_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg0_values_for_seeds_8(&alg0_values_for_seeds_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg2_values_for_seeds_128_8(&alg2_values_for_seeds_128_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg5_values_for_seeds_128_8(&alg5_values_for_seeds_128_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
}

// ---------------------------------------------------------------------------
// Deinterleave loaders
// ---------------------------------------------------------------------------

// Forward deinterleave: block_start[i] = RNG output for logical byte i.
// sel_even picks bytes 0,2,...,14 from a 16-byte chunk (result in low 8 bytes).
// sel_odd  picks bytes 1,3,...,15 from a 16-byte chunk (result in low 8 bytes).
__forceinline void tm_avx_r128s_map_8::_load_fwd(const uint8* block_start, WC_ARGS,	const __m128i& sel_even, const __m128i& sel_odd)
{
	__m128i a, b;

	a = _mm_loadu_si128((const __m128i*)(block_start));
	b = _mm_loadu_si128((const __m128i*)(block_start + 16));
	wc0 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));
	wc1 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));

	a = _mm_loadu_si128((const __m128i*)(block_start + 32));
	b = _mm_loadu_si128((const __m128i*)(block_start + 48));
	wc2 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));
	wc3 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));

	a = _mm_loadu_si128((const __m128i*)(block_start + 64));
	b = _mm_loadu_si128((const __m128i*)(block_start + 80));
	wc4 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));
	wc5 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));

	a = _mm_loadu_si128((const __m128i*)(block_start + 96));
	b = _mm_loadu_si128((const __m128i*)(block_start + 112));
	wc6 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));
	wc7 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));
}

// Reverse deinterleave: wc0[k] = block_start[127-2k], wc1[k] = block_start[126-2k], etc.
// sel_odd  picks bytes 15,13,...,1 from a 16-byte chunk (result in low 8 bytes).
// sel_even picks bytes 14,12,...,0 from a 16-byte chunk (result in low 8 bytes).
__forceinline void tm_avx_r128s_map_8::_load_rev(const uint8* block_start, WC_ARGS, const __m128i& sel_odd, const __m128i& sel_even)
{
	__m128i a, b;

	// wc0/wc1: logical bytes indexed 127,125,...,97 and 126,124,...,96
	a = _mm_loadu_si128((const __m128i*)(block_start + 112));
	b = _mm_loadu_si128((const __m128i*)(block_start + 96));
	wc0 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));
	wc1 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));

	a = _mm_loadu_si128((const __m128i*)(block_start + 80));
	b = _mm_loadu_si128((const __m128i*)(block_start + 64));
	wc2 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));
	wc3 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));

	a = _mm_loadu_si128((const __m128i*)(block_start + 48));
	b = _mm_loadu_si128((const __m128i*)(block_start + 32));
	wc4 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));
	wc5 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));

	a = _mm_loadu_si128((const __m128i*)(block_start + 16));
	b = _mm_loadu_si128((const __m128i*)(block_start));
	wc6 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_odd),  _mm_shuffle_epi8(b, sel_odd));
	wc7 = _mm_unpacklo_epi64(_mm_shuffle_epi8(a, sel_even), _mm_shuffle_epi8(b, sel_even));
}

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128s_map_8::_load_from_mem(WC_ARGS)
{
	wc0 = _mm_load_si128((__m128i*)(working_code_data));
	wc1 = _mm_load_si128((__m128i*)(working_code_data + 16));
	wc2 = _mm_load_si128((__m128i*)(working_code_data + 32));
	wc3 = _mm_load_si128((__m128i*)(working_code_data + 48));
	wc4 = _mm_load_si128((__m128i*)(working_code_data + 64));
	wc5 = _mm_load_si128((__m128i*)(working_code_data + 80));
	wc6 = _mm_load_si128((__m128i*)(working_code_data + 96));
	wc7 = _mm_load_si128((__m128i*)(working_code_data + 112));
}

__forceinline void tm_avx_r128s_map_8::_store_to_mem(WC_ARGS)
{
	_mm_store_si128((__m128i*)(working_code_data),       wc0);
	_mm_store_si128((__m128i*)(working_code_data + 16),  wc1);
	_mm_store_si128((__m128i*)(working_code_data + 32),  wc2);
	_mm_store_si128((__m128i*)(working_code_data + 48),  wc3);
	_mm_store_si128((__m128i*)(working_code_data + 64),  wc4);
	_mm_store_si128((__m128i*)(working_code_data + 80),  wc5);
	_mm_store_si128((__m128i*)(working_code_data + 96),  wc6);
	_mm_store_si128((__m128i*)(working_code_data + 112), wc7);
}

// ---------------------------------------------------------------------------
// Expand (identical to tm_avx_r128s_8 — still uses precomputed expansion table)
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128s_map_8::_expand_code(uint32 data, WC_ARGS)
{
	uint64 x = ((uint64)key << 32) | data;
	__m128i a = _mm_cvtsi64_si128(x);
	__m128i lo_mask = _mm_set_epi8(1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7, 1, 3, 5, 7);
	__m128i hi_mask = _mm_set_epi8(0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6, 0, 2, 4, 6);

	__m128i lo = _mm_shuffle_epi8(a, lo_mask);
	__m128i hi = _mm_shuffle_epi8(a, hi_mask);

	wc0 = lo; wc1 = hi; wc2 = lo; wc3 = hi;
	wc4 = lo; wc5 = hi; wc6 = lo; wc7 = hi;

	// expansion_values_for_seed_128_8_shuffled is already generated for this key's
	// expansion seed, so use it directly with no per-seed offset.
	uint8* rng_start = expansion_values_for_seed_128_8_shuffled;
	wc0 = _mm_add_epi8(wc0, _mm_load_si128((__m128i*)(rng_start)));
	wc1 = _mm_add_epi8(wc1, _mm_load_si128((__m128i*)(rng_start + 16)));
	wc2 = _mm_add_epi8(wc2, _mm_load_si128((__m128i*)(rng_start + 32)));
	wc3 = _mm_add_epi8(wc3, _mm_load_si128((__m128i*)(rng_start + 48)));
	wc4 = _mm_add_epi8(wc4, _mm_load_si128((__m128i*)(rng_start + 64)));
	wc5 = _mm_add_epi8(wc5, _mm_load_si128((__m128i*)(rng_start + 80)));
	wc6 = _mm_add_epi8(wc6, _mm_load_si128((__m128i*)(rng_start + 96)));
	wc7 = _mm_add_epi8(wc7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

void tm_avx_r128s_map_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
		working_code_data[shuffle_8(i, 128)] = new_data[i];
}

void tm_avx_r128s_map_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
		new_data[i] = working_code_data[shuffle_8(i, 128)];
}

// ---------------------------------------------------------------------------
// Carry-propagation helpers (identical to tm_avx_r128s_8)
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128s_map_8::alg_2_sub(
	__m128i& working_a, __m128i& working_b, __m128i& carry)
{
	__m128i temp1         = _mm_srli_epi16(working_a, 1);
	__m128i cur_val1_most = _mm_and_si128(temp1, mask_7F);

	__m128i cur_val2_masked = _mm_and_si128(working_b, mask_80);

	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, mask_01);
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	__m128i temp2         = _mm_slli_epi16(working_b, 1);
	__m128i cur_val2_most = _mm_and_si128(temp2, mask_FE);

	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), mask_top_01);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);
	carry = next_carry;
}

__forceinline void tm_avx_r128s_map_8::alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry)
{
	__m128i temp1         = _mm_slli_epi16(working_a, 1);
	__m128i cur_val1_most = _mm_and_si128(temp1, mask_FE);

	__m128i cur_val2_masked = _mm_and_si128(working_b, mask_01);

	__m128i cur_val1_srl = _mm_srli_si128(working_a, 1);
	__m128i cur_val1_bit = _mm_and_si128(cur_val1_srl, mask_80);
	cur_val1_bit = _mm_or_si128(cur_val1_bit, carry);

	__m128i temp2         = _mm_srli_epi16(working_b, 1);
	__m128i cur_val2_most = _mm_and_si128(temp2, mask_7F);

	__m128i next_carry = _mm_and_si128(_mm_slli_si128(working_a, 15), mask_top_80);

	working_a = _mm_or_si128(cur_val1_most, cur_val2_masked);
	working_b = _mm_or_si128(cur_val2_most, cur_val1_bit);
	carry = next_carry;
}

// ---------------------------------------------------------------------------
// Per-algorithm operations
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128s_map_8::alg_0(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd)
{
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;
	_load_fwd(block_start, r0, r1, r2, r3, r4, r5, r6, r7, sel_even, sel_odd);
	wc0 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc0, 1), mask_FE), r0);
	wc1 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc1, 1), mask_FE), r1);
	wc2 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc2, 1), mask_FE), r2);
	wc3 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc3, 1), mask_FE), r3);
	wc4 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc4, 1), mask_FE), r4);
	wc5 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc5, 1), mask_FE), r5);
	wc6 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc6, 1), mask_FE), r6);
	wc7 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc7, 1), mask_FE), r7);
}

__forceinline void tm_avx_r128s_map_8::alg_1(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd)
{
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;
	_load_fwd(block_start, r0, r1, r2, r3, r4, r5, r6, r7, sel_even, sel_odd);
	wc0 = _mm_add_epi8(wc0, r0);
	wc1 = _mm_add_epi8(wc1, r1);
	wc2 = _mm_add_epi8(wc2, r2);
	wc3 = _mm_add_epi8(wc3, r3);
	wc4 = _mm_add_epi8(wc4, r4);
	wc5 = _mm_add_epi8(wc5, r5);
	wc6 = _mm_add_epi8(wc6, r6);
	wc7 = _mm_add_epi8(wc7, r7);
}

__forceinline void tm_avx_r128s_map_8::alg_2(WC_ARGS, const uint8* carry_ptr)
{
	__m128i carry = _mm_loadu_si128((const __m128i*)carry_ptr);
	alg_2_sub(wc6, wc7, carry);
	alg_2_sub(wc4, wc5, carry);
	alg_2_sub(wc2, wc3, carry);
	alg_2_sub(wc0, wc1, carry);
}

__forceinline void tm_avx_r128s_map_8::alg_3(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd)
{
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;
	_load_fwd(block_start, r0, r1, r2, r3, r4, r5, r6, r7, sel_even, sel_odd);
	wc0 = _mm_xor_si128(wc0, r0);
	wc1 = _mm_xor_si128(wc1, r1);
	wc2 = _mm_xor_si128(wc2, r2);
	wc3 = _mm_xor_si128(wc3, r3);
	wc4 = _mm_xor_si128(wc4, r4);
	wc5 = _mm_xor_si128(wc5, r5);
	wc6 = _mm_xor_si128(wc6, r6);
	wc7 = _mm_xor_si128(wc7, r7);
}

__forceinline void tm_avx_r128s_map_8::alg_4(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd)
{
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;
	_load_fwd(block_start, r0, r1, r2, r3, r4, r5, r6, r7, sel_even, sel_odd);
	wc0 = _mm_sub_epi8(wc0, r0);
	wc1 = _mm_sub_epi8(wc1, r1);
	wc2 = _mm_sub_epi8(wc2, r2);
	wc3 = _mm_sub_epi8(wc3, r3);
	wc4 = _mm_sub_epi8(wc4, r4);
	wc5 = _mm_sub_epi8(wc5, r5);
	wc6 = _mm_sub_epi8(wc6, r6);
	wc7 = _mm_sub_epi8(wc7, r7);
}

__forceinline void tm_avx_r128s_map_8::alg_5(WC_ARGS, const uint8* carry_ptr)
{
	__m128i carry = _mm_loadu_si128((const __m128i*)carry_ptr);
	alg_5_sub(wc6, wc7, carry);
	alg_5_sub(wc4, wc5, carry);
	alg_5_sub(wc2, wc3, carry);
	alg_5_sub(wc0, wc1, carry);
}

__forceinline void tm_avx_r128s_map_8::alg_6(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd)
{
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;
	_load_fwd(block_start, r0, r1, r2, r3, r4, r5, r6, r7, sel_even, sel_odd);
	wc0 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc0, 1), mask_7F), r0);
	wc1 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc1, 1), mask_7F), r1);
	wc2 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc2, 1), mask_7F), r2);
	wc3 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc3, 1), mask_7F), r3);
	wc4 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc4, 1), mask_7F), r4);
	wc5 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc5, 1), mask_7F), r5);
	wc6 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc6, 1), mask_7F), r6);
	wc7 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc7, 1), mask_7F), r7);
}

__forceinline void tm_avx_r128s_map_8::alg_7(WC_ARGS)
{
	wc0 = _mm_xor_si128(wc0, mask_FF);
	wc1 = _mm_xor_si128(wc1, mask_FF);
	wc2 = _mm_xor_si128(wc2, mask_FF);
	wc3 = _mm_xor_si128(wc3, mask_FF);
	wc4 = _mm_xor_si128(wc4, mask_FF);
	wc5 = _mm_xor_si128(wc5, mask_FF);
	wc6 = _mm_xor_si128(wc6, mask_FF);
	wc7 = _mm_xor_si128(wc7, mask_FF);
}

// ---------------------------------------------------------------------------
// add_alg_shuffled — used by expand; indexes precomputed shuffled table by seed
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128s_map_8::add_alg_shuffled(WC_ARGS, uint16* rng_seed, uint8* rng_start)
{
	rng_start += (*rng_seed) * 128;
	wc0 = _mm_add_epi8(wc0, _mm_load_si128((__m128i*)(rng_start)));
	wc1 = _mm_add_epi8(wc1, _mm_load_si128((__m128i*)(rng_start + 16)));
	wc2 = _mm_add_epi8(wc2, _mm_load_si128((__m128i*)(rng_start + 32)));
	wc3 = _mm_add_epi8(wc3, _mm_load_si128((__m128i*)(rng_start + 48)));
	wc4 = _mm_add_epi8(wc4, _mm_load_si128((__m128i*)(rng_start + 64)));
	wc5 = _mm_add_epi8(wc5, _mm_load_si128((__m128i*)(rng_start + 80)));
	wc6 = _mm_add_epi8(wc6, _mm_load_si128((__m128i*)(rng_start + 96)));
	wc7 = _mm_add_epi8(wc7, _mm_load_si128((__m128i*)(rng_start + 112)));
}

// ---------------------------------------------------------------------------
// xor_alg — used by decrypt and alg_3 (precomputed-table path)
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128s_map_8::xor_alg(WC_ARGS, uint8* values)
{
	wc0 = _mm_xor_si128(wc0, _mm_load_si128((__m128i*)(values)));
	wc1 = _mm_xor_si128(wc1, _mm_load_si128((__m128i*)(values + 16)));
	wc2 = _mm_xor_si128(wc2, _mm_load_si128((__m128i*)(values + 32)));
	wc3 = _mm_xor_si128(wc3, _mm_load_si128((__m128i*)(values + 48)));
	wc4 = _mm_xor_si128(wc4, _mm_load_si128((__m128i*)(values + 64)));
	wc5 = _mm_xor_si128(wc5, _mm_load_si128((__m128i*)(values + 80)));
	wc6 = _mm_xor_si128(wc6, _mm_load_si128((__m128i*)(values + 96)));
	wc7 = _mm_xor_si128(wc7, _mm_load_si128((__m128i*)(values + 112)));
}

__forceinline void tm_avx_r128s_map_8::_run_alg(WC_ARGS, int algorithm_id, uint16* local_pos, const uint8* reg_base, const uint8* alg0_base, const uint8* alg2_base, const uint8* alg5_base, const uint8* alg6_base)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS, alg0_base + *local_pos - 127, sel_even, sel_odd);
		*local_pos -= 128;
	}
	else if (algorithm_id == 1)
	{
		alg_1(WC_PASS, reg_base + *local_pos - 127, sel_even, sel_odd);
		*local_pos -= 128;
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS, alg2_base + *local_pos * 16);
		*local_pos -= 1;
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS, reg_base + *local_pos - 127, sel_even, sel_odd);
		*local_pos -= 128;
	}
	else if (algorithm_id == 4)
	{
		alg_4(WC_PASS, reg_base + *local_pos - 127, sel_even, sel_odd);
		*local_pos -= 128;
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS, alg5_base + *local_pos * 16);
		*local_pos -= 1;
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS, alg6_base + (2047 - *local_pos), sel_even, sel_odd);
		*local_pos -= 128;
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS);
	}
}

// ---------------------------------------------------------------------------
// _run_all_maps — hot path using map_rng table + decreasing local_pos
// ---------------------------------------------------------------------------

// _run_all_maps definition below
__forceinline void tm_avx_r128s_map_8::_run_one_map(WC_ARGS, int map_idx)
{
	uint16 nibble_selector = schedule_entries->entries[map_idx].nibble_selector;
	const uint8* reg_base = regular_rng_values_for_seeds_8 + map_idx * 2048;
	const uint8* alg0_base = alg0_values_for_seeds_8 + map_idx * 2048;
	const uint8* alg2_base = alg2_values_for_seeds_128_8 + map_idx * 2048 * 16;
	const uint8* alg5_base = alg5_values_for_seeds_128_8 + map_idx * 2048 * 16;
	const uint8* alg6_base = alg6_values_for_seeds_8 + map_idx * 2048;
	uint16 local_pos = 2047;

	for (int i = 0; i < 16; i++)
	{
		// Flush wc0/wc1 for algorithm byte lookup.
		_mm_store_si128((__m128i*)(working_code_data), wc0);
		_mm_store_si128((__m128i*)(working_code_data + 16), wc1);

		unsigned char nibble = (nibble_selector >> 15) & 0x01;
		nibble_selector <<= 1;

		unsigned char current_byte = working_code_data[shuffle_8(i, 128)];
		if (nibble == 1)
			current_byte >>= 4;
		unsigned char algorithm_id = (current_byte >> 1) & 0x07;

		_run_alg(WC_PASS, algorithm_id, &local_pos, reg_base, alg0_base, alg2_base, alg5_base, alg6_base);
	}

}

__forceinline void tm_avx_r128s_map_8::_run_all_maps(WC_ARGS)
{
	int map_idx = 0;
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
	{
		_run_one_map(WC_PASS, map_idx);
	}
}

void tm_avx_r128s_map_8::run_all_maps(const key_schedule& schedule_entries)
{
	WC_VARS;
	_load_from_mem(WC_PASS);

	_run_all_maps(WC_PASS);

	_store_to_mem(WC_PASS);
}

// ---------------------------------------------------------------------------
// Decrypt helpers
// ---------------------------------------------------------------------------

void tm_avx_r128s_map_8::decrypt_carnival_world()
{
	WC_VARS;
	_load_from_mem(WC_PASS);
	_decrypt_carnival_world(WC_PASS);
	_store_to_mem(WC_PASS);
}

void tm_avx_r128s_map_8::decrypt_other_world()
{
	WC_VARS;
	_load_from_mem(WC_PASS);
	_decrypt_other_world(WC_PASS);
	_store_to_mem(WC_PASS);
}

void tm_avx_r128s_map_8::_decrypt_carnival_world(WC_ARGS)
{
	xor_alg(WC_PASS, carnival_world_data_shuffled);
}

void tm_avx_r128s_map_8::_decrypt_other_world(WC_ARGS)
{
	xor_alg(WC_PASS, other_world_data_shuffled);
}

// ---------------------------------------------------------------------------
// Checksum helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128s_map_8::mid_sum(
	__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask)
{
	__m128i temp_masked  = _mm_and_si128(working_code, sum_mask);
	__m128i temp1_lo_lo  = _mm_and_si128(temp_masked, lo_mask);
	__m128i temp1_lo_hi  = _mm_srli_epi16(temp_masked, 8);
	sum = _mm_add_epi16(sum, temp1_lo_lo);
	sum = _mm_add_epi16(sum, temp1_lo_hi);
}

__forceinline uint16 tm_avx_r128s_map_8::masked_checksum(WC_ARGS, uint8* mask)
{
	__m128i sum     = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m128i sum_mask = _mm_load_si128((__m128i*)(mask));      mid_sum(sum, wc0, sum_mask, lo_mask);
	sum_mask = _mm_load_si128((__m128i*)(mask + 16));         mid_sum(sum, wc1, sum_mask, lo_mask);
	sum_mask = _mm_load_si128((__m128i*)(mask + 32));         mid_sum(sum, wc2, sum_mask, lo_mask);
	sum_mask = _mm_load_si128((__m128i*)(mask + 48));         mid_sum(sum, wc3, sum_mask, lo_mask);
	sum_mask = _mm_load_si128((__m128i*)(mask + 64));         mid_sum(sum, wc4, sum_mask, lo_mask);
	sum_mask = _mm_load_si128((__m128i*)(mask + 80));         mid_sum(sum, wc5, sum_mask, lo_mask);
	sum_mask = _mm_load_si128((__m128i*)(mask + 96));         mid_sum(sum, wc6, sum_mask, lo_mask);
	sum_mask = _mm_load_si128((__m128i*)(mask + 112));        mid_sum(sum, wc7, sum_mask, lo_mask);

	return (uint16)(
		_mm_extract_epi16(sum, 0) + _mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) + _mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) + _mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) + _mm_extract_epi16(sum, 7));
}

uint16 tm_avx_r128s_map_8::calculate_carnival_world_checksum()
{
	WC_VARS;
	_load_from_mem(WC_PASS);
	return _calculate_carnival_world_checksum(WC_PASS);
}

uint16 tm_avx_r128s_map_8::calculate_other_world_checksum()
{
	WC_VARS;
	_load_from_mem(WC_PASS);
	return _calculate_other_world_checksum(WC_PASS);
}

uint16 tm_avx_r128s_map_8::_calculate_carnival_world_checksum(WC_ARGS)
{
	return masked_checksum(WC_PASS, carnival_world_checksum_mask_shuffled);
}

uint16 tm_avx_r128s_map_8::_calculate_other_world_checksum(WC_ARGS)
{
	return masked_checksum(WC_PASS, other_world_checksum_mask_shuffled);
}

__forceinline uint16 tm_avx_r128s_map_8::fetch_checksum_value(WC_ARGS, uint8 code_length)
{
	_store_to_mem(WC_PASS);

	unsigned char lo = working_code_data[shuffle_8(127 - code_length,       128)];
	unsigned char hi = working_code_data[shuffle_8(127 - (code_length + 1), 128)];
	return (uint16)((hi << 8) | lo);
}

uint16 tm_avx_r128s_map_8::_fetch_carnival_world_checksum_value(WC_ARGS)
{
	return fetch_checksum_value(WC_PASS, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

uint16 tm_avx_r128s_map_8::_fetch_other_world_checksum_value(WC_ARGS)
{
	return fetch_checksum_value(WC_PASS, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx_r128s_map_8::check_carnival_world_checksum(WC_ARGS)
{
	return _calculate_carnival_world_checksum(WC_PASS)
	    == _fetch_carnival_world_checksum_value(WC_PASS);
}

__forceinline bool tm_avx_r128s_map_8::check_other_world_checksum(WC_ARGS)
{
	return _calculate_other_world_checksum(WC_PASS)
	    == _fetch_other_world_checksum_value(WC_PASS);
}

// ---------------------------------------------------------------------------
// Bruteforce methods
// ---------------------------------------------------------------------------

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8> tm_avx_r128s_map_8::_decrypt_check(WC_ARGS)
{
	WCXOR_VARS;
	WCXOR_COPY;
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(WCXOR_PASS);

		if constexpr (CHECK_CHECKSUM) {
			if (!check_carnival_world_checksum(WCXOR_PASS))
			{
				return std::nullopt;
			}
		}
	}
	else
	{
		_decrypt_other_world(WCXOR_PASS);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum(WCXOR_PASS))
			{
				return std::nullopt;
			}
		}
	}

	_store_to_mem(WCXOR_PASS);
	uint8 unshuffled_data[128];
	unshuffle_mem(working_code_data, unshuffled_data, 128, false);
	return check_machine_code(unshuffled_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx_r128s_map_8::_run_bruteforce(WC_ARGS, uint32 data, uint8* result_data, uint32* result_size)
{
	_expand_code(data, WC_PASS);

	_run_all_maps(WC_PASS);

	auto carnival_flags = _decrypt_check<CHECK_CHECKSUMS, CARNIVAL_WORLD>(WC_PASS);
	if constexpr (CHECK_CHECKSUMS)
	{
		if (carnival_flags.has_value())
		{
			*((uint32*)(&result_data[*result_size])) = data;
			result_data[*result_size + 4] = *carnival_flags;
			*result_size += 5;

			return;
		}
	}

	auto other_flags = _decrypt_check<CHECK_CHECKSUMS, OTHER_WORLD>(WC_PASS);
	if constexpr (CHECK_CHECKSUMS)
	{
		if (other_flags.has_value())
		{
			*((uint32*)(&result_data[*result_size])) = data;
			result_data[*result_size + 4] = *other_flags;
			*result_size += 5;

			return;
		}
	}
	else
	{
		result_data[*result_size] = carnival_flags.value();
		result_data[*result_size + 1] = other_flags.value();
		*result_size += 2;
	}
}

void tm_avx_r128s_map_8::run_bruteforce_boinc(uint32 start_data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	WC_VARS;

	*result_size = 0;
	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
			return;
		uint32 data = start_data + i;

		_run_bruteforce<true>(WC_PASS, data, result_data, result_size);

		report_progress((double)(i + 1) / amount_to_run);
	}
}

void tm_avx_r128s_map_8::compute_challenge_flags(uint32 data, uint8& carnival_flags_out, uint8& other_flags_out)
{
	WC_VARS;

	uint8 result_data[2];
	uint32 result_pos = 0;

	_run_bruteforce<false>(WC_PASS, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------

bool tm_avx_r128s_map_8::initialized = false;

uint8 tm_avx_r128s_map_8::carnival_world_checksum_mask_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8 tm_avx_r128s_map_8::carnival_world_data_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8 tm_avx_r128s_map_8::other_world_checksum_mask_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8 tm_avx_r128s_map_8::other_world_data_shuffled[128] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void tm_avx_r128s_map_8::test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed)
{
	WC_VARS;
	load_data(data);
	_load_from_mem(WC_PASS);

	if (algorithm_id == 0)
	{
		rng->generate_alg0_values_for_seeds_8(&alg0_values_for_seeds_8, rng_seed, 1);
	}
	else if (algorithm_id == 1 || algorithm_id == 3 || algorithm_id == 4)
	{
		rng->generate_regular_rng_values_for_seeds_8(&regular_rng_values_for_seeds_8, rng_seed, 1);
	}
	else if (algorithm_id == 2)
	{
		rng->generate_alg2_values_for_seeds_128_8(&alg2_values_for_seeds_128_8, rng_seed, 1);
	}
	else if (algorithm_id == 5)
	{
		rng->generate_alg5_values_for_seeds_128_8(&alg5_values_for_seeds_128_8, rng_seed, 1);
	}
	else if (algorithm_id == 6)
	{
		rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, rng_seed, 1);
	}

	uint16_t local_pos = 2047;
	_run_alg(WC_PASS, algorithm_id, &local_pos, regular_rng_values_for_seeds_8, alg0_values_for_seeds_8, alg2_values_for_seeds_128_8, alg5_values_for_seeds_128_8, alg6_values_for_seeds_8);
	local_pos = 0;

	_store_to_mem(WC_PASS);
	fetch_data(data);
}

void tm_avx_r128s_map_8::test_expansion(uint32_t data, uint8* result_out)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_store_to_mem(WC_PASS);
	fetch_data(result_out);
}

void tm_avx_r128s_map_8::test_bruteforce_data(uint32 data, uint8* result_out)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_run_all_maps(WC_PASS);
	_store_to_mem(WC_PASS);
	fetch_data(result_out);
}

bool tm_avx_r128s_map_8::test_bruteforce_checksum(uint32 data, int world)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_run_all_maps(WC_PASS);

	if (world == CARNIVAL_WORLD)
	{
		return _decrypt_check<true, CARNIVAL_WORLD>(WC_PASS).has_value();
	}
	else
	{
		return _decrypt_check<true, OTHER_WORLD>(WC_PASS).has_value();
	}
}


alignas(16) const __m128i tm_avx_r128s_map_8::mask_FF = _mm_set1_epi16(0xFFFF);
alignas(16) const __m128i tm_avx_r128s_map_8::mask_FE = _mm_set1_epi16(0xFEFE);
alignas(16) const __m128i tm_avx_r128s_map_8::mask_7F = _mm_set1_epi16(0x7F7F);
alignas(16) const __m128i tm_avx_r128s_map_8::mask_80 = _mm_set1_epi16(0x8080);
alignas(16) const __m128i tm_avx_r128s_map_8::mask_01 = _mm_set1_epi16(0x0101);
alignas(16) const __m128i tm_avx_r128s_map_8::mask_top_01 = _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0);
alignas(16) const __m128i tm_avx_r128s_map_8::mask_top_80 = _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0);
alignas(16) const __m128i tm_avx_r128s_map_8::sel_even = _mm_set_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 14, 12, 10, 8, 6, 4, 2, 0);
alignas(16) const __m128i tm_avx_r128s_map_8::sel_odd = _mm_set_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 15, 13, 11, 9, 7, 5, 3, 1);
