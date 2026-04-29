#include <stdio.h>
#include <iostream>
#include <string.h>
#include "tm_opencl_seq.h"
#include "key_schedule.h"

static const uint8_t CHECKSUM_SENTINEL = 0x08;

cl_program tm_opencl_seq::_program = NULL;

tm_opencl_seq::tm_opencl_seq(RNG* rng_obj, opencl* cl_init) : rng(rng_obj), _cl(cl_init)
{
	initialize();

	_result_data_d    = _cl->create_readwrite_buffer(BATCH_SIZE * 2);
	_hash_table_d     = _cl->create_readwrite_buffer(HASH_TABLE_SIZE * 4 * sizeof(uint32_t));
	_deferred_pairs_d = _cl->create_readwrite_buffer(BATCH_SIZE * 4 * sizeof(uint32_t));
	_deferred_count_d = _cl->create_readwrite_buffer(sizeof(uint32_t));
}

void tm_opencl_seq::initialize()
{
	if (!_initialized)
	{
		auto _r0 = rng->generate_rng_table();
		auto _r1 = rng->generate_rng_seq_tables();

		_table_refs = { _r0, _r1 };

		_program = _cl->create_program("tm_seq.cl");
		_cl->build_program(_program);
		output_kernel_asm_to_file(_program, "tm_seq.ptx");

		_initialized = true;
	}

	_kernel_bruteforce    = _cl->create_kernel(_program, "tm_bruteforce_seq");
	_kernel_test_expand   = _cl->create_kernel(_program, "tm_test_expand");
	_kernel_test_alg      = _cl->create_kernel(_program, "tm_test_alg");
	_kernel_test_pipeline = _cl->create_kernel(_program, "tm_test_pipeline");
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
			uint16_t next = rng->rng_table.ptr[seed];
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
		uint32_t pos_base = rng->rng_pos_table.ptr[(key >> 16) & 0xFFFF];
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
					uint16_t sv = rng->rng_seq_table.ptr[pos_base + k + i * 8];
					accum += ((sv >> 8) ^ sv) & 0xFF;
				}
				expansion_val |= (accum & 0xFF) << (byte_idx * 8);
			}
			expansion_h[lane] = expansion_val;
		}
	}
	cl_mem expansion_d = _cl->create_readonly_buffer(32 * sizeof(uint32_t));
	_cl->copy_mem_to_device(expansion_d, expansion_h, 32 * sizeof(uint32_t));

	// Zero all run-persistent buffers once before any kernel dispatches.
	// hash_table and deferred buffers persist across inner batches.
	const uint32_t zero = 0;
	clEnqueueFillBuffer(_cl->command_queue, _hash_table_d, &zero, sizeof(zero),
	                    0, HASH_TABLE_SIZE * 4 * sizeof(uint32_t), 0, NULL, NULL);
	clEnqueueFillBuffer(_cl->command_queue, _deferred_count_d, &zero, sizeof(zero),
	                    0, sizeof(uint32_t), 0, NULL, NULL);

	cl_kernel kernel = _kernel_bruteforce;
	set_kernel_arg<cl_mem>(kernel,   0, &_result_data_d);
	set_kernel_arg<cl_mem>(kernel,   1, &map_rng_d);
	set_kernel_arg<cl_mem>(kernel,   2, &nibble_sel_d);
	set_kernel_arg<uint32_t>(kernel, 3, &key);
	// arg 4 = data_start (set per batch)
	set_kernel_arg<int>(kernel,      5, &schedule_count);
	// arg 6 = chunk (set per batch)
	set_kernel_arg<cl_mem>(kernel,   7, &expansion_d);
	set_kernel_arg<cl_mem>(kernel,   8, &_hash_table_d);
	uint32_t hash_table_mask = HASH_TABLE_SIZE - 1;
	set_kernel_arg<uint32_t>(kernel, 9, &hash_table_mask);
	set_kernel_arg<cl_mem>(kernel,  10, &_deferred_pairs_d);
	set_kernel_arg<cl_mem>(kernel,  11, &_deferred_count_d);

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
			uint8 carnival_flags = batch_result_h[i * 2];
			uint8 other_flags    = batch_result_h[i * 2 + 1];
			uint8 flags = carnival_flags ? carnival_flags : other_flags;
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

	// -------------------------------------------------------------------
	// Deferred resolution: candidates that found a slot IN_PROGRESS/CLAIMED
	// had their {reader_data, writer_data} pair recorded in the deferred list.
	// All kernel work is done now, so every writer has finished and its result
	// is in the normal output.  Build a map {lsb -> flags} from what we
	// already collected, then emit a result entry for each deferred reader.
	// -------------------------------------------------------------------
	uint32_t deferred_n = 0;
	_cl->copy_mem_from_device(_deferred_count_d, &deferred_n, sizeof(uint32_t));

	fprintf(stderr, "[hash] deferred entries: %u\n", deferred_n);

	if (deferred_n > 0)
	{
		if (deferred_n > BATCH_SIZE)
		{
			fprintf(stderr, "[hash] deferred overflow: clamping %u -> %u\n",
			        deferred_n, (uint32_t)BATCH_SIZE);
			deferred_n = BATCH_SIZE;
		}

		uint32_t* deferred_h = new uint32_t[deferred_n * 4];
		_cl->copy_mem_from_device(_deferred_pairs_d, deferred_h,
		                          deferred_n * 4 * sizeof(uint32_t));

		uint32_t show = deferred_n < 4 ? deferred_n : 4;
		fprintf(stderr, "[hash] same-state-after-map4 examples (key=0x%08x):\n", key);
		for (uint32_t i = 0; i < show; i++)
		{
			uint32_t reader_data = deferred_h[i * 4 + 0];
			uint32_t writer_data = deferred_h[i * 4 + 1];
			uint32_t h2          = deferred_h[i * 4 + 2];
			uint32_t h3          = deferred_h[i * 4 + 3];
			fprintf(stderr, "[hash]   writer=0x%08x reader=0x%08x  h2=0x%08x h3=0x%08x\n",
			        writer_data, reader_data, h2, h3);
		}

		// Build a lookup from the results we have already written.
		// result_data contains packed 5-byte entries: [lsb(4)] [flags(1)]
		uint32_t existing = *result_size / 5;
		uint32_t resolved = 0, missed = 0;
		for (uint32_t d = 0; d < deferred_n; d++)
		{
			uint32_t reader_lsb = deferred_h[d * 4 + 0];
			uint32_t writer_lsb = deferred_h[d * 4 + 1];

			// Find writer's result entry by linear scan (hits are rare)
			uint8 flags = 0;
			for (uint32_t r = 0; r < existing; r++)
			{
				uint32_t entry_lsb;
				memcpy(&entry_lsb, result_data + r * 5, 4);
				if (entry_lsb == writer_lsb)
				{
					flags = result_data[r * 5 + 4];
					break;
				}
			}

			if (flags != 0 && *result_size + 5 <= result_max_size)
			{
				memcpy(result_data + *result_size, &reader_lsb, 4);
				result_data[*result_size + 4] = flags;
				*result_size += 5;
				resolved++;
			}
			else if (flags == 0)
			{
				missed++;
			}
		}

		fprintf(stderr, "[hash] deferred resolved: %u  no-hit: %u\n", resolved, missed);
		delete[] deferred_h;
	}

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

// ---------------------------------------------------------------------------
// Helper: build the 32-element expansion table for the given key.
// Identical to the computation in run_bruteforce_batch.
// ---------------------------------------------------------------------------
static void build_expansion(RNG* rng, uint32_t key, uint32_t expansion_h[32])
{
	uint32_t pos_base = rng->rng_pos_table.ptr[(key >> 16) & 0xFFFF];
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
				uint16_t sv = rng->rng_seq_table.ptr[pos_base + k + i * 8];
				accum += ((sv >> 8) ^ sv) & 0xFF;
			}
			expansion_val |= (accum & 0xFF) << (byte_idx * 8);
		}
		expansion_h[lane] = expansion_val;
	}
}

// ---------------------------------------------------------------------------
// test_expand_batch: run tm_test_expand for each (key, data) pair.
// keys/datas are 4-byte big-endian values; outputs are 128-byte CPU arrays.
// ---------------------------------------------------------------------------
void tm_opencl_seq::test_expand_batch(const uint8* keys, const uint8* datas, uint8* outputs, uint32 count)
{
	cl_mem out_d   = _cl->create_readwrite_buffer(128);
	cl_mem exp_d   = _cl->create_readonly_buffer(32 * sizeof(uint32_t));

	for (uint32 i = 0; i < count; i++)
	{
		uint32_t key  = ((uint32_t)keys [i*4+0] << 24) | ((uint32_t)keys [i*4+1] << 16)
		              | ((uint32_t)keys [i*4+2] <<  8) |  (uint32_t)keys [i*4+3];
		uint32_t data = ((uint32_t)datas[i*4+0] << 24) | ((uint32_t)datas[i*4+1] << 16)
		              | ((uint32_t)datas[i*4+2] <<  8) |  (uint32_t)datas[i*4+3];

		uint32_t expansion_h[32] = {};
		build_expansion(rng, key, expansion_h);
		_cl->copy_mem_to_device(exp_d, expansion_h, 32 * sizeof(uint32_t));

		cl_kernel k = _kernel_test_expand;
		set_kernel_arg<uint32_t>(k, 0, &key);
		set_kernel_arg<uint32_t>(k, 1, &data);
		set_kernel_arg<cl_mem>  (k, 2, &exp_d);
		set_kernel_arg<cl_mem>  (k, 3, &out_d);

		size_t gs[3] = { 32, 1, 1 };
		size_t ls[3] = { 32, 1, 1 };
		cl_event ev = _cl->run_kernel(k, 3, NULL, gs, ls);
		clWaitForEvents(1, &ev);
		clReleaseEvent(ev);

		_cl->copy_mem_from_device(out_d, outputs + i * 128, 128);
	}

	clReleaseMemObject(out_d);
	clReleaseMemObject(exp_d);
}

// ---------------------------------------------------------------------------
// test_alg_batch: run tm_test_alg for each (alg_id, rng_seed, input) tuple.
// input/output are 128-byte CPU arrays; rng_seeds_in are the 16-bit seeds
// used to build the single-map map_rng slice (2048 bytes) passed to the kernel.
// rng_seeds_out receives the seed after 128 RNG steps (one full map pass).
// ---------------------------------------------------------------------------
void tm_opencl_seq::test_alg_batch(const uint8* alg_ids, const uint16* rng_seeds_in,
                                    const uint8* inputs, uint8* outputs,
                                    uint16* rng_seeds_out, uint32 count)
{
	// map_rng for a single map: 2048 bytes
	cl_mem map_rng_d = _cl->create_readwrite_buffer(2048);
	cl_mem in_d      = _cl->create_readonly_buffer(128);
	cl_mem out_d     = _cl->create_readwrite_buffer(128);

	for (uint32 i = 0; i < count; i++)
	{
		uint8_t  alg_id = alg_ids[i];
		uint16_t seed   = rng_seeds_in[i];

		uint8_t map_rng_h[2048];
		for (int s = 0; s < 2048; s++)
		{
			uint16_t next = rng->rng_table.ptr[seed];
			map_rng_h[s] = (uint8_t)(((next >> 8) ^ next) & 0xFF);
			seed = next;
		}
		if (rng_seeds_out)
			rng_seeds_out[i] = seed;

		_cl->copy_mem_to_device(map_rng_d, map_rng_h, 2048);
		_cl->copy_mem_to_device(in_d, const_cast<uint8*>(inputs + i * 128), 128);

		uint32_t alg_id_u = (uint32_t)alg_id;
		cl_kernel k = _kernel_test_alg;
		set_kernel_arg<cl_mem>  (k, 0, &in_d);
		set_kernel_arg<uint32_t>(k, 1, &alg_id_u);
		set_kernel_arg<cl_mem>  (k, 2, &map_rng_d);
		set_kernel_arg<cl_mem>  (k, 3, &out_d);

		size_t gs[3] = { 32, 1, 1 };
		size_t ls[3] = { 32, 1, 1 };
		cl_event ev = _cl->run_kernel(k, 3, NULL, gs, ls);
		clWaitForEvents(1, &ev);
		clReleaseEvent(ev);

		_cl->copy_mem_from_device(out_d, outputs + i * 128, 128);
	}

	clReleaseMemObject(map_rng_d);
	clReleaseMemObject(in_d);
	clReleaseMemObject(out_d);
}

// ---------------------------------------------------------------------------
// test_run_all_maps_batch: run tm_test_pipeline for each (key, data) pair.
// schedule_data_flat: schedule_count * 4 bytes per map (rng1, rng2, nh, nl).
// outputs: count * 128-byte CPU arrays.
// ---------------------------------------------------------------------------
void tm_opencl_seq::test_run_all_maps_batch(const uint8* keys, const uint8* datas,
                                             const uint8* schedule_data_flat, int schedule_count,
                                             uint8* outputs, uint32 count)
{
	// Build per-schedule map_rng and nibble_sel from schedule_data_flat
	uint8_t*  map_rng_h   = new uint8_t [schedule_count * 2048];
	uint16_t* nibble_sel_h = new uint16_t[schedule_count];
	for (int m = 0; m < schedule_count; m++)
	{
		uint8_t  rng1 = schedule_data_flat[m * 4 + 0];
		uint8_t  rng2 = schedule_data_flat[m * 4 + 1];
		uint8_t  nh   = schedule_data_flat[m * 4 + 2];
		uint8_t  nl   = schedule_data_flat[m * 4 + 3];
		uint16_t seed = ((uint16_t)rng1 << 8) | (uint16_t)rng2;
		for (int s = 0; s < 2048; s++)
		{
			uint16_t next = rng->rng_table.ptr[seed];
			map_rng_h[m * 2048 + s] = (uint8_t)(((next >> 8) ^ next) & 0xFF);
			seed = next;
		}
		nibble_sel_h[m] = ((uint16_t)nh << 8) | (uint16_t)nl;
	}

	cl_mem map_rng_d   = _cl->create_readonly_buffer(schedule_count * 2048);
	cl_mem nibble_d    = _cl->create_readonly_buffer(schedule_count * sizeof(uint16_t));
	cl_mem exp_d       = _cl->create_readonly_buffer(32 * sizeof(uint32_t));
	cl_mem out_d       = _cl->create_readwrite_buffer(128);

	_cl->copy_mem_to_device(map_rng_d,  map_rng_h,   schedule_count * 2048);
	_cl->copy_mem_to_device(nibble_d,   nibble_sel_h, schedule_count * sizeof(uint16_t));
	delete[] map_rng_h;
	delete[] nibble_sel_h;

	for (uint32 i = 0; i < count; i++)
	{
		uint32_t key  = ((uint32_t)keys [i*4+0] << 24) | ((uint32_t)keys [i*4+1] << 16)
		              | ((uint32_t)keys [i*4+2] <<  8) |  (uint32_t)keys [i*4+3];
		uint32_t data = ((uint32_t)datas[i*4+0] << 24) | ((uint32_t)datas[i*4+1] << 16)
		              | ((uint32_t)datas[i*4+2] <<  8) |  (uint32_t)datas[i*4+3];

		uint32_t expansion_h[32] = {};
		build_expansion(rng, key, expansion_h);
		_cl->copy_mem_to_device(exp_d, expansion_h, 32 * sizeof(uint32_t));

		cl_kernel k = _kernel_test_pipeline;
		set_kernel_arg<uint32_t>(k, 0, &key);
		set_kernel_arg<uint32_t>(k, 1, &data);
		set_kernel_arg<cl_mem>  (k, 2, &exp_d);
		set_kernel_arg<cl_mem>  (k, 3, &map_rng_d);
		set_kernel_arg<cl_mem>  (k, 4, &nibble_d);
		set_kernel_arg<int>     (k, 5, &schedule_count);
		set_kernel_arg<cl_mem>  (k, 6, &out_d);

		size_t gs[3] = { 32, 1, 1 };
		size_t ls[3] = { 32, 1, 1 };
		cl_event ev = _cl->run_kernel(k, 3, NULL, gs, ls);
		clWaitForEvents(1, &ev);
		clReleaseEvent(ev);

		_cl->copy_mem_from_device(out_d, outputs + i * 128, 128);
	}

	clReleaseMemObject(map_rng_d);
	clReleaseMemObject(nibble_d);
	clReleaseMemObject(exp_d);
	clReleaseMemObject(out_d);
}
