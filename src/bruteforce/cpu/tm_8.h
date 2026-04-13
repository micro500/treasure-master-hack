#ifndef TM_8_H
#define TM_8_H
#include "data_sizes.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_8: public TM_base
{
public:
	tm_8(RNG *rng);	
	tm_8(RNG* rng, uint32_t key);
	tm_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	void test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed);
	void test_expansion(uint32_t data, uint8* result_out);
	void test_bruteforce_data(uint32 data, uint8* result_out);
	bool test_bruteforce_checksum(uint32 data, int world);

	void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	void _expand(uint32 data);

	void _run_all_maps();

	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(uint32 data, uint8* result_data, uint32* result_size);

	void decrypt_carnival_world();
	void decrypt_other_world();
	uint16 calculate_carnival_world_checksum();
	uint16 calculate_other_world_checksum();
	uint16 fetch_carnival_world_checksum_value();
	uint16 fetch_other_world_checksum_value();

	void run_bruteforce_data(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);

	template<bool CHECK_CHECKSUMS, int WORLD>
	std::optional<uint8> _decrypt_check();

private:
	void _run_alg(int algorithm_id, uint16* rng_seed);
	void _run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void initialize();

	void add_alg(uint8* addition_values, const uint16 rng_seed);
	void xor_alg(uint8* working_data, uint8* xor_values);
	void alg_0(const uint16 rng_seed);
	void alg_1(const uint16 rng_seed);
	void alg_2(const uint16 rng_seed);
	void alg_3(const uint16 rng_seed);
	void alg_4(const uint16 rng_seed);
	void alg_5(const uint16 rng_seed);
	void alg_6(const uint16 rng_seed);
	void alg_7();

	uint16 calculate_masked_checksum(uint8* working_data, uint8* mask);
	uint16 fetch_checksum_value(uint8* working_data, int code_length);

	uint16 _calculate_carnival_world_checksum(uint8* working_data);
	uint16 _calculate_other_world_checksum(uint8* working_data);
	bool check_carnival_world_checksum(uint8* working_data);
	bool check_other_world_checksum(uint8* working_data);
	uint16 _fetch_carnival_world_checksum_value(uint8* working_data);
	uint16 _fetch_other_world_checksum_value(uint8* working_data);

	void _decrypt_carnival_world(uint8* working_data);
	void _decrypt_other_world(uint8* working_data);

	uint8 working_code_data[128 * 2];

	uint8 decrypted_data[128];

	static bool initialized;
};

#endif // TM_8_H