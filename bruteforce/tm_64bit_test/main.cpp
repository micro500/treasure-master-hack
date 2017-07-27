#include <stdio.h>
#include <chrono>
#include <iostream>
#include "data_sizes.h"
#include "tm_64bit_test.h"


int main()
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;


	uint8 test_case[128+128+3+2];

	FILE * pFile;

	pFile = fopen ("TM_test_cases_OUT.txt","r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return 0;
	}

	tm_64bit_test tester;
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
		for (int j = 0; j < 128; j++)
		{
			if (test_data[j] != test_case[3+128+2+j])
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
			printf("Test %i (alg %i): --FAIL--\n",test_case[0],j);
			all_tests_passed = false;
		}
		/*
		else
		{
			printf("Test %i: Pass\n",j);
		}
		*/
	}
	fclose (pFile);

	if (all_tests_passed)
	{
		printf("All tests passed!\n");
	}

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
		tester.run_iterations(test_data,&rng_seed,i,10000000);
		auto end = clock::now();
		std::cout << duration_cast<microseconds>(end-start).count() << "us\n";
	}

}