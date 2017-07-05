#ifndef TM_AVX_INTRINSIC_TEST_H
#define TM_AVX_INTRINSIC_TEST_H
#include "..\tm_8bit\data_sizes.h"

class tm_avx_intrinsics_test
{
public:
	tm_avx_intrinsics_test();

	void process_test_case(uint8 * test_case, uint16 *rng_seed, int algorithm);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);

	uint16 * rng_table;
	uint16 rng_seed;

	uint16 * regular_rng_values;

	uint16 * alg0_values;
	
	uint16 * alg6_values;

	uint8 * alg2_values_8;
	uint8 * alg5_values_8;

	uint16 *rng_seed_forward_1;
	uint16 *rng_seed_forward_128;

};

#endif //TM_AVX_INTRINSIC_TEST_H