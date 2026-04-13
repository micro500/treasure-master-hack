#ifndef TM_32_8_H
#define TM_32_8_H
#include "data_sizes.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_32_8 : public TM_base
{
public:
	tm_32_8(RNG* rng);

	void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	void expand(uint32 key, uint32 data);

	void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	void run_all_maps(const key_schedule& schedule_entries);

	void test_expand_and_map(uint32 key, uint32 data, const key_schedule& schedule, uint8* result_out);
	bool test_pipeline_validate(uint32 key, uint32 data, const key_schedule& schedule, int world);

	void xor_alg(uint32* working_data, const uint8* values);
	uint16 calculate_carnival_world_checksum();
	uint16 calculate_other_world_checksum();
	uint16 fetch_carnival_world_checksum_value();
	uint16 fetch_other_world_checksum_value();

	void _decrypt_carnival_world(uint8* working_data);
	void _decrypt_other_world(uint8* working_data);

	bool check_carnival_world_checksum(uint8* working_data);
	bool check_other_world_checksum(uint8* working_data);

	uint16 calculate_masked_checksum(uint8* working_data, uint8* mask);
	uint16 fetch_checksum_value(uint8* working_data, int code_length);

	uint16 _calculate_carnival_world_checksum(uint8* working_data);
	uint16 _calculate_other_world_checksum(uint8* working_data);
	uint16 _fetch_carnival_world_checksum_value(uint8* working_data);
	uint16 _fetch_other_world_checksum_value(uint8* working_data);


private:
	void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void initialize();

	void add_alg(uint32* addition_values_lo, uint32* addition_values_hi, const uint16 rng_seed);
	void alg_0(const uint16 rng_seed);
	void alg_1(const uint16 rng_seed);
	void alg_2(const uint16 rng_seed);
	void alg_3(const uint16 rng_seed);
	void alg_4(const uint16 rng_seed);
	void alg_5(const uint16 rng_seed);
	void alg_6(const uint16 rng_seed);
	void alg_7();

	uint32 working_code_data[32];

	static bool initialized;
};

#endif // TM_32_8_H