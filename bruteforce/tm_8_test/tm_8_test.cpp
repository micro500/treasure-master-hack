#include "data_sizes.h"
#include "tm_8_test.h"
#include "tm_8.h"
#include "rng.h"

tm_8_test::tm_8_test()
{
	rng_table = new uint16[256*256];
	generate_rng_table(this->rng_table);
}

void tm_8_test::process_test_case(uint8 * test_case, uint16 * rng_seed, int algorithm)
{
	uint8 working_code[128];

	for (int i = 0; i < 128; i++)
	{
		working_code[i] = test_case[i];
	}

	this->rng_seed = *rng_seed;

	if (algorithm == 0)
	{
		working_code_alg_0(working_code, &(this->rng_seed), this->rng_table);
	}
	else if (algorithm == 1)
	{
		working_code_alg_1(working_code, &(this->rng_seed), this->rng_table);
	}
	else if (algorithm == 2)
	{
		working_code_alg_2(working_code, &(this->rng_seed), this->rng_table);
	}
	else if (algorithm == 3)
	{
		working_code_alg_3(working_code, &(this->rng_seed), this->rng_table);
	}
	else if (algorithm == 4)
	{
		working_code_alg_4(working_code, &(this->rng_seed), this->rng_table);
	}
	else if (algorithm == 5)
	{
		working_code_alg_5(working_code, &(this->rng_seed), this->rng_table);
	}
	else if (algorithm == 6)
	{
		working_code_alg_6(working_code, &(this->rng_seed), this->rng_table);
	}
	else if (algorithm == 7)
	{
		working_code_alg_7(working_code, &(this->rng_seed), this->rng_table);
	}

	for (int i = 0; i < 128; i++)
	{
		test_case[i] = working_code[i];
	}

	*rng_seed = this->rng_seed;
}

void tm_8_test::run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations)
{
	uint8 working_code[128];

	for (int i = 0; i < 128; i++)
	{
		working_code[i] = test_case[i];
	}

	this->rng_seed = *rng_seed;


	if (algorithm == 0)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_0(working_code, &(this->rng_seed), this->rng_table);
		}
	}
	else if (algorithm == 1)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_1(working_code, &(this->rng_seed), this->rng_table);
		}
	}
	else if (algorithm == 2)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_2(working_code, &(this->rng_seed), this->rng_table);
		}
	}
	else if (algorithm == 3)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_3(working_code, &(this->rng_seed), this->rng_table);
		}
	}
	else if (algorithm == 4)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_4(working_code, &(this->rng_seed), this->rng_table);
		}
	}
	else if (algorithm == 5)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_5(working_code, &(this->rng_seed), this->rng_table);
		}
	}
	else if (algorithm == 6)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_6(working_code, &(this->rng_seed), this->rng_table);
		}
	}
	else if (algorithm == 7)
	{
		for (int i = 0; i < iterations; i++)
		{
			working_code_alg_7(working_code, &(this->rng_seed), this->rng_table);
		}
	}
}