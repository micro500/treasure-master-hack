#pragma once
#include "data_sizes.h"
#include "key_schedule.h"
#include <string>

class TM_GPU_base
{
public:
	virtual ~TM_GPU_base() {}

	// ---------------------------------------------------------------
	// Batch test interface
	// Same semantics as the single-item methods above, but processes
	// `count` items in one GPU dispatch.  The implementation chunks
	// internally if count exceeds its internal buffer size.
	// Data layout: item i occupies bytes [i*128 .. i*128+127].
	// ---------------------------------------------------------------

	// Expand `count` (key,data) pairs; write results to outputs[count*128].
	virtual void test_expand_batch(
	    const uint8* keys, const uint8* datas,
	    uint8* outputs, uint32 count) = 0;

	// Run one algorithm step on each of `count` 128-byte blocks.
	// inputs and outputs are separate flat [count*128] buffers.
	// rng_seeds_in/out are flat [count] arrays.
	virtual void test_alg_batch(
	    const uint8* alg_ids, const uint16* rng_seeds_in,
	    const uint8* inputs, uint8* outputs,
	    uint16* rng_seeds_out, uint32 count) = 0;

	// Run a sequence of N algorithm steps on each of `count` 128-byte blocks.
	// alg_ids: flat [alg_count] (uniform per-batch); alg_count in [1..16].
	// rng_seeds_in/out are flat [count] arrays.
	virtual void test_wc_alg_multi_batch(
	    const uint8* alg_ids, int alg_count,
	    const uint16* rng_seeds_in,
	    const uint8* inputs, uint8* outputs,
	    uint16* rng_seeds_out, uint32 count) = 0;

	// Expand + run all maps for `count` (key,data) pairs.
	// schedule_data_flat: pre-encoded, count * schedule_count * 4 bytes;
	//   4 bytes per entry: rng1, rng2, nibble_hi, nibble_lo.
	virtual void test_run_all_maps_batch(
	    const uint8* keys, const uint8* datas,
	    const uint8* schedule_data_flat, int schedule_count,
	    uint8* outputs, uint32 count) = 0;

	// ---------------------------------------------------------------
	// Production interface — same signatures as CPU run_bruteforce_boinc
	// ---------------------------------------------------------------

	// Full pipeline, no hash reduction
	virtual void run_bruteforce_boinc(
	    uint32 key,
	    uint32 start_data,
	    const key_schedule& schedule_entries,
	    uint32 amount_to_run,
	    void(*report_progress)(double),
	    uint8* result_data,
	    uint32 result_max_size,
	    uint32* result_size) = 0;

	// Full pipeline with hash reduction at hardcoded cut point.
	// Dummy no-op for now; actual hash implementation TBD.
	virtual void run_bruteforce_hash_reduction(
	    uint32 key,
	    uint32 start_data,
	    const key_schedule& schedule_entries,
	    uint32 amount_to_run,
	    void(*report_progress)(double),
	    uint8* result_data,
	    uint32 result_max_size,
	    uint32* result_size) = 0;

	std::string obj_name;
};
