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

	alg2_values = new uint32[0x10000];
	generate_alg2_values_32_8(alg2_values, rng_table);

	alg5_values = new uint32[0x10000];
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

	if (algorithm == 0)
	{
		alg0((uint32*)working_code, (uint32*)alg0_values, *rng_seed);
		*rng_seed = rng_seed_forward_128[*rng_seed];
	}
	else if (algorithm == 1)
	{
		alg1((uint32*)working_code, (uint32*)regular_rng_values_lo, (uint32*)regular_rng_values_hi, *rng_seed);
		*rng_seed = rng_seed_forward_128[*rng_seed];
	}
	else if (algorithm == 2)
	{
		alg2((uint32*)working_code, (uint32*)alg2_values, *rng_seed);
			*rng_seed = rng_seed_forward_1[*rng_seed];
	}
	else if (algorithm == 3)
	{
		alg3((uint32*)working_code, (uint32*)regular_rng_values, *rng_seed);
		*rng_seed = rng_seed_forward_128[*rng_seed];
	}
	else if (algorithm == 4)
	{
		alg1((uint32*)working_code, (uint32*)alg4_values_lo, (uint32*)alg4_values_hi, *rng_seed);
		*rng_seed = rng_seed_forward_128[*rng_seed];
	}
	else if (algorithm == 5)
	{
		alg5((uint32*)working_code, (uint32*)alg5_values, *rng_seed);
		*rng_seed = rng_seed_forward_1[*rng_seed];
	}
	else if (algorithm == 6)
	{
		alg6((uint32*)working_code, (uint32*)alg6_values, *rng_seed);
		*rng_seed = rng_seed_forward_128[*rng_seed];
	}
	else if (algorithm == 7)
	{
		alg7((uint32*)working_code);
	}

	for (int i = 0; i < 128; i++)
	{
		test_case[i] = working_code[i];
	}
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
			alg0((uint32*)working_code, (uint32*)alg0_values, *rng_seed);
			*rng_seed = rng_seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm == 1)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg1((uint32*)working_code, (uint32*)regular_rng_values_lo, (uint32*)regular_rng_values_hi, *rng_seed);
			*rng_seed = rng_seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm == 2)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg2((uint32*)working_code, (uint32*)alg2_values, *rng_seed);
			*rng_seed = rng_seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm == 3)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg3((uint32*)working_code, (uint32*)regular_rng_values, *rng_seed);
			*rng_seed = rng_seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm == 4)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg1((uint32*)working_code, (uint32*)alg4_values_lo, (uint32*)alg4_values_hi, *rng_seed);
			*rng_seed = rng_seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm == 5)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg5((uint32*)working_code, (uint32*)alg5_values, *rng_seed);
			*rng_seed = rng_seed_forward_1[*rng_seed];
		}
	}
	else if (algorithm == 6)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg6((uint32*)working_code, (uint32*)alg6_values, *rng_seed);
			*rng_seed = rng_seed_forward_128[*rng_seed];
		}
	}
	else if (algorithm == 7)
	{
		for (int i = 0; i < iterations; i++)
		{
			alg7((uint32*)working_code);
		}
	}
}