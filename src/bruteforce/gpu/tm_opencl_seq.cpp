#include <stdio.h>
#include <iostream>
#include <string.h>
#include "tm_opencl_seq.h"
#include "key_schedule.h"

static const uint8_t CHECKSUM_SENTINEL = 0x08;

cl_program tm_opencl_seq::_program   = NULL;
bool       tm_opencl_seq::initialized = false;

tm_opencl_seq::tm_opencl_seq(RNG* rng_obj, opencl* cl_init) : rng(rng_obj), _cl(cl_init)
{
	initialize();

	uint32_t seq_size = rng->rng_seq_table_size;
	_rng_seq_d = _cl->create_readonly_buffer(seq_size * sizeof(uint16_t));
	_cl->copy_mem_to_device(_rng_seq_d, rng->rng_seq_table, seq_size * sizeof(uint16_t));

	_rng_pos_d = _cl->create_readonly_buffer(0x10000 * sizeof(uint32_t));
	_cl->copy_mem_to_device(_rng_pos_d, rng->rng_pos_table, 0x10000 * sizeof(uint32_t));

	// Pre-computed output byte table: rng_out[i] = ((rng_seq[i]>>8) ^ rng_seq[i]) & 0xFF
	uint8_t* rng_out_h = new uint8_t[seq_size];
	for (uint32_t i = 0; i < seq_size; i++)
	{
		uint16_t v = rng->rng_seq_table[i];
		rng_out_h[i] = (uint8_t)(((v >> 8) ^ v) & 0xFF);
	}
	_rng_out_d = _cl->create_readonly_buffer(seq_size * sizeof(uint8_t));
	_cl->copy_mem_to_device(_rng_out_d, rng_out_h, seq_size * sizeof(uint8_t));
	delete[] rng_out_h;

	_result_data_d = _cl->create_readwrite_buffer(BATCH_SIZE * 2);
}

void tm_opencl_seq::initialize()
{
	if (!initialized)
	{
		rng->generate_rng_seq_tables();

		_program = _cl->create_program("tm_seq.cl");
		_cl->build_program(_program);
		output_kernel_asm_to_file(_program, "tm_seq.ptx");

		initialized = true;
	}

	_kernel_bruteforce = _cl->create_kernel(_program, "tm_bruteforce_seq");
	obj_name = "tm_opencl_seq";
}

void tm_opencl_seq::run_bruteforce_batch(
	uint32 key, uint32 start_data,
	const key_schedule& schedule_entries,
	uint32 amount_to_run,
	void(*report_progress)(double),
	uint8* result_data,
	uint32 result_max_size,
	uint32* result_size)
{
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

	// Precompute expansion values on CPU — purely key-dependent, static for this run.
	uint32_t expansion_h[32] = {};
	{
		uint32_t pos_base = rng->rng_pos_table[(key >> 16) & 0xFFFF];
		for (int lane = 0; lane < 32; lane++)
		{
			uint32_t expansion_val = 0;
			for (int byte_idx = 0; byte_idx < 4; byte_idx++)
			{
				uint32_t b = lane * 4 + byte_idx;
				uint32_t j = b / 8;
				uint32_t k = b % 8;
				uint32_t accum = 0;
				for (uint32_t i = 0; i < j; i++)
				{
					uint16_t sv = rng->rng_seq_table[pos_base + k + i * 8];
					accum += ((sv >> 8) ^ sv) & 0xFF;
				}
				expansion_val |= (accum & 0xFF) << (byte_idx * 8);
			}
			expansion_h[lane] = expansion_val;
		}
	}
	cl_mem expansion_d = _cl->create_readonly_buffer(32 * sizeof(uint32_t));
	_cl->copy_mem_to_device(expansion_d, expansion_h, 32 * sizeof(uint32_t));

	cl_kernel kernel = _kernel_bruteforce;
	set_kernel_arg<cl_mem>(kernel,   0, &_result_data_d);
	set_kernel_arg<cl_mem>(kernel,   1, &_rng_seq_d);
	set_kernel_arg<cl_mem>(kernel,   2, &_rng_pos_d);
	set_kernel_arg<cl_mem>(kernel,   3, &schedule_data_d);
	set_kernel_arg<uint32_t>(kernel, 4, &key);
	// arg 5 = data_start (set per batch)
	set_kernel_arg<int>(kernel,      6, &schedule_count);
	// arg 7 = chunk (set per batch)
	set_kernel_arg<cl_mem>(kernel,   8, &expansion_d);
	set_kernel_arg<cl_mem>(kernel,   9, &_rng_out_d);

	*result_size = 0;
	// Result buffer: 2 bytes per candidate, so read back chunk*2 bytes per batch
	// Buffer is sized BATCH_SIZE*2; we allocate a host-side buffer the same size
	uint8* batch_result_h = new uint8[BATCH_SIZE * 2];

	bool     first_batch  = true;
	cl_ulong total_ns     = 0;
	uint64_t total_chunks = 0;
	for (uint32 pos = 0; pos < amount_to_run; pos += BATCH_SIZE)
	{
		uint32 chunk = amount_to_run - pos;
		if (chunk > BATCH_SIZE) chunk = BATCH_SIZE;

		uint32 data_start = start_data + pos;
		set_kernel_arg<uint32_t>(kernel, 5, &data_start);
		set_kernel_arg<uint32_t>(kernel, 7, &chunk);

		// Each workgroup processes CANDIDATES_PER_WG candidates
		uint32 num_wg = (chunk + CANDIDATES_PER_WG - 1) / CANDIDATES_PER_WG;

		size_t global_item_size[3] = { 32, num_wg, 1 };
		size_t local_item_size[3]  = { 32, 1,      1 };

		cl_event event = _cl->run_kernel(kernel, 3, NULL, global_item_size, local_item_size);
		clWaitForEvents(1, &event);
		_cl->finish();
		{
			cl_ulong t0, t1;
			clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(t0), &t0, NULL);
			clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,   sizeof(t1), &t1, NULL);
			clReleaseEvent(event);
			cl_ulong dt = t1 - t0;
			fprintf(stderr, "[seq]%s %.3f ms  %.2f M/s\n",
			        first_batch ? " (warmup)" : "         ",
			        dt / 1e6, chunk / (dt / 1e9) / 1e6);
			if (!first_batch) { total_ns += dt; total_chunks += chunk; }
			first_batch = false;
		}

		// Read back num_wg * 128 bytes (each workgroup writes 128 bytes regardless)
		uint32 read_bytes = num_wg * 128;
		_cl->copy_mem_from_device(_result_data_d, batch_result_h, read_bytes);

		for (uint32 i = 0; i < chunk; i++)
		{
			uint8 flags = batch_result_h[i * 2];
			if (flags != 0 && *result_size + 5 <= result_max_size)
			{
				uint32 lsb = data_start + i;
				memcpy(result_data + *result_size, &lsb, 4);
				result_data[*result_size + 4] = flags & ~CHECKSUM_SENTINEL;
				*result_size += 5;
			}
		}

		if (report_progress)
			report_progress((double)(pos + chunk) / amount_to_run);
	}

	if (total_ns > 0)
		fprintf(stderr, "[seq] avg %.2f M/s\n", total_chunks / (total_ns / 1e9) / 1e6);

	delete[] batch_result_h;
	clReleaseMemObject(schedule_data_d);
	clReleaseMemObject(expansion_d);
}

void tm_opencl_seq::run_bruteforce_boinc(
	uint32 key, uint32 start_data,
	const key_schedule& schedule_entries,
	uint32 amount_to_run,
	void(*report_progress)(double),
	uint8* result_data,
	uint32 result_max_size,
	uint32* result_size)
{
	run_bruteforce_batch(key, start_data, schedule_entries, amount_to_run,
	                     report_progress, result_data, result_max_size, result_size);
}

void tm_opencl_seq::run_bruteforce_hash_reduction(
	uint32 key, uint32 start_data,
	const key_schedule& schedule_entries,
	uint32 amount_to_run,
	void(*report_progress)(double),
	uint8* result_data,
	uint32 result_max_size,
	uint32* result_size)
{
	run_bruteforce_batch(key, start_data, schedule_entries, amount_to_run,
	                     report_progress, result_data, result_max_size, result_size);
}

// Test interface stubs — not implemented for this class
void tm_opencl_seq::test_expand_batch(const uint8*, const uint8*, uint8*, uint32)
{
	std::cout << "test_expand_batch not implemented for tm_opencl_seq" << std::endl;
}
void tm_opencl_seq::test_alg_batch(const uint8*, const uint16*, const uint8*, uint8*, uint16*, uint32)
{
	std::cout << "test_alg_batch not implemented for tm_opencl_seq" << std::endl;
}
void tm_opencl_seq::test_run_all_maps_batch(const uint8*, const uint8*, const uint8*, int, uint8*, uint32)
{
	std::cout << "test_run_all_maps_batch not implemented for tm_opencl_seq" << std::endl;
}
