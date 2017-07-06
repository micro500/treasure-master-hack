#include "../tm_8bit/data_sizes.h"
#include "alignment.h"
#include "tm_avx_intrinsics2_test.h"
#include "tm_avx_intrinsics2.h"

tm_avx_intrinsics2_test::tm_avx_intrinsics2_test()
{
	this->rng_table = new uint16[256*256];
	generate_rng_table(this->rng_table);


	regular_rng_values = (uint16*)aligned_malloc(0x10000 * 128 * 2, 32); // new uint16[0x10000 * 128];
	generate_regular_rng_values(regular_rng_values, &rng_seed, rng_table);

	alg0_values = (uint16*)aligned_malloc(0x10000 * 128 * 2, 32); //new uint16[0x10000 * 128];
	generate_alg0_values(alg0_values, &rng_seed, rng_table);

	alg6_values = (uint16*)aligned_malloc(0x10000 * 128 * 2, 32); //new uint16[0x10000 * 128];
	generate_alg6_values(alg6_values, &rng_seed, rng_table);

	alg2_values_8 = (uint8*)aligned_malloc(0x10000 * 32, 32); //new uint64[0x10000 * 128];
	generate_alg2_values(alg2_values_8, &rng_seed, rng_table);

	alg5_values_8 = (uint8*)aligned_malloc(0x10000 * 32, 32); //new uint64[0x10000 * 128];
	generate_alg5_values(alg5_values_8, &rng_seed, rng_table);

	rng_seed_forward_1 = new uint16[256*256];
	generate_seed_forward_1(rng_seed_forward_1, rng_table);

	rng_seed_forward_128 = new uint16[256*256];
	generate_seed_forward_128(rng_seed_forward_128, &rng_seed, rng_table);
}

void tm_avx_intrinsics2_test::process_test_case(uint8 * test_case, uint16 * rng_seed, int algorithm)
{
	ALIGNED(32) uint16 working_code[128];

	for (int i = 0; i < 128; i++)
	{
		working_code[i] = test_case[i];
	}

	this->rng_seed = *rng_seed;

	run_alg(algorithm,1,(uint8*)working_code,(uint8*)this->regular_rng_values,(uint8*)this->alg0_values,this->alg2_values_8,this->alg5_values_8,(uint8*)this->alg6_values,&(this->rng_seed), rng_seed_forward_1, rng_seed_forward_128);

	for (int i = 0; i < 128; i++)
	{
		test_case[i] = working_code[i];
	}

	*rng_seed = this->rng_seed;
}

void tm_avx_intrinsics2_test::run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations)
{
	ALIGNED(32) uint16 working_code[128];

	for (int i = 0; i < 128; i++)
	{
		working_code[i] = test_case[i];
	}

	this->rng_seed = *rng_seed;

	run_alg(algorithm,iterations,(uint8*)working_code,(uint8*)this->regular_rng_values,(uint8*)this->alg0_values,this->alg2_values_8,this->alg5_values_8,(uint8*)this->alg6_values,&(this->rng_seed), rng_seed_forward_1, rng_seed_forward_128);
}