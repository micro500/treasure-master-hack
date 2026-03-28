#pragma once
// GPU counterpart to tester2.h.
// Runs the same test case files against a tm_gpu_tester using batched
// GPU dispatches — many test cases per kernel launch instead of one.

#include <stdio.h>
#include <time.h>
#include <chrono>
#include "data_sizes.h"
#include "key_schedule.h"
#include "tm_gpu_tester.h"

// Number of test cases packed into one GPU dispatch.
// Must be <= TM_GPU_base implementation's internal TEST_BATCH_SIZE.
static const uint32_t GPU_TEST_BATCH = 1u << 14;  // 16384

void run_expansion_validity_tests_gpu(tm_gpu_tester tester)
{
	// Per-batch host-side arrays
	static uint8 keys_h  [GPU_TEST_BATCH * 4];
	static uint8 datas_h [GPU_TEST_BATCH * 4];
	static uint8  outputs_h  [GPU_TEST_BATCH * 128];
	static uint8  expected_h [GPU_TEST_BATCH * 128];

	fprintf(stderr, "Expansion tests: opening file...\n");
	FILE* pFile = fopen("../../../../../tm/TM_expansion_test_cases4.bin", "rb");
	if (pFile == NULL) { fprintf(stderr, "File error: TM_expansion_test_cases4.bin not found\n"); return; }
	fprintf(stderr, "Expansion tests: file opened, running %d cases in batches of %d...\n", 1000000, (int)GPU_TEST_BATCH);

	bool all_passed  = true;
	int  total       = 1000000;
	int  done        = 0;
	double t_read = 0, t_gpu = 0, t_cmp = 0;

	while (done < total)
	{
		int batch = total - done;
		if (batch > (int)GPU_TEST_BATCH) batch = (int)GPU_TEST_BATCH;

		static uint8 file_buf[GPU_TEST_BATCH * 136];
		clock_t t0 = clock();
		fread(file_buf, 136, batch, pFile);
		for (int j = 0; j < batch; j++)
		{
			uint8* tc = file_buf + j * 136;
			keys_h [j*4+0] = tc[0]; keys_h [j*4+1] = tc[1]; keys_h [j*4+2] = tc[2]; keys_h [j*4+3] = tc[3];
			datas_h[j*4+0] = tc[4]; datas_h[j*4+1] = tc[5]; datas_h[j*4+2] = tc[6]; datas_h[j*4+3] = tc[7];
			for (int i = 0; i < 128; i++) expected_h[j * 128 + i] = tc[8 + i];
		}

		clock_t t1 = clock();
		tester.run_expansion_batch(keys_h, datas_h, outputs_h, (uint32)batch);

		clock_t t2 = clock();
		for (int j = 0; j < batch; j++)
		{
			for (int i = 0; i < 128; i++)
			{
				if (outputs_h[j * 128 + i] != expected_h[j * 128 + i])
				{
					fprintf(stderr, "Expansion test %i: --FAIL--\n", done + j);
					all_passed = false;
					break;
				}
			}
		}
		clock_t t3 = clock();

		t_read += (double)(t1 - t0) / CLOCKS_PER_SEC;
		t_gpu  += (double)(t2 - t1) / CLOCKS_PER_SEC;
		t_cmp  += (double)(t3 - t2) / CLOCKS_PER_SEC;

		done += batch;
		fprintf(stderr, "  %d / %d  (read=%.2fs  gpu=%.2fs  cmp=%.2fs)\n", done, total, t_read, t_gpu, t_cmp);
	}
	fclose(pFile);
	fprintf(stderr, "Expansion tests: %s\n", all_passed ? "PASSED" : "FAILED");
}

void run_alg_validity_tests_gpu(tm_gpu_tester tester)
{
	static uint8  alg_ids_h       [GPU_TEST_BATCH];
	static uint16 rng_seeds_in_h  [GPU_TEST_BATCH];
	static uint16 rng_seeds_out_h [GPU_TEST_BATCH];
	static uint8  inputs_h        [GPU_TEST_BATCH * 128];
	static uint8  outputs_h       [GPU_TEST_BATCH * 128];
	static uint16 expected_seeds_h[GPU_TEST_BATCH];
	static uint8  expected_out_h  [GPU_TEST_BATCH * 128];

	fprintf(stderr, "Alg tests: opening file...\n");
	FILE* pFile = fopen("../../../../../tm/TM_alg_test_cases4.bin", "rb");
	if (pFile == NULL) { fprintf(stderr, "File error: TM_alg_test_cases4.bin not found\n"); return; }
	fprintf(stderr, "Alg tests: file opened, running %d cases in batches of %d...\n", 1326336, (int)GPU_TEST_BATCH);

	static const int ALG_FILTER = -1;  // set to 0-7 to test only that algorithm, -1 for all

	bool all_passed = true;
	int  total      = 1326336;
	int  done       = 0;

	while (done < total)
	{
		int batch = total - done;
		if (batch > (int)GPU_TEST_BATCH) batch = (int)GPU_TEST_BATCH;

		static uint8 file_buf[GPU_TEST_BATCH * 261];
		fread(file_buf, 261, batch, pFile);
		int filtered_batch = 0;
		for (int j = 0; j < batch; j++)
		{
			// Format: alg(1), seed_hi, seed_lo, input[128], out_seed_hi, out_seed_lo, output[128]
			uint8* tc = file_buf + j * 261;
			if (ALG_FILTER != -1 && tc[0] != ALG_FILTER) continue;
			int k = filtered_batch++;
			alg_ids_h     [k] = tc[0];
			rng_seeds_in_h[k] = ((uint16)tc[1] << 8) | tc[2];
			for (int i = 0; i < 128; i++) inputs_h[k * 128 + i] = tc[3 + i];
			expected_seeds_h[k] = ((uint16)tc[3 + 128] << 8) | tc[3 + 128 + 1];
			for (int i = 0; i < 128; i++) expected_out_h[k * 128 + i] = tc[3 + 128 + 2 + i];
		}

		if (filtered_batch > 0)
		tester.process_alg_test_case_batch(
		    alg_ids_h, rng_seeds_in_h,
		    inputs_h, outputs_h, rng_seeds_out_h, (uint32)filtered_batch);

		for (int j = 0; j < filtered_batch; j++)
		{
			bool ok = (rng_seeds_out_h[j] == expected_seeds_h[j]);
			if (ok)
			{
				for (int i = 0; i < 128; i++)
				{
					if (outputs_h[j * 128 + i] != expected_out_h[j * 128 + i])
					{
						ok = false;
						break;
					}
				}
			}
			if (!ok)
			{
				fprintf(stderr, "Alg test %i (alg %i): --FAIL--\n", done + j, alg_ids_h[j]);
				all_passed = false;
				break;
			}
		}

		done += batch;
		fprintf(stderr, "  %d / %d\n", done, total);
	}
	fclose(pFile);
	fprintf(stderr, "Alg tests: %s\n", all_passed ? "PASSED" : "FAILED");
}

void run_full_validity_tests_gpu(tm_gpu_tester tester)
{
	static uint8 keys_h  [GPU_TEST_BATCH * 4];
	static uint8 datas_h [GPU_TEST_BATCH * 4];
	static uint8  sched_h [GPU_TEST_BATCH * 27 * 4];
	static uint8  outputs_h  [GPU_TEST_BATCH * 128];
	static uint8  expected_h [GPU_TEST_BATCH * 128];

	fprintf(stderr, "Full tests: opening file...\n");
	FILE* pFile = fopen("../../../../../tm/TM_full_test_cases.txt", "r+");
	if (pFile == NULL) { fprintf(stderr, "File error: TM_full_test_cases.txt not found\n"); return; }
	fprintf(stderr, "Full tests: file opened, running %d cases in batches of %d...\n", 10000, (int)GPU_TEST_BATCH);

	bool all_passed   = true;
	int  total        = 10000;
	int  done         = 0;
	int  schedule_count = 0;

	while (done < total)
	{
		int batch = total - done;
		if (batch > (int)GPU_TEST_BATCH) batch = (int)GPU_TEST_BATCH;

		for (int j = 0; j < batch; j++)
		{
			uint8 tc[8 + 128];
			for (int i = 0; i < 8 + 128; i++) { int v; fscanf(pFile, "%x,", &v); tc[i] = (uint8)v; }

			keys_h [j*4+0] = tc[0]; keys_h [j*4+1] = tc[1]; keys_h [j*4+2] = tc[2]; keys_h [j*4+3] = tc[3];
			datas_h[j*4+0] = tc[4]; datas_h[j*4+1] = tc[5]; datas_h[j*4+2] = tc[6]; datas_h[j*4+3] = tc[7];
			for (int i = 0; i < 128; i++) expected_h[j * 128 + i] = tc[8 + i];

			uint32 key = ((uint32)tc[0] << 24) | ((uint32)tc[1] << 16) | ((uint32)tc[2] << 8) | tc[3];
			key_schedule sched(key, key_schedule::ALL_MAPS);
			schedule_count = (int)sched.entries.size();
			for (int k = 0; k < schedule_count; k++)
			{
				sched_h[(j * schedule_count + k) * 4 + 0] = sched.entries[k].rng1;
				sched_h[(j * schedule_count + k) * 4 + 1] = sched.entries[k].rng2;
				sched_h[(j * schedule_count + k) * 4 + 2] = (sched.entries[k].nibble_selector >> 8) & 0xFF;
				sched_h[(j * schedule_count + k) * 4 + 3] =  sched.entries[k].nibble_selector       & 0xFF;
			}
		}

		tester.run_full_process_batch(keys_h, datas_h, sched_h, schedule_count, outputs_h, (uint32)batch);

		for (int j = 0; j < batch; j++)
		{
			for (int i = 0; i < 128; i++)
			{
				if (outputs_h[j * 128 + i] != expected_h[j * 128 + i])
				{
					fprintf(stderr, "Full test %i: --FAIL--\n", done + j);
					all_passed = false;
					break;
				}
			}
		}

		done += batch;
		fprintf(stderr, "  %d / %d\n", done, total);
	}
	fclose(pFile);
	fprintf(stderr, "Full tests: %s\n", all_passed ? "PASSED" : "FAILED");
}

void run_result_tests_gpu(tm_gpu_tester tester)
{
	uint8 test_case[16];

	fprintf(stderr, "Result tests: opening file...\n");
	FILE* pFile = fopen("../../../../tm/TM_result_test_cases.txt", "rb");
	if (pFile == NULL) { fprintf(stderr, "File error: TM_result_test_cases.txt not found\n"); return; }
	fprintf(stderr, "Result tests: running 20 cases...\n");

	static uint8 case_result_data[400000];
	static uint8 result_data[400000];
	bool all_passed = true;

	for (int j = 0; j < 20; j++)
	{
		fread(test_case, 1, 16, pFile);

		uint32 key             = ((uint32)test_case[3] << 24) | ((uint32)test_case[2] << 16) | ((uint32)test_case[1] << 8) | test_case[0];
		uint32 data            = ((uint32)test_case[7] << 24) | ((uint32)test_case[6] << 16) | ((uint32)test_case[5] << 8) | test_case[4];
		uint32 amount_to_run   = ((uint32)test_case[11] << 24) | ((uint32)test_case[10] << 16) | ((uint32)test_case[9] << 8) | test_case[8];
		uint32 case_result_size = ((uint32)test_case[15] << 24) | ((uint32)test_case[14] << 16) | ((uint32)test_case[13] << 8) | test_case[12];

		int bytes_read = (int)fread(case_result_data, 1, case_result_size, pFile);
		if (bytes_read != (int)case_result_size)
		{
			printf("File read error. Got: %i Expected: %u\n", bytes_read, case_result_size);
			return;
		}

		key_schedule schedule_data(key, key_schedule::ALL_MAPS);
		uint32 result_size;
		tester.run_results_process(key, data, schedule_data, amount_to_run, result_data, 400000, &result_size);

		if (result_size != case_result_size)
		{
			fprintf(stderr, "Result test %i --FAIL-- size mismatch. Got: %u Expected: %u\n", j, result_size, case_result_size);
			all_passed = false;
			continue;
		}

		bool ok = true;
		for (uint32 i = 0; i < result_size; i++)
		{
			if (result_data[i] != case_result_data[i]) { ok = false; break; }
		}
		if (!ok)
		{
			fprintf(stderr, "Result test %i: --FAIL-- data mismatch\n", j);
			all_passed = false;
		}
	}
	fclose(pFile);
	fprintf(stderr, "Result tests: %s\n", all_passed ? "PASSED" : "FAILED");
}

void run_full_speed_test_gpu(tm_gpu_tester tester, int num_batches = 5)
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;

	static const uint32_t BATCH_SIZE = 1u << 24;  // 1M items per batch
	static uint8 result_data[BATCH_SIZE];

	uint32 key = 0x2CA5B42D;
	key_schedule schedule_data(key, key_schedule::ALL_MAPS);
	uint32 result_size;

	fprintf(stderr, "Speed test: %d batches of %u items\n", num_batches, BATCH_SIZE);

	double total_sec = 0;

	for (int i = 0; i < num_batches; i++)
	{
		uint32 start_data = (uint32)i * BATCH_SIZE;

		auto t0 = clock::now();
		tester.run_results_process(key, start_data, schedule_data, BATCH_SIZE, result_data, BATCH_SIZE, &result_size);
		auto t1 = clock::now();

		double sec = duration_cast<microseconds>(t1 - t0).count() / 1e6;
		total_sec += sec;
		fprintf(stderr, "  Batch %2d: %6.2f M items/sec\n", i, (BATCH_SIZE / 1e6) / sec);
	}

	double avg = ((double)num_batches * BATCH_SIZE / 1e6) / total_sec;
	fprintf(stderr, "  Average:  %6.2f M items/sec\n", avg);
}
