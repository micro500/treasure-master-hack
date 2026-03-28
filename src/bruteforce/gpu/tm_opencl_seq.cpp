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

	// Build per-schedule RNG output table: schedule_count * 2048 bytes.
	// map_rng[m*2048 + i] = output byte at step i from map m's initial seed.
	uint8_t* map_rng_h = new uint8_t[schedule_count * 2048];
	uint16_t* nibble_sel_h = new uint16_t[schedule_count];
	for (int m = 0; m < schedule_count; m++)
	{
		uint16_t seed = ((uint16_t)schedule_entries.entries[m].rng1 << 8)
		              |  (uint16_t)schedule_entries.entries[m].rng2;
		for (int i = 0; i < 2048; i++)
		{
			uint16_t next = rng->rng_table[seed];
			map_rng_h[m * 2048 + i] = (uint8_t)(((next >> 8) ^ next) & 0xFF);
			seed = next;
		}
		nibble_sel_h[m] = schedule_entries.entries[m].nibble_selector;
	}
	cl_mem map_rng_d = _cl->create_readonly_buffer(schedule_count * 2048);
	_cl->copy_mem_to_device(map_rng_d, map_rng_h, schedule_count * 2048);
	cl_mem nibble_sel_d = _cl->create_readonly_buffer(schedule_count * sizeof(uint16_t));
	_cl->copy_mem_to_device(nibble_sel_d, nibble_sel_h, schedule_count * sizeof(uint16_t));
	delete[] map_rng_h;
	delete[] nibble_sel_h;

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
	set_kernel_arg<cl_mem>(kernel,   1, &map_rng_d);
	set_kernel_arg<cl_mem>(kernel,   2, &nibble_sel_d);
	set_kernel_arg<uint32_t>(kernel, 3, &key);
	// arg 4 = data_start (set per batch)
	set_kernel_arg<int>(kernel,      5, &schedule_count);
	// arg 6 = chunk (set per batch)
	set_kernel_arg<cl_mem>(kernel,   7, &expansion_d);

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
		set_kernel_arg<uint32_t>(kernel, 4, &data_start);
		set_kernel_arg<uint32_t>(kernel, 6, &chunk);

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
	clReleaseMemObject(map_rng_d);
	clReleaseMemObject(nibble_sel_d);
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
