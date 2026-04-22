#ifndef TM_SSSE3_M128_8_H
#define TM_SSSE3_M128_8_H
#include <optional>
#include "simd.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_ssse3_m128_8 : public TM_base
{
public:
	tm_ssse3_m128_8(RNG* rng);
	tm_ssse3_m128_8(RNG* rng, uint32_t key);
	tm_ssse3_m128_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	void test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed) override;
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
	void alg_2_sub(__m128i& cur_val, __m128i& carry);
	void alg_5_sub(__m128i& cur_val, __m128i& carry);

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
	void mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask);
	uint16_t fetch_checksum_value(uint8_t code_length);

	ALIGNED(32) uint8_t working_code_data[128];
	const alignas(16) __m128i mask_FF;
	const alignas(16) __m128i mask_FE;
	const alignas(16) __m128i mask_7F;
	const alignas(16) __m128i mask_80;
	const alignas(16) __m128i mask_01;
	const alignas(16) __m128i mask_top_01;
	const alignas(16) __m128i mask_top_80;

	static bool initialized;
};
#endif // TM_SSSE3_M128_8_H
