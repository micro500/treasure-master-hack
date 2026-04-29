#ifndef TM_AVX_R256_MAP_8_H
#define TM_AVX_R256_MAP_8_H
#include <optional>
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

#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

// Natural (non-shuffled) memory layout with per-schedule precomputed RNG tables.
// Uses 4×__m256i registers: wc0=bytes[0..31], wc1=bytes[32..63],
// wc2=bytes[64..95], wc3=bytes[96..127].
//
// Per-schedule tables (built by generate_map_rng):
//   regular_rng_values_for_seeds_8  — used by alg 1/3/4.
//   alg0_values_for_seeds_8         — used by alg 0.
//   alg2_values_for_seeds_256_8     — 32-byte carry per position; used by alg 2.
//   alg5_values_for_seeds_256_8     — 32-byte carry per position; used by alg 5.
//   alg6_values_for_seeds_8         — used by alg 6.
//
// AVX1 (not AVX2) integer operations: uses cast-to-pd trick for bitwise ops,
// extracts/inserts 128-bit halves for shifts.

class tm_avx_r256_map_8 : public TM_base
{
public:
	tm_avx_r256_map_8(RNG* rng);
	tm_avx_r256_map_8(RNG* rng, uint32_t key);
	tm_avx_r256_map_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	~tm_avx_r256_map_8();

	void test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed);
	void test_expansion(uint32_t data, uint8_t* result_out);
	void test_bruteforce_data(uint32_t data, uint8_t* result_out);
	bool test_bruteforce_checksum(uint32_t data, int world);
	void test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations);
	void run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size);
	void compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out);

//private:
	void generate_map_rng();
	void initialize();
	void load_data(uint8_t* new_data);
	void fetch_data(uint8_t* new_data);

	void alg_2_sub(__m256i& wc, __m256i& carry);
	void alg_5_sub(__m256i& wc, __m256i& carry);

	void alg_0(WC_ARGS_256, const uint8_t* block_start);
	void alg_1(WC_ARGS_256, const uint8_t* block_start);
	void alg_2(WC_ARGS_256, const uint8_t* carry_ptr);
	void alg_3(WC_ARGS_256, const uint8_t* block_start);
	void alg_4(WC_ARGS_256, const uint8_t* block_start);
	void alg_5(WC_ARGS_256, const uint8_t* carry_ptr);
	void alg_6(WC_ARGS_256, const uint8_t* block_start);
	void alg_7(WC_ARGS_256);

	void _run_one_map(WC_ARGS_256, int map_idx);
	void _run_alg(WC_ARGS_256, int algorithm_id, uint16_t* local_pos,
	              const uint8_t* reg_base, const uint8_t* alg0_base,
	              const uint8_t* alg2_base, const uint8_t* alg5_base,
	              const uint8_t* alg6_base);

	template<bool CHECK_CHECKSUM, int WORLD>
	std::optional<uint8_t> _decrypt_check(WC_ARGS_256);

	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(WC_ARGS_256, uint32_t data, uint8_t* result_data, uint32_t* result_size);

	void _load_from_mem(WC_ARGS_256);
	void _store_to_mem(WC_ARGS_256);
	void _expand_code(uint32_t data, WC_ARGS_256);
	void _run_all_maps(WC_ARGS_256);

	void _decrypt_carnival_world(WC_ARGS_256);
	void _decrypt_other_world(WC_ARGS_256);

	uint16_t _calculate_carnival_world_checksum(WC_ARGS_256);
	uint16_t _calculate_other_world_checksum(WC_ARGS_256);

	uint16_t _fetch_carnival_world_checksum_value(WC_ARGS_256);
	uint16_t _fetch_other_world_checksum_value(WC_ARGS_256);
	bool check_carnival_world_checksum(WC_ARGS_256);
	bool check_other_world_checksum(WC_ARGS_256);

	uint16_t masked_checksum(WC_ARGS_256, uint8_t* mask);
	void mid_sum(__m128i& sum, __m256i& working_code, __m256i& sum_mask, __m128i& lo_mask);
	uint16_t fetch_checksum_value(WC_ARGS_256, uint8_t code_length);
	void xor_alg(WC_ARGS_256, uint8_t* values);

	AlignedPtr<uint8_t> expansion_values_for_seed_128_8;
	AlignedPtr<uint8_t> regular_rng_values_for_seeds_8;
	AlignedPtr<uint8_t> alg0_values_for_seeds_8;
	AlignedPtr<uint8_t> alg2_values_for_seeds_256_8;
	AlignedPtr<uint8_t> alg5_values_for_seeds_256_8;
	AlignedPtr<uint8_t> alg6_values_for_seeds_8;

	bool _initialized = false;
	std::vector<std::shared_ptr<void>> _table_refs;

	const alignas(32) __m256i mask_FF;
	const alignas(32) __m256i mask_FE;
	const alignas(32) __m256i mask_7F;
	const alignas(32) __m256i mask_top_01;
	const alignas(32) __m256i mask_top_80;
	// alg_2_sub masks
	const alignas(32) __m256i mask_alg2;
	const alignas(32) __m256i mask_007F;
	const alignas(32) __m256i mask_FE00;
	const alignas(32) __m256i mask_0080;
	// alg_5_sub masks
	const alignas(32) __m256i mask_alg5;
	const alignas(32) __m256i mask_7F00;
	const alignas(32) __m256i mask_00FE;
	const alignas(32) __m256i mask_0001;

	ALIGNED(32) uint8_t working_code_data[128];
};
#endif // TM_AVX_R256_MAP_8_H
