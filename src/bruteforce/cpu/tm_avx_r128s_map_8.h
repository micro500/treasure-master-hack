#ifndef TM_AVX_R128S_MAP_8_H
#define TM_AVX_R128S_MAP_8_H
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
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

#define WC_ARGS __m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3, __m128i& wc4, __m128i& wc5, __m128i& wc6, __m128i& wc7

#define WC_PASS wc0, wc1, wc2, wc3, wc4, wc5, wc6, wc7
#define WCXOR_PASS wc_xor0, wc_xor1, wc_xor2, wc_xor3, wc_xor4, wc_xor5, wc_xor6, wc_xor7

#define WC_VARS __m128i WC_PASS
#define WCXOR_VARS __m128i WCXOR_PASS

#define WCXOR_COPY wc_xor0 = wc0; wc_xor1 = wc1; wc_xor2 = wc2; wc_xor3 = wc3; wc_xor4 = wc4; wc_xor5 = wc5; wc_xor6 = wc6; wc_xor7 = wc7;

// Like tm_avx_r128s_8 but the hot path uses per-schedule precomputed RNG tables
// from the RNG object instead of large precomputed per-seed tables.
//
// Three per-schedule tables (built by generate_map_rng via the RNG object):
//   regular_rng_values_for_seeds_8 — raw RNG bytes, reversed storage; used by alg 1/2/3/4/5.
//   alg0_values_for_seeds_8        — bit 7 extracted as bit 0, reversed storage; used by alg 0.
//   alg6_values_for_seeds_8        — bit 7 masked (& 0x80), FORWARD storage; used by alg 6.
//
// Position counter (local_pos):
//   Starts at 2047 per map and DECREASES.
//   After a 128-byte algorithm step: local_pos -= 128.
//   After a 1-byte  algorithm step: local_pos -= 1.
//   For reversed tables: block_start = base + local_pos - 127.
//   For alg6 forward table: block_start = base + (2047 - local_pos).
//
// Call generate_map_rng() once with the key schedule before any bruteforce run.
// The expand step still uses the RNG object's precomputed expansion table.

class tm_avx_r128s_map_8 : public TM_base
{
public:
	tm_avx_r128s_map_8(RNG* rng);
	tm_avx_r128s_map_8(RNG* rng, uint32_t key);
	tm_avx_r128s_map_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	~tm_avx_r128s_map_8();

	void test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed);
	void test_expansion(uint32_t data, uint8* result_out);
	void test_bruteforce_data(uint32 data, uint8* result_out);
	bool test_bruteforce_checksum(uint32 data, int world);

	// Build the raw RNG table from the key schedule.
	void generate_map_rng();

	void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	void expand(uint32 key, uint32 data);

	void decrypt_carnival_world();
	void decrypt_other_world();

	uint16 calculate_carnival_world_checksum();
	uint16 calculate_other_world_checksum();

	void run_all_maps(const key_schedule& schedule_entries);

	void run_bruteforce_boinc(uint32 start_data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);
	void compute_challenge_flags(uint32 data, uint8& carnival_flags_out, uint8& other_flags_out);

	void test_expand_and_map(uint32 key, uint32 data, const key_schedule& schedule, uint8* result_out);
	bool test_pipeline_validate(uint32 key, uint32 data, const key_schedule& schedule, int world);

//private:
	void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void initialize();

	// Carry-propagation helpers (identical to tm_avx_r128s_8).
	void alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry);
	void alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry);

	// Per-algorithm operations. 128-byte algs take block_start + deinterleave masks.
	void alg_0(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd);
	void alg_1(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd);
	void alg_2(WC_ARGS, const uint8* carry_ptr);
	void alg_3(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd);
	void alg_4(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd);
	void alg_5(WC_ARGS, const uint8* carry_ptr);
	void alg_6(WC_ARGS, const uint8* block_start, const __m128i& sel_even, const __m128i& sel_odd);
	void alg_7(WC_ARGS);

	void _run_one_map(WC_ARGS, int map_idx);
	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(WC_ARGS, uint32 data, uint8* result_data, uint32* result_size);

	template<bool CHECK_CHECKSUM, int WORLD>
	std::optional<uint8> _decrypt_check(WC_ARGS);

	// Forward deinterleave: load 128 raw bytes from block_start[0..127] where
	// block_start = map_rng + map_base + local_pos - 127.
	// Logical byte i for alg 0/1/3/4 is at block_start[i], so even bytes go to
	// the even-named working registers and odd bytes to the odd-named ones.
	//   wc0 <- block[0,2,...,14,16,...,30]   wc1 <- block[1,3,...,31]
	//   wc2 <- block[32,34,...,62]            wc3 <- block[33,...,63]
	//   wc4 <- block[64,...,94]               wc5 <- block[65,...,95]
	//   wc6 <- block[96,...,126]              wc7 <- block[97,...,127]
	void _load_fwd(const uint8* block_start,
		WC_ARGS,
	               const __m128i& sel_even, const __m128i& sel_odd);

	// Reverse deinterleave: same block_start pointer as _load_fwd, but loads from
	// the HIGH end of the block first, giving bytes in reverse order.  Used by alg 6
	// which needs RNG outputs in forward generation order.
	//   wc0 <- block[127,125,...,97]   wc1 <- block[126,124,...,96]
	//   wc2 <- block[95,...,65]        wc3 <- block[94,...,64]
	//   wc4 <- block[63,...,33]        wc5 <- block[62,...,32]
	//   wc6 <- block[31,...,1]         wc7 <- block[30,...,0]
	void _load_rev(const uint8* block_start,
		WC_ARGS,
	               const __m128i& sel_odd, const __m128i& sel_even);

	void _load_from_mem(WC_ARGS);
	void _store_to_mem(WC_ARGS);

	void _expand_code(uint32 data, WC_ARGS);

	void _run_all_maps(WC_ARGS);
	void _run_alg(WC_ARGS, int algorithm_id, uint16* local_pos, const uint8* reg_base, const uint8* alg0_base, const uint8* alg2_base, const uint8* alg5_base, const uint8* alg6_base);

	void _decrypt_carnival_world(WC_ARGS);
	void _decrypt_other_world(WC_ARGS);

	uint16 _calculate_carnival_world_checksum(WC_ARGS);
	uint16 _calculate_other_world_checksum(WC_ARGS);

	uint16 _fetch_carnival_world_checksum_value(WC_ARGS);
	uint16 _fetch_other_world_checksum_value(WC_ARGS);

	bool check_carnival_world_checksum(WC_ARGS);
	bool check_other_world_checksum(WC_ARGS);

	uint16 masked_checksum(WC_ARGS, uint8* mask);
	void mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask);
	uint16 fetch_checksum_value(WC_ARGS, uint8 code_length);

	void xor_alg(WC_ARGS, uint8* values);
	// Used only by expand (still indexes the RNG object's precomputed table).
	void add_alg_shuffled(WC_ARGS, uint16* rng_seed, uint8* rng_start);

	uint8_t* expansion_values_for_seed_128_8_shuffled;
	uint8_t* regular_rng_values_for_seeds_8;
	uint8_t* alg0_values_for_seeds_8;
	uint8_t* alg2_values_for_seeds_128_8;
	uint8_t* alg5_values_for_seeds_128_8;
	uint8_t* alg6_values_for_seeds_8;

	static const alignas(16) __m128i mask_FF;
	static const alignas(16) __m128i mask_FE;
	static const alignas(16) __m128i mask_7F;
	static const alignas(16) __m128i mask_80;
	static const alignas(16) __m128i mask_01;
	static const alignas(16) __m128i mask_top_01;
	static const alignas(16) __m128i mask_top_80;
	static const alignas(16) __m128i sel_even;
	static const alignas(16) __m128i sel_odd;

	ALIGNED(32) uint8 working_code_data[128];

	ALIGNED(32) static uint8 carnival_world_checksum_mask_shuffled[128];
	ALIGNED(32) static uint8 carnival_world_data_shuffled[128];
	ALIGNED(32) static uint8 other_world_checksum_mask_shuffled[128];
	ALIGNED(32) static uint8 other_world_data_shuffled[128];

	static bool initialized;
};
#endif // TM_AVX_R128S_MAP_8_H
