#ifndef TM_TESTER_H
#define TM_TESTER_H

#include <chrono>
#include <iostream>
#include "data_sizes.h"

template<class T>
void run_validity_tests(T tester)
{
	uint8 test_case[128+128+3+2];

	FILE * pFile;

	pFile = fopen ("../common/TM_test_cases_OUT.txt","r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return;
	}

	bool all_tests_passed = true;
	for (int j = 0; j < 80000; j++)
	{
		for (int i = 0; i < 128+128+1+2+2; i++)
		{
			int val;
			fscanf (pFile, "%i,", &val);
			test_case[i] = val;
		}

		uint16 rng_seed = (test_case[1] << 8) | test_case[2];
		uint16 output_rng_seed = (test_case[3+128] << 8) | test_case[3+128+1];

		uint8 test_data[128];
		for (int i = 0; i < 128; i++)
		{
			test_data[i] = test_case[3+i];
		}

		tester.process_test_case(test_data,&rng_seed,test_case[0]);

		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (test_data[i] != test_case[3+128+2+i])
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
			printf("Test %i (alg %i): --FAIL--\n",j,test_case[0]);
			all_tests_passed = false;
		}
	}
	fclose (pFile);

	if (all_tests_passed)
	{
		printf("All tests passed!\n");
	}
}

template<class T>
void run_speed_tests(T tester, int iterations)
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;
  
	uint8 test_case[128+128+3+2] = {7,128,174,117,63,86,170,5,184,105,5,221,0,73,188,83,122,82,161,96,193,97,208,110,165,7,11,12,168,126,88,96,82,67,99,237,184,118,218,144,202,92,185,150,57,236,32,147,150,224,31,107,111,253,45,198,12,238,187,216,179,96,245,174,101,168,148,45,10,189,81,170,245,42,29,27,156,126,171,238,108,178,171,121,99,105,66,37,85,210,150,171,188,7,211,33,63,88,42,223,224,116,16,127,194,24,137,234,150,168,121,104,164,67,148,64,249,67,137,184,166,206,116,101,171,162,39,27,24,41,17,128,174,138,192,169,85,250,71,150,250,34,255,182,67,172,133,173,94,159,62,158,47,145,90,248,244,243,87,129,167,159,173,188,156,18,71,137,37,111,53,163,70,105,198,19,223,108,105,31,224,148,144,2,210,57,243,17,68,39,76,159,10,81,154,87,107,210,245,66,174,85,10,213,226,228,99,129,84,17,147,77,84,134,156,150,189,218,170,45,105,84,67,248,44,222,192,167,213,32,31,139,239,128,61,231,118,21,105,87,134,151,91,188,107,191,6,188,118,71,89,49,139,154,84,93,216,228,231,214,238};

	for (int i = 0; i < 8; i++)
	{
		uint16 rng_seed = (test_case[1] << 8) | test_case[2];
		uint16 output_rng_seed = (test_case[3+128] << 8) | test_case[3+128+1];

		uint8 test_data[128];
		for (int j = 0; j < 128; j++)
		{
			test_data[j] = test_case[3+j];
		}

		auto start = clock::now();
		tester.run_iterations(test_data,&rng_seed,i,iterations);
		auto end = clock::now();
		std::cout << duration_cast<microseconds>(end-start).count() << "\n";
	}
}

#endif //TM_TESTER_H