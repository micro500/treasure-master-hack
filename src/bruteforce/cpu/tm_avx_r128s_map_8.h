#ifndef TM_AVX_R128S_MAP_8_H
#define TM_AVX_R128S_MAP_8_H
#include <optional>
#include "simd.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

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

class tm_avx_r128s_map_8 : public TM_base
{
public:
	tm_avx_r128s_map_8(RNG* rng);
	tm_avx_r128s_map_8(RNG* rng, uint32_t key);
	tm_avx_r128s_map_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	~tm_avx_r128s_map_8();

	void test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed) override;
	void test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations) override;
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

	void alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry);
	void alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry);

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

	void _load_fwd(const uint8_t* block_start, WC_ARGS_128);
	void _load_rev(const uint8_t* block_start, WC_ARGS_128);

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
	void add_alg_shuffled(WC_ARGS_128, uint16_t* rng_seed, uint8_t* rng_start);

	AlignedPtr<uint8_t> expansion_values_for_seed_128_8_shuffled;
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
	const alignas(16) __m128i sel_even;
	const alignas(16) __m128i sel_odd;

	ALIGNED(32) uint8_t working_code_data[128];
};
#endif // TM_AVX_R128S_MAP_8_H
