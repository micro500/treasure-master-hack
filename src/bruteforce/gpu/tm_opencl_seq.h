#pragma once
#include "rng_obj.h"
#include "tm_base.h"
#include "tm_gpu_base.h"
#include "opencl.h"

// GPU bruteforce implementation using a per-schedule RNG table.
// Replaces rng_seq/rng_pos/rng_out (~450KB) with a single per-run map_rng
// table (~52KB) that fits in L1 cache, eliminating the 2-level lookup chain.
// Each workgroup of 32 threads processes 64 candidates sequentially, then
// writes results as a single coalesced 128-byte global write.
class tm_opencl_seq : public TM_GPU_base
{
public:
	tm_opencl_seq(RNG* rng, opencl* cl);

	// Batch test interface (stubs — delegate to base or not implemented)
	void test_expand_batch(const uint8* keys, const uint8* datas, uint8* outputs, uint32 count) override;
	void test_alg_batch(const uint8* alg_ids, const uint16* rng_seeds_in, const uint8* inputs, uint8* outputs, uint16* rng_seeds_out, uint32 count) override;
	void test_run_all_maps_batch(const uint8* keys, const uint8* datas, const uint8* schedule_data_flat, int schedule_count, uint8* outputs, uint32 count) override;

	// Production interface
	void run_bruteforce_boinc(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size) override;
	void run_bruteforce_hash_reduction(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size) override;

private:
	void initialize();
	void run_bruteforce_batch(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);

	cl_kernel _kernel_bruteforce;
	cl_kernel _kernel_test_expand;
	cl_kernel _kernel_test_alg;
	cl_kernel _kernel_test_pipeline;

	cl_mem _result_data_d;   // BATCH_SIZE * 2 bytes (2 bytes per candidate)
	cl_mem _hash_table_d;    // HASH_TABLE_SIZE * 4 uints; zeroed once per run, persists across batches
	cl_mem _deferred_pairs_d; // BATCH_SIZE * 4 uints: {reader_data, writer_data, h2, reader_lane0 | writer_lane0<<32} per pair
	cl_mem _deferred_count_d; // 1 uint: atomic counter for deferred_pairs

	// 64 candidates per workgroup, 2 bytes per result = 128-byte coalesced write
	static const uint32_t BATCH_SIZE         = 1u << 22;
	static const uint32_t CANDIDATES_PER_WG  = 64;
	// Hash table: 8M entries * 16 bytes = 128 MB.  Must be a power of 2.
	static const uint32_t HASH_TABLE_SIZE    = 1u << 23;

	opencl* _cl;
	RNG* rng;

	static cl_program _program;
	static bool initialized;
};
