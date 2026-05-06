#ifndef TM_AVX2_M256_8_H
#define TM_AVX2_M256_8_H
#include <optional>
#include "simd.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

// Natural (non-shuffled) AVX2 256-bit memory-based implementation (32-bit only).
// Working data kept in memory (working_code_data), accessed via _load_si256/_store_si256.
// wc0 = bytes[0..31], wc1 = bytes[32..63], wc2 = bytes[64..95], wc3 = bytes[96..127].
// Uses non-shuffled RNG tables (_8 suffix, stride = seed*128).
// Carry seeds: alg2_values_8_8, alg5_values_8_8 (1 byte per seed, same as tm_8 / tm_ssse3_m128_8).

class tm_avx2_m256_8 : public TM_base
{
public:
	tm_avx2_m256_8(RNG* rng);
	tm_avx2_m256_8(RNG* rng, uint32_t key);
	tm_avx2_m256_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	void test_algorithm_chain(const uint8_t* algorithm_ids, int chain_length,
	                          uint8_t* data, uint16_t* rng_seed) override;
	void test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations) override;
	void test_expansion(uint32_t data, uint8_t* result_out) override;
	void test_bruteforce_data(uint32_t data, uint8_t* result_out) override;
	bool test_bruteforce_checksum(uint32_t data, int world) override;
	void run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size) override;
	void compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out) override;

private:
	void initialize();
	void load_data(uint8_t* new_data);
	void fetch_data(uint8_t* new_data);

	void alg_0(uint16_t* rng_seed);
	void alg_2(uint16_t* rng_seed);
	void alg_3(uint16_t* rng_seed);
	void alg_5(uint16_t* rng_seed);
	void alg_6(uint16_t* rng_seed);
	void alg_7();
	void xor_alg(uint8_t* values);
	void add_alg(uint16_t* rng_seed, uint8_t* rng_start);
	void alg_2_sub(__m256i& cur_val, __m256i& carry);
	void alg_5_sub(__m256i& cur_val, __m256i& carry);

	void _expand_code(uint32_t data);
	void _run_alg(int algorithm_id, uint16_t* rng_seed);
	void _run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void _run_all_maps();

	template<bool CHECK_CHECKSUM, int WORLD>
	std::optional<uint8_t> _decrypt_check();

	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(uint32_t data, uint8_t* result_data, uint32_t* result_size);

	void _decrypt_carnival_world();
	void _decrypt_other_world();

	uint16_t _calculate_carnival_world_checksum();
	uint16_t _calculate_other_world_checksum();
	uint16_t _fetch_carnival_world_checksum_value();
	uint16_t _fetch_other_world_checksum_value();
	bool check_carnival_world_checksum();
	bool check_other_world_checksum();

	uint16_t masked_checksum(uint8_t* mask);
	void mid_sum(__m128i& sum, __m256i& working_code, __m256i& sum_mask);

	ALIGNED(32) uint8_t working_code_data[128];
	const alignas(32) __m256i mask_FF;
	const alignas(32) __m256i mask_FE;
	const alignas(32) __m256i mask_7F;
	const alignas(32) __m256i mask_80;
	const alignas(32) __m256i mask_01;
	const alignas(32) __m256i mask_top_01;
	const alignas(32) __m256i mask_top_80;
	const alignas(32) __m256i mask_0001;
	const alignas(32) __m256i mask_0080;
	const alignas(32) __m256i mask_007F;
	const alignas(32) __m256i mask_00FE;
	const alignas(32) __m256i mask_00FF;
	const alignas(32) __m256i mask_FF00;
	const alignas(32) __m256i mask_FE00;
	const alignas(32) __m256i mask_7F00;

	bool _initialized = false;
	std::vector<std::shared_ptr<void>> _table_refs;
	uint8_t* _expansion_8 = nullptr;
	uint16_t* _seed_fwd_1 = nullptr;
	uint16_t* _seed_fwd_128 = nullptr;
	uint8_t* _regular_8 = nullptr;
	uint8_t* _alg0_8 = nullptr;
	uint8_t* _alg2_8_8 = nullptr;
	uint8_t* _alg4_8 = nullptr;
	uint8_t* _alg5_8_8 = nullptr;
	uint8_t* _alg6_8 = nullptr;
};
#endif // TM_AVX2_M256_8_H
