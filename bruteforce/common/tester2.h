#ifndef TM_TESTER2_H
#define TM_TESTER2_H

#include <chrono>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "data_sizes.h"
#include "tm_8.h"
#include "tm_tester.h"

void run_load_fetch_tests(tm_tester tester)
{
	bool all_tests_passed = true;
	uint8 test_data_in[128];
	uint8 test_data_out[128];
	srand(time(NULL));

	for (int j = 0; j < 100000; j++)
	{
		for (int k = 0; k < 128; k++)
		{
			test_data_in[k] = rand() % 0x100;
		}

		tester.process_load_fetch(test_data_in, test_data_out);

		for (int k = 0; k < 128; k++)
		{
			if (test_data_in[k] != test_data_out[k])
			{
				printf("-- Load/fetch tests FAILED. --\n");
				return;
			}
		}
	}

	if (all_tests_passed)
	{
		printf("Load/fetch tests passed.\n");
	}
}

void run_alg_validity_tests(tm_tester tester)
{
	uint8 test_case[128 + 128 + 3 + 2];

	FILE * pFile;

	pFile = fopen("../common/TM_alg_test_cases4.txt", "r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return;
	}

	bool all_tests_passed = true;
	for (int j = 0; j < 1326336; j++)
	{
		for (int i = 0; i < 128 + 128 + 1 + 2 + 2; i++)
		{
			int val;
			fscanf(pFile, "%i,", &val);
			test_case[i] = val;
		}

		uint16 rng_seed = (test_case[1] << 8) | test_case[2];
		uint16 output_rng_seed = (test_case[3 + 128] << 8) | test_case[3 + 128 + 1];

		uint8 test_data[128];
		for (int i = 0; i < 128; i++)
		{
			test_data[i] = test_case[3 + i];
		}

		tester.process_alg_test_case(test_data, &rng_seed, test_case[0]);

		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (test_data[i] != test_case[3 + 128 + 2 + i])
			{
				matching = 0;
				break;
			}
		}

		if (rng_seed != output_rng_seed)
		{
			matching = 0;
		}

		if (matching == 0)
		{
			printf("Alg test %i (alg %i): --FAIL--\n", j, test_case[0]);
			all_tests_passed = false;
		}
	}
	fclose(pFile);

	if (all_tests_passed)
	{
		printf("Alg tests passed.\n");
	}
}

void run_checksum_tests(tm_tester tester)
{
	uint8 test_case[128 + 128 + 3 + 2];

	FILE* pFile;

	pFile = fopen("../common/TM_checksum_test_cases.txt", "r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return;
	}

	bool all_tests_passed = true;
	for (int j = 0; j < 200512; j++)
	{
		for (int i = 0; i < 1 + 128 + 2 + 2; i++)
		{
			int val;
			fscanf(pFile, "%i,", &val);
			test_case[i] = val;
		}

		uint8 world = test_case[0];
		uint8 test_data[128];
		for (int i = 0; i < 128; i++)
		{
			test_data[i] = test_case[1 + i];
		}

		uint16 checksum = (test_case[1 + 128 + 1] << 8) | test_case[1 + 128];
		uint16 checksum_value = (test_case[1 + 128 + 3] << 8) | test_case[1 + 128 + 2];
		//if (world != 1) continue;

		uint16 result_checksum = tester.calculate_checksum(test_data, world);
		uint16 result_checksum_value = tester.fetch_checksum_value(test_data, world);

		if (result_checksum != checksum)
		{
			printf("Checksum test %i (world %i): --FAIL: Checksum--\n", j, test_case[0]);
			all_tests_passed = false;
		}

		if (result_checksum_value != checksum_value)
		{
			printf("Checksum test %i (world %i): --FAIL: Checksum Value--\n", j, test_case[0]);
			all_tests_passed = false;
		}
	}
	fclose(pFile);

	if (all_tests_passed)
	{
		printf("Checksum tests passed.\n");
	}
}


void run_expansion_validity_tests(tm_tester tester)
{
	uint8 test_case[8 + 128];

	FILE* pFile;

	pFile = fopen("../common/TM_expansion_test_cases4.txt", "r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return;
	}

	uint8 result_data[128];
	bool all_tests_passed = true;
	for (int j = 0; j < 1000000; j++)
	{
		for (int i = 0; i < 8 + 128; i++)
		{
			int val;
			fscanf(pFile, "%i,", &val);
			test_case[i] = val;
		}

		uint32 key = (test_case[0] << 24) | (test_case[1] << 16) | (test_case[2] << 8) | test_case[3];
		uint32 data = (test_case[4] << 24) | (test_case[5] << 16) | (test_case[6] << 8) | test_case[7];

		tester.run_expansion(key, data, result_data);

		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (result_data[i] != test_case[8 + i])
			{
				matching = 0;
				break;
			}
		}

		if (matching == 0)
		{
			printf("Expansion test %i: --FAIL--\n", j);
			all_tests_passed = false;
		}
	}
	fclose(pFile);

	if (all_tests_passed)
	{
		printf("Expansion tests passed.\n");
	}
}

void run_full_validity_tests(tm_tester tester)
{
	uint8 test_case[8 + 128];
	int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11 };

	FILE* pFile;

	pFile = fopen("../common/TM_full_test_cases.txt", "r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return;
	}

	uint8 result_data[128];
	bool all_tests_passed = true;
	for (int j = 0; j < 10000; j++)
	{
		for (int i = 0; i < 8 + 128; i++)
		{
			int val;
			fscanf(pFile, "%x,", &val);
			test_case[i] = val;
		}

		uint32 key = (test_case[0] << 24) | (test_case[1] << 16) | (test_case[2] << 8) | test_case[3];
		uint32 data = (test_case[4] << 24) | (test_case[5] << 16) | (test_case[6] << 8) | test_case[7];

		key_schedule_data schedule_data;
		schedule_data.as_uint8[0] = (key >> 24) & 0xFF;
		schedule_data.as_uint8[1] = (key >> 16) & 0xFF;
		schedule_data.as_uint8[2] = (key >> 8) & 0xFF;
		schedule_data.as_uint8[3] = key & 0xFF;

		key_schedule_entry schedule_entries[27];

		int schedule_counter = 0;
		for (int i = 0; i < 26; i++)
		{
			schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i], &schedule_data);

			if (map_list[i] == 0x22)
			{
				schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i], &schedule_data, 4);
			}
		}

		tester.run_full_process(key, data, schedule_entries, result_data);

		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (result_data[i] != test_case[8 + i])
			{
				matching = 0;
				break;
			}
		}

		if (matching == 0)
		{
			printf("Full test %i: --FAIL--\n", j);
			all_tests_passed = false;
		}
	}
	fclose(pFile);

	if (all_tests_passed)
	{
		printf("Full tests passed.\n");
	}
}

void run_speed_tests2(tm_tester tester, int iterations)
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;

	uint8 test_case[128 + 128 + 3 + 2] = { 7,128,174,117,63,86,170,5,184,105,5,221,0,73,188,83,122,82,161,96,193,97,208,110,165,7,11,12,168,126,88,96,82,67,99,237,184,118,218,144,202,92,185,150,57,236,32,147,150,224,31,107,111,253,45,198,12,238,187,216,179,96,245,174,101,168,148,45,10,189,81,170,245,42,29,27,156,126,171,238,108,178,171,121,99,105,66,37,85,210,150,171,188,7,211,33,63,88,42,223,224,116,16,127,194,24,137,234,150,168,121,104,164,67,148,64,249,67,137,184,166,206,116,101,171,162,39,27,24,41,17,128,174,138,192,169,85,250,71,150,250,34,255,182,67,172,133,173,94,159,62,158,47,145,90,248,244,243,87,129,167,159,173,188,156,18,71,137,37,111,53,163,70,105,198,19,223,108,105,31,224,148,144,2,210,57,243,17,68,39,76,159,10,81,154,87,107,210,245,66,174,85,10,213,226,228,99,129,84,17,147,77,84,134,156,150,189,218,170,45,105,84,67,248,44,222,192,167,213,32,31,139,239,128,61,231,118,21,105,87,134,151,91,188,107,191,6,188,118,71,89,49,139,154,84,93,216,228,231,214,238 };

	for (int i = 0; i < 8; i++)
	{
		uint16 rng_seed = (test_case[1] << 8) | test_case[2];
		uint16 output_rng_seed = (test_case[3 + 128] << 8) | test_case[3 + 128 + 1];

		uint8 test_data[128];
		for (int j = 0; j < 128; j++)
		{
			test_data[j] = test_case[3 + j];
		}

		auto start = clock::now();
		tester.run_iterations(test_data, &rng_seed, i, iterations);
		auto end = clock::now();
		std::cout << i << " - " << duration_cast<microseconds>(end - start).count() << "\n";
	}
}

void run_full_speed_test(tm_tester tester, int iterations)
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;

	uint32 key = 0x2CA5B42D;

	auto start = clock::now();
	for (int i = 0; i < iterations; i++)
	{
		tester.run_full_process(key, i);
	}
	auto end = clock::now();
	std::cout << duration_cast<microseconds>(end - start).count() << "\n";
}

#endif //TM_TESTER2_H