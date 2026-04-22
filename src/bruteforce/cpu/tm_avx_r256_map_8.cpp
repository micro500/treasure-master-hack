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

#include "tm_avx_r256_map_8.h"

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
        _mm256_castpd_si256(_mm256_insertf128_pd(_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), _mm_castsi128_pd(vh), 1))
#endif

// ---------------------------------------------------------------------------
// Constructors / destructor
// ---------------------------------------------------------------------------

tm_avx_r256_map_8::tm_avx_r256_map_8(RNG* rng_obj) : tm_avx_r256_map_8(rng_obj, 0) {}

tm_avx_r256_map_8::tm_avx_r256_map_8(RNG* rng_obj, const uint32_t key) : tm_avx_r256_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx_r256_map_8::tm_avx_r256_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj)
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;

	expansion_values_for_seed_128_8 = nullptr;
	regular_rng_values_for_seeds_8 = nullptr;
	alg0_values_for_seeds_8 = nullptr;
	alg2_values_for_seeds_256_8 = nullptr;
	alg5_values_for_seeds_256_8 = nullptr;
	alg6_values_for_seeds_8 = nullptr;

	generate_map_rng();
}

tm_avx_r256_map_8::~tm_avx_r256_map_8()
{
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

void tm_avx_r256_map_8::initialize()
{
	if (!initialized)
	{
		initialized = true;
	}
	obj_name = "tm_avx_r256_map_8";
}

// ---------------------------------------------------------------------------
// Map RNG table generation
// ---------------------------------------------------------------------------

void tm_avx_r256_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(&expansion_values_for_seed_128_8, (key >> 16) & 0xFFFF, false, 128);
	rng->generate_regular_rng_values_for_seeds_8(&regular_rng_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg0_values_for_seeds_8(&alg0_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg2_values_for_seeds(&alg2_values_for_seeds_256_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count, 256, false);
	rng->generate_alg5_values_for_seeds(&alg5_values_for_seeds_256_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count, 256, false);
	rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, const_cast<uint16_t*>(schedule_entries->seeds), schedule_entries->entry_count);
}

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::_load_from_mem(WC_ARGS_256)
{
	wc0 = _mm256_load_si256((__m256i*)(working_code_data));
	wc1 = _mm256_load_si256((__m256i*)(working_code_data + 32));
	wc2 = _mm256_load_si256((__m256i*)(working_code_data + 64));
	wc3 = _mm256_load_si256((__m256i*)(working_code_data + 96));
}

__forceinline void tm_avx_r256_map_8::_store_to_mem(WC_ARGS_256)
{
	_mm256_store_si256((__m256i*)(working_code_data),      wc0);
	_mm256_store_si256((__m256i*)(working_code_data + 32), wc1);
	_mm256_store_si256((__m256i*)(working_code_data + 64), wc2);
	_mm256_store_si256((__m256i*)(working_code_data + 96), wc3);
}

void tm_avx_r256_map_8::load_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		working_code_data[i] = new_data[i];
}

void tm_avx_r256_map_8::fetch_data(uint8_t* new_data)
{
	for (int i = 0; i < 128; i++)
		new_data[i] = working_code_data[i];
}

// ---------------------------------------------------------------------------
// Expand
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::_expand_code(uint32_t data, WC_ARGS_256)
{
	// Build natural 16-byte pattern [K0,K1,K2,K3,D0,D1,D2,D3] × 2, then
	// broadcast to both 128-bit halves of each __m256i register.
	uint64_t x = ((uint64_t)key << 32) | data;
	__m128i a = _mm_cvtsi64_si128(static_cast<int64_t>(x));
	__m128i nat_mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7);
	__m128i pattern = _mm_shuffle_epi8(a, nat_mask);
	__m256i pat256 = _mm256_set_m128i(pattern, pattern);

	wc0 = pat256; wc1 = pat256; wc2 = pat256; wc3 = pat256;

	uint8_t* rng_start = expansion_values_for_seed_128_8;

	__m128i lo, hi;

	lo = _mm256_castsi256_si128(wc0);
	hi = _mm256_extractf128_si256(wc0, 1);
	lo = _mm_add_epi8(lo, _mm_load_si128((__m128i*)(rng_start)));
	hi = _mm_add_epi8(hi, _mm_load_si128((__m128i*)(rng_start + 16)));
	wc0 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc1);
	hi = _mm256_extractf128_si256(wc1, 1);
	lo = _mm_add_epi8(lo, _mm_load_si128((__m128i*)(rng_start + 32)));
	hi = _mm_add_epi8(hi, _mm_load_si128((__m128i*)(rng_start + 48)));
	wc1 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc2);
	hi = _mm256_extractf128_si256(wc2, 1);
	lo = _mm_add_epi8(lo, _mm_load_si128((__m128i*)(rng_start + 64)));
	hi = _mm_add_epi8(hi, _mm_load_si128((__m128i*)(rng_start + 80)));
	wc2 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc3);
	hi = _mm256_extractf128_si256(wc3, 1);
	lo = _mm_add_epi8(lo, _mm_load_si128((__m128i*)(rng_start + 96)));
	hi = _mm_add_epi8(hi, _mm_load_si128((__m128i*)(rng_start + 112)));
	wc3 = _mm256_set_m128i(hi, lo);
}

// ---------------------------------------------------------------------------
// Carry-propagation helpers (natural interleaved layout, AVX1)
// ---------------------------------------------------------------------------

// alg_2_sub: processes one 256-bit register with carry chain (right-rotate variant).
// carry format: 32-byte __m256i with carry bit at byte 31, bit 0 (mask_top_01).
// On return, carry holds the outgoing carry from byte 0 of wc, ready for the next register.
__forceinline void tm_avx_r256_map_8::alg_2_sub(__m256i& wc, __m256i& carry)
{
	__m128i cur_val_lo = _mm256_castsi256_si128(wc);
	__m128i cur_val_hi = _mm256_extractf128_si256(wc, 1);

	// next_carry = bit 0 of byte 0, placed at byte 31
	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)),
		_mm256_castsi256_pd(mask_top_01)));

	// Shift entire 256-bit value right by one byte (across 128-bit lane boundary)
	__m128i temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 1);
	__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

	// Merge internal carries (bit 0 of each even byte → high byte of next-lower word)
	// with incoming external carry (for word 15)
	__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg2)));
	carry = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));

	// part1: (even bytes >> 1) & 0x7F
	__m128i tlo = _mm_srli_epi16(cur_val_lo, 1);
	__m128i thi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(thi, tlo);
	__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_007F)));

	// part2: (odd bytes << 1) & 0xFE (in high byte position)
	tlo = _mm_slli_epi16(cur_val_lo, 1);
	thi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(thi, tlo);
	__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_FE00)));

	// part3: bit 7 of odd bytes → new even bytes (via byte-shifted view)
	__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0080)));

	wc = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	wc = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(wc), _mm256_castsi256_pd(part3)));
	wc = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(wc), _mm256_castsi256_pd(carry)));

	carry = next_carry;
}

// alg_5_sub: symmetric left-rotate variant.
// carry format: byte 31, bit 7 (mask_top_80).
__forceinline void tm_avx_r256_map_8::alg_5_sub(__m256i& wc, __m256i& carry)
{
	__m128i cur_val_lo = _mm256_castsi256_si128(wc);
	__m128i cur_val_hi = _mm256_extractf128_si256(wc, 1);

	// next_carry = bit 7 of byte 0, placed at byte 31 bit 7
	__m256i next_carry = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(_mm256_set_m128i(_mm_slli_si128(cur_val_lo, 15), cur_val_lo)),
		_mm256_castsi256_pd(mask_top_80)));

	// Shift entire 256-bit value right by one byte
	__m128i temp_lo = _mm_srli_si128(cur_val_lo, 1);
	temp_lo = _mm_or_si128(temp_lo, _mm_slli_si128(cur_val_hi, 15));
	__m128i temp_hi = _mm_srli_si128(cur_val_hi, 1);
	__m256i cur_val_srl = _mm256_set_m128i(temp_hi, temp_lo);

	// Merge internal carries (bit 7 of each even byte → high byte of next-lower word)
	__m256i temp256 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_alg5)));
	carry = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(carry), _mm256_castsi256_pd(temp256)));

	// part1: (odd bytes >> 1) & 0x7F (in high byte position)
	__m128i tlo = _mm_srli_epi16(cur_val_lo, 1);
	__m128i thi = _mm_srli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(thi, tlo);
	__m256i part1 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_7F00)));

	// part2: (even bytes << 1) & 0xFE
	tlo = _mm_slli_epi16(cur_val_lo, 1);
	thi = _mm_slli_epi16(cur_val_hi, 1);
	temp256 = _mm256_set_m128i(thi, tlo);
	__m256i part2 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(temp256), _mm256_castsi256_pd(mask_00FE)));

	// part3: bit 0 of odd bytes → new even bytes (via byte-shifted view)
	__m256i part3 = _mm256_castpd_si256(_mm256_and_pd(
		_mm256_castsi256_pd(cur_val_srl), _mm256_castsi256_pd(mask_0001)));

	wc = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(part1), _mm256_castsi256_pd(part2)));
	wc = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(wc), _mm256_castsi256_pd(part3)));
	wc = _mm256_castpd_si256(_mm256_or_pd(
		_mm256_castsi256_pd(wc), _mm256_castsi256_pd(carry)));

	carry = next_carry;
}

// ---------------------------------------------------------------------------
// Per-algorithm operations
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::alg_0(WC_ARGS_256, const uint8_t* block_start)
{
	__m128i lo, hi;
	__m256i rng_val;

	lo = _mm256_castsi256_si128(wc0); lo = _mm_slli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc0, 1); hi = _mm_slli_epi16(hi, 1);
	wc0 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start));
	wc0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc0), _mm256_castsi256_pd(mask_FE)));
	wc0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc0), _mm256_castsi256_pd(rng_val)));

	lo = _mm256_castsi256_si128(wc1); lo = _mm_slli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc1, 1); hi = _mm_slli_epi16(hi, 1);
	wc1 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 32));
	wc1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc1), _mm256_castsi256_pd(mask_FE)));
	wc1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc1), _mm256_castsi256_pd(rng_val)));

	lo = _mm256_castsi256_si128(wc2); lo = _mm_slli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc2, 1); hi = _mm_slli_epi16(hi, 1);
	wc2 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 64));
	wc2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc2), _mm256_castsi256_pd(mask_FE)));
	wc2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc2), _mm256_castsi256_pd(rng_val)));

	lo = _mm256_castsi256_si128(wc3); lo = _mm_slli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc3, 1); hi = _mm_slli_epi16(hi, 1);
	wc3 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 96));
	wc3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc3), _mm256_castsi256_pd(mask_FE)));
	wc3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_r256_map_8::alg_1(WC_ARGS_256, const uint8_t* block_start)
{
	__m128i lo, hi;

	lo = _mm256_castsi256_si128(wc0);
	hi = _mm256_extractf128_si256(wc0, 1);
	lo = _mm_add_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start)));
	hi = _mm_add_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 16)));
	wc0 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc1);
	hi = _mm256_extractf128_si256(wc1, 1);
	lo = _mm_add_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start + 32)));
	hi = _mm_add_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 48)));
	wc1 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc2);
	hi = _mm256_extractf128_si256(wc2, 1);
	lo = _mm_add_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start + 64)));
	hi = _mm_add_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 80)));
	wc2 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc3);
	hi = _mm256_extractf128_si256(wc3, 1);
	lo = _mm_add_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start + 96)));
	hi = _mm_add_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 112)));
	wc3 = _mm256_set_m128i(hi, lo);
}

__forceinline void tm_avx_r256_map_8::alg_2(WC_ARGS_256, const uint8_t* carry_ptr)
{
	// alg2_values_for_seeds_256_8 stores carry at byte 31 (mask_top_01)
	__m256i carry = _mm256_loadu_si256((const __m256i*)carry_ptr);
	alg_2_sub(wc3, carry);
	alg_2_sub(wc2, carry);
	alg_2_sub(wc1, carry);
	alg_2_sub(wc0, carry);
}

__forceinline void tm_avx_r256_map_8::alg_3(WC_ARGS_256, const uint8_t* block_start)
{
	__m256i rng_val;

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start));
	wc0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc0), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 32));
	wc1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc1), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 64));
	wc2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc2), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 96));
	wc3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_r256_map_8::alg_4(WC_ARGS_256, const uint8_t* block_start)
{
	__m128i lo, hi;

	lo = _mm256_castsi256_si128(wc0);
	hi = _mm256_extractf128_si256(wc0, 1);
	lo = _mm_sub_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start)));
	hi = _mm_sub_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 16)));
	wc0 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc1);
	hi = _mm256_extractf128_si256(wc1, 1);
	lo = _mm_sub_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start + 32)));
	hi = _mm_sub_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 48)));
	wc1 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc2);
	hi = _mm256_extractf128_si256(wc2, 1);
	lo = _mm_sub_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start + 64)));
	hi = _mm_sub_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 80)));
	wc2 = _mm256_set_m128i(hi, lo);

	lo = _mm256_castsi256_si128(wc3);
	hi = _mm256_extractf128_si256(wc3, 1);
	lo = _mm_sub_epi8(lo, _mm_loadu_si128((const __m128i*)(block_start + 96)));
	hi = _mm_sub_epi8(hi, _mm_loadu_si128((const __m128i*)(block_start + 112)));
	wc3 = _mm256_set_m128i(hi, lo);
}

__forceinline void tm_avx_r256_map_8::alg_5(WC_ARGS_256, const uint8_t* carry_ptr)
{
	__m256i carry = _mm256_loadu_si256((const __m256i*)carry_ptr);
	alg_5_sub(wc3, carry);
	alg_5_sub(wc2, carry);
	alg_5_sub(wc1, carry);
	alg_5_sub(wc0, carry);
}

__forceinline void tm_avx_r256_map_8::alg_6(WC_ARGS_256, const uint8_t* block_start)
{
	__m128i lo, hi;
	__m256i rng_val;

	lo = _mm256_castsi256_si128(wc0); lo = _mm_srli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc0, 1); hi = _mm_srli_epi16(hi, 1);
	wc0 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start));
	wc0 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc0), _mm256_castsi256_pd(mask_7F)));
	wc0 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc0), _mm256_castsi256_pd(rng_val)));

	lo = _mm256_castsi256_si128(wc1); lo = _mm_srli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc1, 1); hi = _mm_srli_epi16(hi, 1);
	wc1 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 32));
	wc1 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc1), _mm256_castsi256_pd(mask_7F)));
	wc1 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc1), _mm256_castsi256_pd(rng_val)));

	lo = _mm256_castsi256_si128(wc2); lo = _mm_srli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc2, 1); hi = _mm_srli_epi16(hi, 1);
	wc2 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 64));
	wc2 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc2), _mm256_castsi256_pd(mask_7F)));
	wc2 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc2), _mm256_castsi256_pd(rng_val)));

	lo = _mm256_castsi256_si128(wc3); lo = _mm_srli_epi16(lo, 1);
	hi = _mm256_extractf128_si256(wc3, 1); hi = _mm_srli_epi16(hi, 1);
	wc3 = _mm256_set_m128i(hi, lo);
	rng_val = _mm256_loadu_si256((const __m256i*)(block_start + 96));
	wc3 = _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(wc3), _mm256_castsi256_pd(mask_7F)));
	wc3 = _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(wc3), _mm256_castsi256_pd(rng_val)));
}

__forceinline void tm_avx_r256_map_8::alg_7(WC_ARGS_256)
{
	wc0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc0), _mm256_castsi256_pd(mask_FF)));
	wc1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc1), _mm256_castsi256_pd(mask_FF)));
	wc2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc2), _mm256_castsi256_pd(mask_FF)));
	wc3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc3), _mm256_castsi256_pd(mask_FF)));
}

// ---------------------------------------------------------------------------
// xor_alg — used by decrypt
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::xor_alg(WC_ARGS_256, uint8_t* values)
{
	__m256i rng_val;

	rng_val = _mm256_load_si256((__m256i*)(values));
	wc0 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc0), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(values + 32));
	wc1 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc1), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(values + 64));
	wc2 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc2), _mm256_castsi256_pd(rng_val)));

	rng_val = _mm256_load_si256((__m256i*)(values + 96));
	wc3 = _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(wc3), _mm256_castsi256_pd(rng_val)));
}

// ---------------------------------------------------------------------------
// _run_alg — dispatches to the appropriate algorithm
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::_run_alg(WC_ARGS_256, int algorithm_id, uint16_t* local_pos,
	const uint8_t* reg_base, const uint8_t* alg0_base,
	const uint8_t* alg2_base, const uint8_t* alg5_base,
	const uint8_t* alg6_base)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS_256, alg0_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 1)
	{
		alg_1(WC_PASS_256, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS_256, alg2_base + *local_pos * 32);
		*local_pos -= 1;
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS_256, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 4)
	{
		alg_4(WC_PASS_256, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS_256, alg5_base + *local_pos * 32);
		*local_pos -= 1;
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS_256, alg6_base + (2047 - *local_pos));
		*local_pos -= 128;
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS_256);
	}
}

// ---------------------------------------------------------------------------
// _run_one_map / _run_all_maps
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::_run_one_map(WC_ARGS_256, int map_idx)
{
	uint16_t nibble_selector = schedule_entries->entries[static_cast<size_t>(map_idx)].nibble_selector;
	const uint8_t* reg_base = regular_rng_values_for_seeds_8 + map_idx * 2048;
	const uint8_t* alg0_base = alg0_values_for_seeds_8 + map_idx * 2048;
	const uint8_t* alg2_base = alg2_values_for_seeds_256_8 + map_idx * 2048 * 32;
	const uint8_t* alg5_base = alg5_values_for_seeds_256_8 + map_idx * 2048 * 32;
	const uint8_t* alg6_base = alg6_values_for_seeds_8 + map_idx * 2048;
	uint16_t local_pos = 2047;

	for (int i = 0; i < 16; i++)
	{
		// Natural layout: bytes 0-31 are in wc0.
		_mm256_store_si256((__m256i*)(working_code_data), wc0);

		unsigned char nibble = static_cast<uint8_t>((nibble_selector >> 15) & 0x01);
		nibble_selector <<= 1;

		unsigned char current_byte = working_code_data[i];
		if (nibble == 1)
			current_byte >>= 4;
		unsigned char algorithm_id = static_cast<uint8_t>((current_byte >> 1) & 0x07);

		_run_alg(WC_PASS_256, algorithm_id, &local_pos, reg_base, alg0_base, alg2_base, alg5_base, alg6_base);
	}
}

__forceinline void tm_avx_r256_map_8::_run_all_maps(WC_ARGS_256)
{
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
	{
		_run_one_map(WC_PASS_256, map_idx);
	}
}

// ---------------------------------------------------------------------------
// Decrypt helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::_decrypt_carnival_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, carnival_world_data);
}

__forceinline void tm_avx_r256_map_8::_decrypt_other_world(WC_ARGS_256)
{
	xor_alg(WC_PASS_256, other_world_data);
}

// ---------------------------------------------------------------------------
// Checksum helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r256_map_8::mid_sum(
	__m128i& sum, __m256i& working_code, __m256i& sum_mask, __m128i& lo_mask)
{
	__m128i masked_lo = _mm_and_si128(
		_mm256_castsi256_si128(working_code),
		_mm256_castsi256_si128(sum_mask));
	__m128i masked_hi = _mm_and_si128(
		_mm256_extractf128_si256(working_code, 1),
		_mm256_extractf128_si256(sum_mask, 1));

	sum = _mm_add_epi16(sum, _mm_and_si128(masked_lo, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_lo, 8));
	sum = _mm_add_epi16(sum, _mm_and_si128(masked_hi, lo_mask));
	sum = _mm_add_epi16(sum, _mm_srli_epi16(masked_hi, 8));
}

__forceinline uint16_t tm_avx_r256_map_8::masked_checksum(WC_ARGS_256, uint8_t* mask)
{
	__m128i sum = _mm_setzero_si128();
	__m128i lo_mask = _mm_set1_epi16(0x00FF);

	__m256i sum_mask = _mm256_load_si256((__m256i*)(mask));       mid_sum(sum, wc0, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 32));          mid_sum(sum, wc1, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 64));          mid_sum(sum, wc2, sum_mask, lo_mask);
	sum_mask = _mm256_load_si256((__m256i*)(mask + 96));          mid_sum(sum, wc3, sum_mask, lo_mask);

	return (uint16_t)(
		_mm_extract_epi16(sum, 0) + _mm_extract_epi16(sum, 1) +
		_mm_extract_epi16(sum, 2) + _mm_extract_epi16(sum, 3) +
		_mm_extract_epi16(sum, 4) + _mm_extract_epi16(sum, 5) +
		_mm_extract_epi16(sum, 6) + _mm_extract_epi16(sum, 7));
}

__forceinline uint16_t tm_avx_r256_map_8::_calculate_carnival_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, carnival_world_checksum_mask);
}

__forceinline uint16_t tm_avx_r256_map_8::_calculate_other_world_checksum(WC_ARGS_256)
{
	return masked_checksum(WC_PASS_256, other_world_checksum_mask);
}

__forceinline uint16_t tm_avx_r256_map_8::fetch_checksum_value(WC_ARGS_256, uint8_t code_length)
{
	_store_to_mem(WC_PASS_256);
	uint8_t lo = working_code_data[127 - code_length];
	uint8_t hi = working_code_data[127 - (code_length + 1)];
	return (uint16_t)((hi << 8) | lo);
}

__forceinline uint16_t tm_avx_r256_map_8::_fetch_carnival_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16_t tm_avx_r256_map_8::_fetch_other_world_checksum_value(WC_ARGS_256)
{
	return fetch_checksum_value(WC_PASS_256, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx_r256_map_8::check_carnival_world_checksum(WC_ARGS_256)
{
	return _calculate_carnival_world_checksum(WC_PASS_256)
	    == _fetch_carnival_world_checksum_value(WC_PASS_256);
}

__forceinline bool tm_avx_r256_map_8::check_other_world_checksum(WC_ARGS_256)
{
	return _calculate_other_world_checksum(WC_PASS_256)
	    == _fetch_other_world_checksum_value(WC_PASS_256);
}

// ---------------------------------------------------------------------------
// Bruteforce methods
// ---------------------------------------------------------------------------

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8_t> tm_avx_r256_map_8::_decrypt_check(WC_ARGS_256)
{
	WC_XOR_VARS_256;
	WC_COPY_XOR_256;
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(WC_XOR_PASS_256);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_carnival_world_checksum(WC_XOR_PASS_256))
			{
				return std::nullopt;
			}
		}
	}
	else
	{
		_decrypt_other_world(WC_XOR_PASS_256);

		if constexpr (CHECK_CHECKSUM)
		{
			if (!check_other_world_checksum(WC_XOR_PASS_256))
			{
				return std::nullopt;
			}
		}
	}

	_store_to_mem(WC_XOR_PASS_256);
	return check_machine_code(working_code_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx_r256_map_8::_run_bruteforce(WC_ARGS_256, uint32_t data, uint8_t* result_data, uint32_t* result_size)
{
	_expand_code(data, WC_PASS_256);

	_run_all_maps(WC_PASS_256);

	auto carnival_flags = _decrypt_check<CHECK_CHECKSUMS, CARNIVAL_WORLD>(WC_PASS_256);
	if constexpr (CHECK_CHECKSUMS)
	{
		if (carnival_flags.has_value())
		{
			*((uint32_t*)(&result_data[*result_size])) = data;
			result_data[*result_size + 4] = *carnival_flags;
			*result_size += 5;

			return;
		}
	}

	auto other_flags = _decrypt_check<CHECK_CHECKSUMS, OTHER_WORLD>(WC_PASS_256);
	if constexpr (CHECK_CHECKSUMS)
	{
		if (other_flags.has_value())
		{
			*((uint32_t*)(&result_data[*result_size])) = data;
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

void tm_avx_r256_map_8::run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size)
{
	WC_VARS_256;

	for (uint32_t i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
		{
			return;
		}
		uint32_t data = start_data + i;

		_run_bruteforce<true>(WC_PASS_256, data, result_data, result_size);

		report_progress((double)(i + 1) / amount_to_run);
	}
}

void tm_avx_r256_map_8::compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out)
{
	WC_VARS_256;

	uint8_t result_data[2];
	uint32_t result_pos = 0;

	_run_bruteforce<false>(WC_PASS_256, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

// ---------------------------------------------------------------------------
// Test interface
// ---------------------------------------------------------------------------

void tm_avx_r256_map_8::test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed)
{
	test_algorithm_n(algorithm_id, data, rng_seed, 1);
}

void tm_avx_r256_map_8::test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations)
{
	WC_VARS_256;
	load_data(data);
	_load_from_mem(WC_PASS_256);

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
		rng->generate_alg2_values_for_seeds(&alg2_values_for_seeds_256_8, rng_seed, 1, 256, false);
	}
	else if (algorithm_id == 5)
	{
		rng->generate_alg5_values_for_seeds(&alg5_values_for_seeds_256_8, rng_seed, 1, 256, false);
	}
	else if (algorithm_id == 6)
	{
		rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, rng_seed, 1);
	}

	uint16_t local_pos = 2047;
	for (int i = 0; i < iterations; i++)
	{
		if (local_pos < 128)
		{
			local_pos = 2047;
		}
		_run_alg(WC_PASS_256, algorithm_id, &local_pos,
			regular_rng_values_for_seeds_8, alg0_values_for_seeds_8,
			alg2_values_for_seeds_256_8, alg5_values_for_seeds_256_8,
			alg6_values_for_seeds_8);
	}

	_store_to_mem(WC_PASS_256);
	fetch_data(data);
}

void tm_avx_r256_map_8::test_expansion(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

void tm_avx_r256_map_8::test_bruteforce_data(uint32_t data, uint8_t* result_out)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_run_all_maps(WC_PASS_256);
	_store_to_mem(WC_PASS_256);
	fetch_data(result_out);
}

bool tm_avx_r256_map_8::test_bruteforce_checksum(uint32_t data, int world)
{
	WC_VARS_256;
	_expand_code(data, WC_PASS_256);
	_run_all_maps(WC_PASS_256);

	if (world == CARNIVAL_WORLD)
	{
		return _decrypt_check<true, CARNIVAL_WORLD>(WC_PASS_256).has_value();
	}
	else
	{
		return _decrypt_check<true, OTHER_WORLD>(WC_PASS_256).has_value();
	}
}

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------

bool tm_avx_r256_map_8::initialized = false;

alignas(32) const __m256i tm_avx_r256_map_8::mask_FF     = _mm256_set1_epi8(static_cast<int8_t>(0xFF));
alignas(32) const __m256i tm_avx_r256_map_8::mask_FE     = _mm256_set1_epi8(static_cast<int8_t>(0xFE));
alignas(32) const __m256i tm_avx_r256_map_8::mask_7F     = _mm256_set1_epi8(static_cast<int8_t>(0x7F));
alignas(32) const __m256i tm_avx_r256_map_8::mask_top_01 = _mm256_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
alignas(32) const __m256i tm_avx_r256_map_8::mask_top_80 = _mm256_set_epi16(static_cast<int16_t>(0x8000), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
// alg_2_sub masks
alignas(32) const __m256i tm_avx_r256_map_8::mask_alg2   = _mm256_set_epi16(0, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100);
alignas(32) const __m256i tm_avx_r256_map_8::mask_007F   = _mm256_set1_epi16(0x007F);
alignas(32) const __m256i tm_avx_r256_map_8::mask_FE00   = _mm256_set1_epi16(static_cast<int16_t>(0xFE00));
alignas(32) const __m256i tm_avx_r256_map_8::mask_0080   = _mm256_set1_epi16(0x0080);
// alg_5_sub masks
alignas(32) const __m256i tm_avx_r256_map_8::mask_alg5   = _mm256_set_epi16(0, static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000));
alignas(32) const __m256i tm_avx_r256_map_8::mask_7F00   = _mm256_set1_epi16(static_cast<int16_t>(0x7F00));
alignas(32) const __m256i tm_avx_r256_map_8::mask_00FE   = _mm256_set1_epi16(0x00FE);
alignas(32) const __m256i tm_avx_r256_map_8::mask_0001   = _mm256_set1_epi16(0x0001);
