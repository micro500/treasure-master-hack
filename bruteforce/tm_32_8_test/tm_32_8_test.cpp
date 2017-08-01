#include "data_sizes.h"
#include "tm_32_8_test.h"
#include "tm_32_8.h"
#include "rng.h"

tm_32_8_test::tm_32_8_test()
{
	this->rng_table = new uint16[256*256];
	generate_rng_table(this->rng_table);

	regular_rng_values = new uint8[0x10000 * 128];
	generate_regular_rng_values_8(regular_rng_values, rng_table);

	regular_rng_values_lo = new uint8[0x10000 * 128];
	generate_regular_rng_values_8_lo(regular_rng_values_lo, rng_table);

	regular_rng_values_hi = new uint8[0x10000 * 128];
	generate_regular_rng_values_8_hi(regular_rng_values_hi, rng_table);

	alg0_values = new uint8[0x10000 * 128];
	generate_alg0_values_8(alg0_values, rng_table);

	alg4_values_lo = new uint8[0x10000 * 128];
	generate_alg4_values_8_lo(alg4_values_lo, rng_table);
	alg4_values_hi = new uint8[0x10000 * 128];
	generate_alg4_values_8_hi(alg4_values_hi, rng_table);

	alg6_values = new uint8[0x10000 * 128];
	generate_alg6_values_8(alg6_values, rng_table);

	alg2_values = new uint32[0x10000 * 128];
	generate_alg2_values_32_8(alg2_values, rng_table);

	alg5_values = new uint32[0x10000 * 128];
	generate_alg5_values_32_8(alg5_values, rng_table);

	rng_seed_forward_1 = new uint16[256*256];
	generate_seed_forward_1(rng_seed_forward_1, rng_table);

	rng_seed_forward_128 = new uint16[256*256];
	generate_seed_forward_128(rng_seed_forward_128, rng_table);
}

void tm_32_8_test::process_test_case(uint8 * test_case, uint16 * rng_seed, int algorithm)
{
	uint8 working_code[128];

	for (int i = 0; i < 128; i++)
	{
		working_code[i] = test_case[i];
	}

	this->rng_seed = *rng_seed;

	if (algorithm == 0)
	{
		TM_32_8_ALG0((this->rng_seed))
	}
	else if (algorithm == 1)
	{
		TM_32_8_ALG1(regular_rng_values_lo, regular_rng_values_hi, (this->rng_seed))
	}
	else if (algorithm == 2)
	{
		TM_32_8_ALG2((this->rng_seed))
	}
	else if (algorithm == 3)
	{
		TM_32_8_ALG3((this->rng_seed))
	}
	else if (algorithm == 4)
	{
		TM_32_8_ALG1(alg4_values_lo, alg4_values_hi, (this->rng_seed))
	}
	else if (algorithm == 5)
	{
		TM_32_8_ALG5((this->rng_seed))
	}
	else if (algorithm == 6)
	{
		TM_32_8_ALG6((this->rng_seed))
	}
	else if (algorithm == 7)
	{
		TM_32_8_ALG7
	}

	for (int i = 0; i < 128; i++)
	{
		test_case[i] = working_code[i];
	}

	*rng_seed = this->rng_seed;
}

void tm_32_8_test::run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations)
{
	uint16 working_code[128];

	for (int i = 0; i < 128; i++)
	{
		working_code[i] = test_case[i];
	}

	this->rng_seed = *rng_seed;


	if (algorithm == 0)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG0((this->rng_seed))
		}
	}
	else if (algorithm == 1)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG1(regular_rng_values_lo, regular_rng_values_hi, (this->rng_seed))
		}
	}
	else if (algorithm == 2)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG2((this->rng_seed))
		}
	}
	else if (algorithm == 3)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG3((this->rng_seed))
		}
	}
	else if (algorithm == 4)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG1(alg4_values_lo, alg4_values_hi, (this->rng_seed))
		}
	}
	else if (algorithm == 5)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG5((this->rng_seed))
		}
	}
	else if (algorithm == 6)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG6((this->rng_seed))
		}
	}
	else if (algorithm == 7)
	{
		for (int i = 0; i < iterations; i++)
		{
			TM_32_8_ALG7
		}
	}
}