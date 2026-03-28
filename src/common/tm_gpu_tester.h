#pragma once
#include "data_sizes.h"
#include "key_schedule.h"
#include "tm_gpu_base.h"

// Analogous to tm_tester, but wraps TM_GPU_base*.
// Methods match tm_tester's interface so the same test functions can drive both.
// Not implemented: process_load_fetch, calculate_checksum, fetch_checksum_value,
// run_iterations — these have no GPU equivalent.
class tm_gpu_tester
{
public:
	tm_gpu_tester(TM_GPU_base* obj) : UUT(obj) {}

	void run_results_process(uint32 key, uint32 data, const key_schedule& schedule_entries, uint32 amount_to_run, uint8* result_data, uint32 result_max_size, uint32* result_size)
	{
		UUT->run_bruteforce_boinc(key, data, schedule_entries, amount_to_run, nullptr, result_data, result_max_size, result_size);
	}

	// Batch interface — process many test cases in a single dispatch
	void run_expansion_batch(const uint8* keys, const uint8* datas, uint8* outputs, uint32 count)
	{
		UUT->test_expand_batch(keys, datas, outputs, count);
	}

	void process_alg_test_case_batch(
	    const uint8* alg_ids, const uint16* rng_seeds_in,
	    const uint8* inputs, uint8* outputs,
	    uint16* rng_seeds_out, uint32 count)
	{
		UUT->test_alg_batch(alg_ids, rng_seeds_in, inputs, outputs, rng_seeds_out, count);
	}

	void run_full_process_batch(
	    const uint8* keys, const uint8* datas,
	    const uint8* schedule_data_flat, int schedule_count,
	    uint8* outputs, uint32 count)
	{
		UUT->test_run_all_maps_batch(keys, datas, schedule_data_flat, schedule_count, outputs, count);
	}

private:
	TM_GPU_base* UUT;
};
