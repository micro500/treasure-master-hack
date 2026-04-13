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
#include "tm_avx_r128_map_8.h"

// ---------------------------------------------------------------------------
// Constructors / destructor
// ---------------------------------------------------------------------------

tm_avx_r128_map_8::tm_avx_r128_map_8(RNG* rng_obj) : tm_avx_r128_map_8(rng_obj, 0) {}

tm_avx_r128_map_8::tm_avx_r128_map_8(RNG* rng_obj, const uint32_t key) : tm_avx_r128_map_8(rng_obj, key, key_schedule(key, key_schedule::ALL_MAPS)) {}

tm_avx_r128_map_8::tm_avx_r128_map_8(RNG* rng_obj, const uint32_t key, const key_schedule& schedule_entries) : TM_base(rng_obj)
{
	initialize();
	this->key = key;
	this->schedule_entries = schedule_entries;

	expansion_values_for_seed_128_8 = nullptr;
	regular_rng_values_for_seeds_8 = nullptr;
	alg0_values_for_seeds_8 = nullptr;
	alg2_values_for_seeds_128_8 = nullptr;
	alg5_values_for_seeds_128_8 = nullptr;
	alg6_values_for_seeds_8 = nullptr;

	generate_map_rng();
}

tm_avx_r128_map_8::~tm_avx_r128_map_8()
{
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

void tm_avx_r128_map_8::initialize()
{
	if (!initialized)
	{
		initialized = true;
	}
	obj_name = "tm_avx_r128_map_8";
}

// ---------------------------------------------------------------------------
// Map RNG table generation
// ---------------------------------------------------------------------------

void tm_avx_r128_map_8::generate_map_rng()
{
	rng->_generate_expansion_values_for_seed_8(&expansion_values_for_seed_128_8, (key >> 16) & 0xFFFF, false, 128);
	rng->generate_regular_rng_values_for_seeds_8(&regular_rng_values_for_seeds_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg0_values_for_seeds_8(&alg0_values_for_seeds_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg2_values_for_seeds_128_8(&alg2_values_for_seeds_128_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg5_values_for_seeds_128_8(&alg5_values_for_seeds_128_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
	rng->generate_alg6_values_for_seeds_8(&alg6_values_for_seeds_8, const_cast<uint16*>(schedule_entries->seeds), schedule_entries->entry_count);
}

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::_load_from_mem(WC_ARGS)
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

__forceinline void tm_avx_r128_map_8::_store_to_mem(WC_ARGS)
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

void tm_avx_r128_map_8::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
		working_code_data[i] = new_data[i];
}

void tm_avx_r128_map_8::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
		new_data[i] = working_code_data[i];
}

// ---------------------------------------------------------------------------
// Expand
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::_expand_code(uint32 data, WC_ARGS)
{
	// Build natural 16-byte pattern: [K0,K1,K2,K3,D0,D1,D2,D3] repeated twice
	uint64 x = ((uint64)key << 32) | data;
	__m128i a = _mm_cvtsi64_si128(x);
	__m128i nat_mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7);
	__m128i pattern = _mm_shuffle_epi8(a, nat_mask);

	wc0 = pattern; wc1 = pattern; wc2 = pattern; wc3 = pattern;
	wc4 = pattern; wc5 = pattern; wc6 = pattern; wc7 = pattern;

	uint8* rng_start = expansion_values_for_seed_128_8;
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
// Carry-propagation helpers (natural interleaved layout — same as tm_avx_r128_8)
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::alg_2_sub(__m128i& wc, __m128i& carry)
{
	__m128i part_lo = _mm_and_si128(_mm_srli_epi16(wc, 1), _mm_set1_epi16(0x007F));
	__m128i part_hi_bit = _mm_and_si128(_mm_srli_epi16(wc, 8), _mm_set1_epi16(0x0080));
	__m128i new_lo = _mm_or_si128(part_lo, part_hi_bit);

	__m128i low_bit0 = _mm_and_si128(wc, _mm_set1_epi16(0x0001));
	__m128i carry_in = _mm_or_si128(
		_mm_srli_si128(_mm_slli_epi16(low_bit0, 8), 2),
		carry);

	__m128i new_hi = _mm_or_si128(
		_mm_and_si128(_mm_slli_epi16(wc, 1), _mm_set1_epi16(0xFE00)),
		carry_in);

	carry = _mm_and_si128(
		_mm_slli_si128(_mm_and_si128(wc, _mm_set1_epi16(0x0001)), 15),
		mask_top_01);

	wc = _mm_or_si128(
		_mm_and_si128(new_lo, _mm_set1_epi16(0x00FF)),
		_mm_and_si128(new_hi, _mm_set1_epi16(0xFF00)));
}

__forceinline void tm_avx_r128_map_8::alg_5_sub(__m128i& wc, __m128i& carry)
{
	__m128i part_lo = _mm_and_si128(_mm_slli_epi16(wc, 1), _mm_set1_epi16(0x00FE));
	__m128i part_lo_bit = _mm_and_si128(_mm_srli_epi16(wc, 8), _mm_set1_epi16(0x0001));
	__m128i new_lo = _mm_or_si128(part_lo, part_lo_bit);

	__m128i low_bit7 = _mm_and_si128(wc, _mm_set1_epi16(0x0080));
	__m128i carry_in = _mm_or_si128(
		_mm_srli_si128(_mm_slli_epi16(low_bit7, 8), 2),
		carry);

	__m128i new_hi = _mm_or_si128(
		_mm_and_si128(_mm_srli_epi16(wc, 1), _mm_set1_epi16(0x7F00)),
		carry_in);

	carry = _mm_and_si128(
		_mm_slli_si128(_mm_and_si128(wc, _mm_set1_epi16(0x0080)), 15),
		mask_top_80);

	wc = _mm_or_si128(
		_mm_and_si128(new_lo, _mm_set1_epi16(0x00FF)),
		_mm_and_si128(new_hi, _mm_set1_epi16(0xFF00)));
}

// ---------------------------------------------------------------------------
// Per-algorithm operations
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::alg_0(WC_ARGS, const uint8* block_start)
{
	wc0 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc0, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start)));
	wc1 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc1, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start + 16)));
	wc2 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc2, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start + 32)));
	wc3 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc3, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start + 48)));
	wc4 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc4, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start + 64)));
	wc5 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc5, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start + 80)));
	wc6 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc6, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start + 96)));
	wc7 = _mm_or_si128(_mm_and_si128(_mm_slli_epi16(wc7, 1), mask_FE), _mm_loadu_si128((const __m128i*)(block_start + 112)));
}

__forceinline void tm_avx_r128_map_8::alg_1(WC_ARGS, const uint8* block_start)
{
	wc0 = _mm_add_epi8(wc0, _mm_loadu_si128((const __m128i*)(block_start)));
	wc1 = _mm_add_epi8(wc1, _mm_loadu_si128((const __m128i*)(block_start + 16)));
	wc2 = _mm_add_epi8(wc2, _mm_loadu_si128((const __m128i*)(block_start + 32)));
	wc3 = _mm_add_epi8(wc3, _mm_loadu_si128((const __m128i*)(block_start + 48)));
	wc4 = _mm_add_epi8(wc4, _mm_loadu_si128((const __m128i*)(block_start + 64)));
	wc5 = _mm_add_epi8(wc5, _mm_loadu_si128((const __m128i*)(block_start + 80)));
	wc6 = _mm_add_epi8(wc6, _mm_loadu_si128((const __m128i*)(block_start + 96)));
	wc7 = _mm_add_epi8(wc7, _mm_loadu_si128((const __m128i*)(block_start + 112)));
}

__forceinline void tm_avx_r128_map_8::alg_2(WC_ARGS, const uint8* carry_ptr)
{
	// alg2_values_for_seeds_128_8 stores carry at byte 15 (compatible with mask_top_01)
	__m128i carry = _mm_loadu_si128((const __m128i*)carry_ptr);
	alg_2_sub(wc7, carry);
	alg_2_sub(wc6, carry);
	alg_2_sub(wc5, carry);
	alg_2_sub(wc4, carry);
	alg_2_sub(wc3, carry);
	alg_2_sub(wc2, carry);
	alg_2_sub(wc1, carry);
	alg_2_sub(wc0, carry);
}

__forceinline void tm_avx_r128_map_8::alg_3(WC_ARGS, const uint8* block_start)
{
	wc0 = _mm_xor_si128(wc0, _mm_loadu_si128((const __m128i*)(block_start)));
	wc1 = _mm_xor_si128(wc1, _mm_loadu_si128((const __m128i*)(block_start + 16)));
	wc2 = _mm_xor_si128(wc2, _mm_loadu_si128((const __m128i*)(block_start + 32)));
	wc3 = _mm_xor_si128(wc3, _mm_loadu_si128((const __m128i*)(block_start + 48)));
	wc4 = _mm_xor_si128(wc4, _mm_loadu_si128((const __m128i*)(block_start + 64)));
	wc5 = _mm_xor_si128(wc5, _mm_loadu_si128((const __m128i*)(block_start + 80)));
	wc6 = _mm_xor_si128(wc6, _mm_loadu_si128((const __m128i*)(block_start + 96)));
	wc7 = _mm_xor_si128(wc7, _mm_loadu_si128((const __m128i*)(block_start + 112)));
}

__forceinline void tm_avx_r128_map_8::alg_4(WC_ARGS, const uint8* block_start)
{
	wc0 = _mm_sub_epi8(wc0, _mm_loadu_si128((const __m128i*)(block_start)));
	wc1 = _mm_sub_epi8(wc1, _mm_loadu_si128((const __m128i*)(block_start + 16)));
	wc2 = _mm_sub_epi8(wc2, _mm_loadu_si128((const __m128i*)(block_start + 32)));
	wc3 = _mm_sub_epi8(wc3, _mm_loadu_si128((const __m128i*)(block_start + 48)));
	wc4 = _mm_sub_epi8(wc4, _mm_loadu_si128((const __m128i*)(block_start + 64)));
	wc5 = _mm_sub_epi8(wc5, _mm_loadu_si128((const __m128i*)(block_start + 80)));
	wc6 = _mm_sub_epi8(wc6, _mm_loadu_si128((const __m128i*)(block_start + 96)));
	wc7 = _mm_sub_epi8(wc7, _mm_loadu_si128((const __m128i*)(block_start + 112)));
}

__forceinline void tm_avx_r128_map_8::alg_5(WC_ARGS, const uint8* carry_ptr)
{
	__m128i carry = _mm_loadu_si128((const __m128i*)carry_ptr);
	alg_5_sub(wc7, carry);
	alg_5_sub(wc6, carry);
	alg_5_sub(wc5, carry);
	alg_5_sub(wc4, carry);
	alg_5_sub(wc3, carry);
	alg_5_sub(wc2, carry);
	alg_5_sub(wc1, carry);
	alg_5_sub(wc0, carry);
}

__forceinline void tm_avx_r128_map_8::alg_6(WC_ARGS, const uint8* block_start)
{
	wc0 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc0, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start)));
	wc1 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc1, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start + 16)));
	wc2 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc2, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start + 32)));
	wc3 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc3, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start + 48)));
	wc4 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc4, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start + 64)));
	wc5 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc5, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start + 80)));
	wc6 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc6, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start + 96)));
	wc7 = _mm_or_si128(_mm_and_si128(_mm_srli_epi16(wc7, 1), mask_7F), _mm_loadu_si128((const __m128i*)(block_start + 112)));
}

__forceinline void tm_avx_r128_map_8::alg_7(WC_ARGS)
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
// xor_alg — used by decrypt
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::xor_alg(WC_ARGS, uint8* values)
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

// ---------------------------------------------------------------------------
// _run_alg — dispatches to the appropriate algorithm
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::_run_alg(WC_ARGS, int algorithm_id, uint16* local_pos,
	const uint8* reg_base, const uint8* alg0_base,
	const uint8* alg2_base, const uint8* alg5_base,
	const uint8* alg6_base)
{
	if (algorithm_id == 0)
	{
		alg_0(WC_PASS, alg0_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 1)
	{
		alg_1(WC_PASS, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 2)
	{
		alg_2(WC_PASS, alg2_base + *local_pos * 16);
		*local_pos -= 1;
	}
	else if (algorithm_id == 3)
	{
		alg_3(WC_PASS, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 4)
	{
		alg_4(WC_PASS, reg_base + *local_pos - 127);
		*local_pos -= 128;
	}
	else if (algorithm_id == 5)
	{
		alg_5(WC_PASS, alg5_base + *local_pos * 16);
		*local_pos -= 1;
	}
	else if (algorithm_id == 6)
	{
		alg_6(WC_PASS, alg6_base + (2047 - *local_pos));
		*local_pos -= 128;
	}
	else if (algorithm_id == 7)
	{
		alg_7(WC_PASS);
	}
}

// ---------------------------------------------------------------------------
// _run_one_map / _run_all_maps
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::_run_one_map(WC_ARGS, int map_idx)
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
		// Natural layout: byte i (0..15) is in wc0.
		_mm_store_si128((__m128i*)(working_code_data), wc0);

		unsigned char nibble = (nibble_selector >> 15) & 0x01;
		nibble_selector <<= 1;

		unsigned char current_byte = working_code_data[i];
		if (nibble == 1)
			current_byte >>= 4;
		unsigned char algorithm_id = (current_byte >> 1) & 0x07;

		_run_alg(WC_PASS, algorithm_id, &local_pos, reg_base, alg0_base, alg2_base, alg5_base, alg6_base);
	}
}

__forceinline void tm_avx_r128_map_8::_run_all_maps(WC_ARGS)
{
	for (int map_idx = 0; map_idx < schedule_entries->entry_count; map_idx++)
	{
		_run_one_map(WC_PASS, map_idx);
	}
}

// ---------------------------------------------------------------------------
// Decrypt helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::_decrypt_carnival_world(WC_ARGS)
{
	xor_alg(WC_PASS, carnival_world_data);
}

__forceinline void tm_avx_r128_map_8::_decrypt_other_world(WC_ARGS)
{
	xor_alg(WC_PASS, other_world_data);
}

// ---------------------------------------------------------------------------
// Checksum helpers
// ---------------------------------------------------------------------------

__forceinline void tm_avx_r128_map_8::mid_sum(
	__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask)
{
	__m128i temp_masked = _mm_and_si128(working_code, sum_mask);
	__m128i temp1_lo_lo = _mm_and_si128(temp_masked, lo_mask);
	__m128i temp1_lo_hi = _mm_srli_epi16(temp_masked, 8);
	sum = _mm_add_epi16(sum, temp1_lo_lo);
	sum = _mm_add_epi16(sum, temp1_lo_hi);
}

__forceinline uint16 tm_avx_r128_map_8::masked_checksum(WC_ARGS, uint8* mask)
{
	__m128i sum = _mm_setzero_si128();
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

__forceinline uint16 tm_avx_r128_map_8::_calculate_carnival_world_checksum(WC_ARGS)
{
	return masked_checksum(WC_PASS, carnival_world_checksum_mask);
}

__forceinline uint16 tm_avx_r128_map_8::_calculate_other_world_checksum(WC_ARGS)
{
	return masked_checksum(WC_PASS, other_world_checksum_mask);
}

__forceinline uint16 tm_avx_r128_map_8::fetch_checksum_value(WC_ARGS, uint8 code_length)
{
	_store_to_mem(WC_PASS);
	unsigned char lo = working_code_data[127 - code_length];
	unsigned char hi = working_code_data[127 - (code_length + 1)];
	return (uint16)((hi << 8) | lo);
}

__forceinline uint16 tm_avx_r128_map_8::_fetch_carnival_world_checksum_value(WC_ARGS)
{
	return fetch_checksum_value(WC_PASS, CARNIVAL_WORLD_CODE_LENGTH - 2);
}

__forceinline uint16 tm_avx_r128_map_8::_fetch_other_world_checksum_value(WC_ARGS)
{
	return fetch_checksum_value(WC_PASS, OTHER_WORLD_CODE_LENGTH - 2);
}

__forceinline bool tm_avx_r128_map_8::check_carnival_world_checksum(WC_ARGS)
{
	return _calculate_carnival_world_checksum(WC_PASS)
	    == _fetch_carnival_world_checksum_value(WC_PASS);
}

__forceinline bool tm_avx_r128_map_8::check_other_world_checksum(WC_ARGS)
{
	return _calculate_other_world_checksum(WC_PASS)
	    == _fetch_other_world_checksum_value(WC_PASS);
}

// ---------------------------------------------------------------------------
// Bruteforce methods
// ---------------------------------------------------------------------------

template<bool CHECK_CHECKSUM, int WORLD>
std::optional<uint8> tm_avx_r128_map_8::_decrypt_check(WC_ARGS)
{
	WCXOR_VARS;
	WCXOR_COPY;
	if constexpr (WORLD == CARNIVAL_WORLD)
	{
		_decrypt_carnival_world(WCXOR_PASS);

		if constexpr (CHECK_CHECKSUM)
		{
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
	return check_machine_code(working_code_data, WORLD);
}

template<bool CHECK_CHECKSUMS>
__forceinline void tm_avx_r128_map_8::_run_bruteforce(WC_ARGS, uint32 data, uint8* result_data, uint32* result_size)
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

void tm_avx_r128_map_8::run_bruteforce_boinc(uint32 start_data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size)
{
	WC_VARS;

	for (uint32 i = 0; i < amount_to_run; i++)
	{
		if ((result_max_size - *result_size) < 5)
		{
			return;
		}
		uint32 data = start_data + i;

		_run_bruteforce<true>(WC_PASS, data, result_data, result_size);

		report_progress((double)(i + 1) / amount_to_run);
	}
}

void tm_avx_r128_map_8::compute_challenge_flags(uint32 data, uint8& carnival_flags_out, uint8& other_flags_out)
{
	WC_VARS;

	uint8 result_data[2];
	uint32 result_pos = 0;

	_run_bruteforce<false>(WC_PASS, data, result_data, &result_pos);

	carnival_flags_out = result_data[0];
	other_flags_out = result_data[1];
}

// ---------------------------------------------------------------------------
// Test interface
// ---------------------------------------------------------------------------

void tm_avx_r128_map_8::test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed)
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

	uint16 local_pos = 2047;
	_run_alg(WC_PASS, algorithm_id, &local_pos,
	         regular_rng_values_for_seeds_8, alg0_values_for_seeds_8,
	         alg2_values_for_seeds_128_8, alg5_values_for_seeds_128_8,
	         alg6_values_for_seeds_8);

	_store_to_mem(WC_PASS);
	fetch_data(data);
}

void tm_avx_r128_map_8::test_expansion(uint32_t data, uint8* result_out)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_store_to_mem(WC_PASS);
	fetch_data(result_out);
}

void tm_avx_r128_map_8::test_bruteforce_data(uint32 data, uint8* result_out)
{
	WC_VARS;
	_expand_code(data, WC_PASS);
	_run_all_maps(WC_PASS);
	_store_to_mem(WC_PASS);
	fetch_data(result_out);
}

bool tm_avx_r128_map_8::test_bruteforce_checksum(uint32 data, int world)
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

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------

bool tm_avx_r128_map_8::initialized = false;

alignas(16) const __m128i tm_avx_r128_map_8::mask_FF = _mm_set1_epi16(0xFFFF);
alignas(16) const __m128i tm_avx_r128_map_8::mask_FE = _mm_set1_epi16(0xFEFE);
alignas(16) const __m128i tm_avx_r128_map_8::mask_7F = _mm_set1_epi16(0x7F7F);
alignas(16) const __m128i tm_avx_r128_map_8::mask_80 = _mm_set1_epi16(0x8080);
alignas(16) const __m128i tm_avx_r128_map_8::mask_01 = _mm_set1_epi16(0x0101);
alignas(16) const __m128i tm_avx_r128_map_8::mask_top_01 = _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0);
alignas(16) const __m128i tm_avx_r128_map_8::mask_top_80 = _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0);
