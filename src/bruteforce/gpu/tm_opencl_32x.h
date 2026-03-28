#pragma once
#include "rng_obj.h"
#include "tm_base.h"
#include "tm_gpu_base.h"
#include "opencl.h"

class tm_opencl_32x : public TM_GPU_base
{
public:
	tm_opencl_32x(RNG* rng, opencl* cl);

	// Batch test interface
	void test_expand_batch(const uint8* keys, const uint8* datas, uint8* outputs, uint32 count) override;
	void test_alg_batch(const uint8* alg_ids, const uint16* rng_seeds_in, const uint8* inputs, uint8* outputs, uint16* rng_seeds_out, uint32 count) override;
	void test_run_all_maps_batch(const uint8* keys, const uint8* datas, const uint8* schedule_data_flat, int schedule_count, uint8* outputs, uint32 count) override;

	// Production interface
	void run_bruteforce_boinc(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size) override;
	void run_bruteforce_hash_reduction(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size) override;

private:
	void initialize();
	void run_bruteforce_batch(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size, cl_kernel kernel);

	cl_kernel _kernel_expand_batch;
	cl_kernel _kernel_alg_batch;
	cl_kernel _kernel_all_maps_batch;
	cl_kernel _kernel_bruteforce;
	cl_kernel _kernel_hash_reduction;

	cl_mem _rng_seed_forward_1_d;
	cl_mem _rng_seed_forward_128_d;
	cl_mem _expansion_values_d;
	cl_mem _regular_rng_values_d;
	cl_mem _alg0_values_d;
	cl_mem _alg2_values_d;
	cl_mem _alg5_values_d;
	cl_mem _alg6_values_d;
	cl_mem _carnival_data_d;
	cl_mem _result_data_d;

	// Batch test buffers (pre-allocated, sized for TEST_BATCH_SIZE items)
	cl_mem _test_expand_keys_d;
	cl_mem _test_expand_datas_d;
	cl_mem _test_expand_output_d;
	cl_mem _test_alg_ids_d;
	cl_mem _test_alg_seeds_in_d;
	cl_mem _test_alg_seeds_out_d;
	cl_mem _test_alg_input_d;
	cl_mem _test_alg_output_d;
	cl_mem _test_maps_keys_d;
	cl_mem _test_maps_datas_d;
	cl_mem _test_maps_sched_d;
	cl_mem _test_maps_output_d;

	static const uint32_t BATCH_SIZE      = 1u << 20;
	static const uint32_t TEST_BATCH_SIZE = 1u << 14;  // 16384 items per test dispatch

	opencl* _cl;
	RNG* rng;

	static cl_program _program;
	static cl_program _program_hash_reduction;
	static bool initialized;
};
