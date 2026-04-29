#ifndef TM_SSSE3_R128_MAP_8_H
#define TM_SSSE3_R128_MAP_8_H
#include <optional>
#include "simd.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

// Like tm_ssse3_r128_8 but uses per-schedule precomputed RNG tables
// (built by generate_map_rng) instead of global per-seed tables.
//
// Per-schedule precomputed RNG tables (built by generate_map_rng):
//   regular_rng_values_for_seeds_8  — raw RNG bytes; used by alg 1/3/4.
//   alg0_values_for_seeds_8         — bit 7 extracted as bit 0; used by alg 0.
//   alg2_values_for_seeds_128_8     — 16-byte carry per position; used by alg 2.
//   alg5_values_for_seeds_128_8     — 16-byte carry per position; used by alg 5.
//   alg6_values_for_seeds_8         — bit 7 masked (& 0x80); used by alg 6.
//
// Expansion: single per-key 128-byte non-shuffled buffer (expansion_values_for_seed_128_8).
// No global per-seed static tables needed.

class tm_ssse3_r128_map_8 : public TM_base
{
public:
	tm_ssse3_r128_map_8(RNG* rng);
	tm_ssse3_r128_map_8(RNG* rng, uint32_t key);
	tm_ssse3_r128_map_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	~tm_ssse3_r128_map_8();
	void test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations) override;
	void test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed) override;
	void test_expansion(uint32_t data, uint8_t* result_out) override;
	void test_bruteforce_data(uint32_t data, uint8_t* result_out) override;
	bool test_bruteforce_checksum(uint32_t data, int world) override;
	void run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size) override;
	void compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out) override;

private:
	void generate_map_rng();
	void initialize();
	void load_data(uint8_t* new_data);
	void fetch_data(uint8_t* new_data);

	void alg_2_sub(__m128i& wc, __m128i& carry);
	void alg_5_sub(__m128i& wc, __m128i& carry);

	void alg_0(WC_ARGS_128, const uint8_t* block_start);
	void alg_1(WC_ARGS_128, const uint8_t* block_start);
	void alg_2(WC_ARGS_128, uint8_t carry_byte);
	void alg_3(WC_ARGS_128, const uint8_t* block_start);
	void alg_4(WC_ARGS_128, const uint8_t* block_start);
	void alg_5(WC_ARGS_128, uint8_t carry_byte);
	void alg_6(WC_ARGS_128, const uint8_t* block_start);
	void alg_7(WC_ARGS_128);

	void _run_one_map(WC_ARGS_128, int map_idx);
	void _run_alg(WC_ARGS_128, int algorithm_id, uint16_t* local_pos,
	              const uint8_t* reg_base, const uint8_t* alg0_base,
	              const uint8_t* alg6_base);

	template<bool CHECK_CHECKSUM, int WORLD>
	std::optional<uint8_t> _decrypt_check(WC_ARGS_128);

	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(WC_ARGS_128, uint32_t data, uint8_t* result_data, uint32_t* result_size);

	void _load_from_mem(WC_ARGS_128);
	void _store_to_mem(WC_ARGS_128);
	void _expand_code(uint32_t data, WC_ARGS_128);
	void _run_all_maps(WC_ARGS_128);

	void _decrypt_carnival_world(WC_ARGS_128);
	void _decrypt_other_world(WC_ARGS_128);

	uint16_t _calculate_carnival_world_checksum(WC_ARGS_128);
	uint16_t _calculate_other_world_checksum(WC_ARGS_128);

	uint16_t _fetch_carnival_world_checksum_value(WC_ARGS_128);
	uint16_t _fetch_other_world_checksum_value(WC_ARGS_128);
	bool check_carnival_world_checksum(WC_ARGS_128);
	bool check_other_world_checksum(WC_ARGS_128);

	uint16_t masked_checksum(WC_ARGS_128, uint8_t* mask);
	void mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask);
	uint16_t fetch_checksum_value(WC_ARGS_128, uint8_t code_length);
	void xor_alg(WC_ARGS_128, uint8_t* values);

	AlignedPtr<uint8_t> expansion_values_for_seed_128_8;
	AlignedPtr<uint8_t> regular_rng_values_for_seeds_8;
	AlignedPtr<uint8_t> alg0_values_for_seeds_8;
	AlignedPtr<uint8_t> alg6_values_for_seeds_8;

	bool _initialized = false;
	std::vector<std::shared_ptr<void>> _table_refs;

	const alignas(16) __m128i mask_FF;
	const alignas(16) __m128i mask_FE;
	const alignas(16) __m128i mask_7F;
	const alignas(16) __m128i mask_80;
	const alignas(16) __m128i mask_01;
	const alignas(16) __m128i mask_top_01;
	const alignas(16) __m128i mask_top_80;

	ALIGNED(32) uint8_t working_code_data[128];
};
#endif // TM_SSSE3_R128_MAP_8_H
