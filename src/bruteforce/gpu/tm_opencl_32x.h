#pragma once
#include "rng_obj.h"
#include "tm_base.h"
#include "opencl.h"

class tm_opencl_32x
{
public:
	tm_opencl_32x(RNG* rng, opencl* _cl);

	void expand(uint32_t key, uint32_t data);

	void fetch_data(uint8* new_data);
	void load_data(uint8* new_data);

	void run_alg(int algorithm_id, uint16* rng_seed, int iterations);
	void run_all_maps(uint32_t key, uint32_t data, const key_schedule& schedule_entries);

	/*
	virtual void expand(uint32 key, uint32 data);

	

	virtual void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);

	virtual void run_all_maps(const key_schedule& schedule_entries);

	virtual void decrypt_carnival_world();
	virtual void decrypt_other_world();
	virtual uint16 calculate_carnival_world_checksum();
	virtual uint16 calculate_other_world_checksum();
	virtual uint16 fetch_carnival_world_checksum_value();
	virtual uint16 fetch_other_world_checksum_value();

	void run_bruteforce_data(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);*/

private:
	void initialize();

	void init_kernel_expand();
	void init_kernel_alg();
	void init_kernel_all_maps();

	//void add_alg(uint8* addition_values, const uint16 rng_seed);
	//void xor_alg(uint8* working_data, uint8* xor_values);
	//void alg_0(const uint16 rng_seed);
	//void alg_1(const uint16 rng_seed);
	//void alg_2(const uint16 rng_seed);
	//void alg_3(const uint16 rng_seed);
	//void alg_4(const uint16 rng_seed);
	//void alg_5(const uint16 rng_seed);
	//void alg_6(const uint16 rng_seed);
	//void alg_7();

	//uint16 calculate_masked_checksum(uint8* working_data, uint8* mask);
	//uint16 fetch_checksum_value(uint8* working_data, int code_length);

	//uint16 _calculate_carnival_world_checksum(uint8* working_data);
	//uint16 _calculate_other_world_checksum(uint8* working_data);
	//bool check_carnival_world_checksum(uint8* working_data);
	//bool check_other_world_checksum(uint8* working_data);
	//uint16 _fetch_carnival_world_checksum_value(uint8* working_data);
	//uint16 _fetch_other_world_checksum_value(uint8* working_data);

	//void _decrypt_carnival_world(uint8* working_data);
	//void _decrypt_other_world(uint8* working_data);

	//uint8 working_code_data[128 * 2];

	cl_kernel _kernel_expand;
	cl_kernel _kernel_alg;
	cl_kernel _kernel_all_maps;
	
	cl_mem _rng_seed_forward_1_d;
	cl_mem _rng_seed_forward_128_d;

	cl_mem _expansion_values_d;
	cl_mem _regular_rng_values_d;
	cl_mem _alg0_values_d;
	cl_mem _alg2_values_d;
	cl_mem _alg5_values_d;
	cl_mem _alg6_values_d;
	
	

	uint8_t working_code_single[128];
	

	opencl* _cl;
	RNG* rng;

	std::string obj_name;

	static cl_program _program;
	static bool initialized;
};
