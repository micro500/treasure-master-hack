#include <stdio.h>
#include <iostream>
#include <string.h>
#include "tm_opencl_32x.h"
#include "key_schedule.h"

static const uint8_t CHECKSUM_SENTINEL = 0x08;

cl_program tm_opencl_32x::_program                = NULL;
cl_program tm_opencl_32x::_program_hash_reduction = NULL;
bool       tm_opencl_32x::initialized             = false;

tm_opencl_32x::tm_opencl_32x(RNG* rng_obj, opencl* cl_init) : rng(rng_obj), _cl(cl_init)
{
	initialize();

	_expansion_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_expansion_values_d, rng->expansion_values_8, 0x10000 * 128);

	_rng_seed_forward_1_d = _cl->create_readonly_buffer(0x10000 * 2);
	_cl->copy_mem_to_device(_rng_seed_forward_1_d, rng->seed_forward_1, 0x10000 * 2);

	_rng_seed_forward_128_d = _cl->create_readonly_buffer(0x10000 * 2);
	_cl->copy_mem_to_device(_rng_seed_forward_128_d, rng->seed_forward_128, 0x10000 * 2);

	_regular_rng_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_regular_rng_values_d, rng->regular_rng_values_8, 0x10000 * 128);

	_alg0_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_alg0_values_d, rng->alg0_values_8, 0x10000 * 128);

	_alg2_values_d = _cl->create_readonly_buffer(0x10000 * 4);
	_cl->copy_mem_to_device(_alg2_values_d, rng->alg2_values_32_8, 0x10000 * 4);

	_alg5_values_d = _cl->create_readonly_buffer(0x10000 * 4);
	_cl->copy_mem_to_device(_alg5_values_d, rng->alg5_values_32_8, 0x10000 * 4);

	_alg6_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_alg6_values_d, rng->alg6_values_8, 0x10000 * 128);

	_carnival_data_d = _cl->create_readonly_buffer(128);
	_cl->copy_mem_to_device(_carnival_data_d, TM_base::carnival_world_data, 128);

	_result_data_d = _cl->create_readwrite_buffer(BATCH_SIZE);

	// Batch test buffers
	_test_expand_keys_d   = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * sizeof(uint32_t));
	_test_expand_datas_d  = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * sizeof(uint32_t));
	_test_expand_output_d = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * 128);

	_test_alg_ids_d      = _cl->create_readwrite_buffer(TEST_BATCH_SIZE);
	_test_alg_seeds_in_d = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * sizeof(uint16_t));
	_test_alg_seeds_out_d= _cl->create_readwrite_buffer(TEST_BATCH_SIZE * sizeof(uint16_t));
	_test_alg_input_d    = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * 128);
	_test_alg_output_d   = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * 128);

	_test_maps_keys_d   = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * sizeof(uint32_t));
	_test_maps_datas_d  = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * sizeof(uint32_t));
	_test_maps_sched_d  = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * 27 * 4);
	_test_maps_output_d = _cl->create_readwrite_buffer(TEST_BATCH_SIZE * 128);
}

void tm_opencl_32x::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();
		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();
		rng->generate_regular_rng_values_8();
		rng->generate_alg0_values_8();
		rng->generate_alg2_values_32_8();
		rng->generate_alg5_values_32_8();
		rng->generate_alg6_values_8();

		_program = _cl->create_program("tm.cl");
		_cl->build_program(_program);
		output_kernel_asm_to_file(_program, "kernel.ptx");

		_program_hash_reduction = _cl->create_program("tm.cl");
		_cl->build_program(_program_hash_reduction, "-DHASH_REDUCTION=1");

		initialized = true;
	}

	_kernel_expand_batch    = _cl->create_kernel(_program, "expand_test_batch");
	_kernel_alg_batch       = _cl->create_kernel(_program, "alg_test_batch");
	_kernel_all_maps_batch  = _cl->create_kernel(_program, "all_maps_test_batch");
	_kernel_bruteforce      = _cl->create_kernel(_program, "tm_bruteforce");
	_kernel_hash_reduction  = _cl->create_kernel(_program_hash_reduction, "tm_bruteforce");

	obj_name = "tm_opencl_32x";
}

// -------------------------------------------------------------------
// Test interface
// -------------------------------------------------------------------

// -------------------------------------------------------------------
// Production interface
// -------------------------------------------------------------------

void tm_opencl_32x::run_bruteforce_batch(
	uint32 key, uint32 start_data,
	const key_schedule& schedule_entries,
	uint32 amount_to_run,
	void(*report_progress)(double),
	uint8* result_data,
	uint32 result_max_size,
	uint32* result_size,
	cl_kernel kernel)
{
	// Encode schedule entries
	int schedule_count = (int)schedule_entries.entries.size();
	uint8 schedule_data_h[27 * 4] = {};
	for (int i = 0; i < schedule_count; i++)
	{
		schedule_data_h[i*4+0] = schedule_entries.entries[i].rng1;
		schedule_data_h[i*4+1] = schedule_entries.entries[i].rng2;
		schedule_data_h[i*4+2] = (schedule_entries.entries[i].nibble_selector >> 8) & 0xFF;
		schedule_data_h[i*4+3] =  schedule_entries.entries[i].nibble_selector       & 0xFF;
	}

	cl_mem schedule_data_d = _cl->create_readonly_buffer(27 * 4);
	_cl->copy_mem_to_device(schedule_data_d, schedule_data_h, 27 * 4);

	// Set fixed kernel args
	set_kernel_arg<cl_mem>(kernel,  0, &_result_data_d);
	set_kernel_arg<cl_mem>(kernel,  1, &_regular_rng_values_d);
	set_kernel_arg<cl_mem>(kernel,  2, &_alg0_values_d);
	set_kernel_arg<cl_mem>(kernel,  3, &_alg6_values_d);
	set_kernel_arg<cl_mem>(kernel,  4, &_rng_seed_forward_1_d);
	set_kernel_arg<cl_mem>(kernel,  5, &_rng_seed_forward_128_d);
	set_kernel_arg<cl_mem>(kernel,  6, &_alg2_values_d);
	set_kernel_arg<cl_mem>(kernel,  7, &_alg5_values_d);
	set_kernel_arg<cl_mem>(kernel,  8, &_expansion_values_d);
	set_kernel_arg<cl_mem>(kernel,  9, &schedule_data_d);
	set_kernel_arg<cl_mem>(kernel, 10, &_carnival_data_d);
	set_kernel_arg<uint32_t>(kernel, 11, &key);
	// arg 12 = data_start (set per batch)
	set_kernel_arg<int>(kernel, 13, &schedule_count);

	*result_size = 0;
	uint8* batch_result_h = new uint8[BATCH_SIZE];

	for (uint32 pos = 0; pos < amount_to_run; pos += BATCH_SIZE)
	{
		uint32 chunk = amount_to_run - pos;
		if (chunk > BATCH_SIZE) chunk = BATCH_SIZE;

		uint32 data_start = start_data + pos;
		set_kernel_arg<uint32_t>(kernel, 12, &data_start);

		size_t global_item_size[3] = { 32, chunk, 1 };
		size_t local_item_size[3]  = { 32, 1,     1 };

		cl_event event = _cl->run_kernel(kernel, 3, NULL, global_item_size, local_item_size);
		clWaitForEvents(1, &event);
		_cl->finish();
		{
			cl_ulong t0, t1;
			clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(t0), &t0, NULL);
			clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,   sizeof(t1), &t1, NULL);
			clReleaseEvent(event);
			fprintf(stderr, "[32x] %.3f ms  %.2f M/s\n",
			        (t1 - t0) / 1e6, chunk / ((t1 - t0) / 1e9) / 1e6);
		}

		_cl->copy_mem_from_device(_result_data_d, batch_result_h, chunk);

		for (uint32 i = 0; i < chunk; i++)
		{
			if (batch_result_h[i] != 0 && *result_size + 5 <= result_max_size)
			{
				uint32 lsb = data_start + i;
				memcpy(result_data + *result_size, &lsb, 4);
				result_data[*result_size + 4] = batch_result_h[i] & ~CHECKSUM_SENTINEL;
				*result_size += 5;
			}
		}

		if (report_progress)
			report_progress((double)(pos + chunk) / amount_to_run);
	}

	delete[] batch_result_h;
	clReleaseMemObject(schedule_data_d);
}

// -------------------------------------------------------------------
// Batch test methods
// -------------------------------------------------------------------

void tm_opencl_32x::test_expand_batch(
	const uint8* keys, const uint8* datas,
	uint8* outputs, uint32 count)
{
	for (uint32 pos = 0; pos < count; pos += TEST_BATCH_SIZE)
	{
		uint32 chunk = count - pos;
		if (chunk > TEST_BATCH_SIZE) chunk = TEST_BATCH_SIZE;

		_cl->copy_mem_to_device(_test_expand_keys_d,  (void*)(keys  + pos * 4), chunk * 4);
		_cl->copy_mem_to_device(_test_expand_datas_d, (void*)(datas + pos * 4), chunk * 4);

		set_kernel_arg<cl_mem>(_kernel_expand_batch, 0, &_test_expand_keys_d);
		set_kernel_arg<cl_mem>(_kernel_expand_batch, 1, &_test_expand_datas_d);
		set_kernel_arg<cl_mem>(_kernel_expand_batch, 2, &_expansion_values_d);
		set_kernel_arg<cl_mem>(_kernel_expand_batch, 3, &_test_expand_output_d);

		size_t global_item_size[3] = { 32, chunk, 1 };
		size_t local_item_size[3]  = { 32, 1,     1 };

		cl_event event = _cl->run_kernel(_kernel_expand_batch, 3, NULL, global_item_size, local_item_size);
		clWaitForEvents(1, &event);
		_cl->finish();

		_cl->copy_mem_from_device(_test_expand_output_d, outputs + pos * 128, chunk * 128);
	}
}

void tm_opencl_32x::test_alg_batch(
	const uint8* alg_ids, const uint16* rng_seeds_in,
	const uint8* inputs, uint8* outputs,
	uint16* rng_seeds_out, uint32 count)
{
	for (uint32 pos = 0; pos < count; pos += TEST_BATCH_SIZE)
	{
		uint32 chunk = count - pos;
		if (chunk > TEST_BATCH_SIZE) chunk = TEST_BATCH_SIZE;

		_cl->copy_mem_to_device(_test_alg_ids_d,      (void*)(alg_ids      + pos),       chunk);
		_cl->copy_mem_to_device(_test_alg_seeds_in_d, (void*)(rng_seeds_in + pos),       chunk * sizeof(uint16_t));
		_cl->copy_mem_to_device(_test_alg_input_d,    (void*)(inputs       + pos * 128), chunk * 128);

		set_kernel_arg<cl_mem>(_kernel_alg_batch,  0, &_test_alg_ids_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  1, &_test_alg_seeds_in_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  2, &_test_alg_input_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  3, &_test_alg_output_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  4, &_rng_seed_forward_1_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  5, &_rng_seed_forward_128_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  6, &_regular_rng_values_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  7, &_alg0_values_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  8, &_alg2_values_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch,  9, &_alg5_values_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch, 10, &_alg6_values_d);
		set_kernel_arg<cl_mem>(_kernel_alg_batch, 11, &_test_alg_seeds_out_d);

		size_t global_item_size[3] = { 32, chunk, 1 };
		size_t local_item_size[3]  = { 32, 1,     1 };

		cl_event event = _cl->run_kernel(_kernel_alg_batch, 3, NULL, global_item_size, local_item_size);
		clWaitForEvents(1, &event);
		_cl->finish();

		_cl->copy_mem_from_device(_test_alg_output_d,    outputs      + pos * 128,       chunk * 128);
		_cl->copy_mem_from_device(_test_alg_seeds_out_d, rng_seeds_out + pos,            chunk * sizeof(uint16_t));
	}
}

void tm_opencl_32x::test_run_all_maps_batch(
	const uint8* keys, const uint8* datas,
	const uint8* schedule_data_flat, int schedule_count,
	uint8* outputs, uint32 count)
{
	for (uint32 pos = 0; pos < count; pos += TEST_BATCH_SIZE)
	{
		uint32 chunk = count - pos;
		if (chunk > TEST_BATCH_SIZE) chunk = TEST_BATCH_SIZE;

		_cl->copy_mem_to_device(_test_maps_keys_d,  (void*)(keys  + pos * 4), chunk * 4);
		_cl->copy_mem_to_device(_test_maps_datas_d, (void*)(datas + pos * 4), chunk * 4);
		_cl->copy_mem_to_device(_test_maps_sched_d, (void*)(schedule_data_flat + pos * schedule_count * 4), chunk * schedule_count * 4);

		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  0, &_test_maps_keys_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  1, &_test_maps_datas_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  2, &_test_maps_sched_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  3, &_test_maps_output_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  4, &_expansion_values_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  5, &_rng_seed_forward_1_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  6, &_rng_seed_forward_128_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  7, &_regular_rng_values_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  8, &_alg0_values_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch,  9, &_alg2_values_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch, 10, &_alg5_values_d);
		set_kernel_arg<cl_mem>(_kernel_all_maps_batch, 11, &_alg6_values_d);
		set_kernel_arg<int>   (_kernel_all_maps_batch, 12, &schedule_count);

		size_t global_item_size[3] = { 32, chunk, 1 };
		size_t local_item_size[3]  = { 32, 1,     1 };

		cl_event event = _cl->run_kernel(_kernel_all_maps_batch, 3, NULL, global_item_size, local_item_size);
		clWaitForEvents(1, &event);
		_cl->finish();

		_cl->copy_mem_from_device(_test_maps_output_d, outputs + pos * 128, chunk * 128);
	}
}

void tm_opencl_32x::run_bruteforce_boinc(
	uint32 key, uint32 start_data,
	const key_schedule& schedule_entries,
	uint32 amount_to_run,
	void(*report_progress)(double),
	uint8* result_data,
	uint32 result_max_size,
	uint32* result_size)
{
	run_bruteforce_batch(key, start_data, schedule_entries, amount_to_run,
	                     report_progress, result_data, result_max_size, result_size,
	                     _kernel_bruteforce);
}

void tm_opencl_32x::run_bruteforce_hash_reduction(
	uint32 key, uint32 start_data,
	const key_schedule& schedule_entries,
	uint32 amount_to_run,
	void(*report_progress)(double),
	uint8* result_data,
	uint32 result_max_size,
	uint32* result_size)
{
	run_bruteforce_batch(key, start_data, schedule_entries, amount_to_run,
	                     report_progress, result_data, result_max_size, result_size,
	                     _kernel_hash_reduction);
}
