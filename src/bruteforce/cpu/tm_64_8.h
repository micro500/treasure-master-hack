#ifndef TM_64_8_H
#define TM_64_8_H
#include <optional>
#include "rng_obj.h"
#include "tm_base.h"

class tm_64_8 : public TM_base
{
public:
	tm_64_8(RNG* rng);
	tm_64_8(RNG* rng, uint32_t key);
	tm_64_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	void test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed) override;
	void test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations) override;
	void test_expansion(uint32_t data, uint8_t* result_out) override;
	void test_bruteforce_data(uint32_t data, uint8_t* result_out) override;
	bool test_bruteforce_checksum(uint32_t data, int world) override;
	void run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size) override;
	void compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out) override;

private:
	void initialize();
	void load_data(uint8_t* src);
	void fetch_data(uint8_t* dst);

	void _expand_code(uint32_t data);

	void _run_alg(int algorithm_id, uint16_t* rng_seed);
	void _run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void _run_all_maps();

	template<bool CHECK_CHECKSUM, int WORLD>
	std::optional<uint8_t> _decrypt_check();

	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(uint32_t data, uint8_t* result_data, uint32_t* result_size);

	void _decrypt_carnival_world(uint8_t* out);
	void _decrypt_other_world(uint8_t* out);

	uint16_t _calculate_carnival_world_checksum(uint8_t* working_data);
	uint16_t _calculate_other_world_checksum(uint8_t* working_data);
	uint16_t _fetch_carnival_world_checksum_value(uint8_t* working_data);
	uint16_t _fetch_other_world_checksum_value(uint8_t* working_data);
	bool check_carnival_world_checksum(uint8_t* working_data);
	bool check_other_world_checksum(uint8_t* working_data);

	uint16_t masked_checksum(uint8_t* working_data, uint8_t* mask);
	uint16_t fetch_checksum_value(uint8_t* working_data, uint8_t code_length);

	void add_alg(uint64_t* addition_values_lo, uint64_t* addition_values_hi, const uint16_t rng_seed);
	void xor_alg(uint64_t* working_data, const uint8_t* values);
	void alg_0(const uint16_t rng_seed);
	void alg_2(const uint16_t rng_seed);
	void alg_3(const uint16_t rng_seed);
	void alg_5(const uint16_t rng_seed);
	void alg_6(const uint16_t rng_seed);
	void alg_7();

	uint64_t working_code_data[16];
	uint8_t decrypted_data[128];

	static bool initialized;
};

#endif // TM_64_8_H