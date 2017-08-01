#ifndef TM_64_16_TEST_H
#define TM_64_16_TEST_H
#include "data_sizes.h"

class tm_64_16_test
{
public:
	tm_64_16_test();

	void process_test_case(uint8 * test_case, uint16 *rng_seed, int algorithm);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);

	uint16 * rng_table;
	uint16 rng_seed;

	uint16 * regular_rng_values;

	uint16 * alg0_values;

	uint16 * alg6_values;

	uint16 * alg4_values;

	uint64 * alg2_values;

	uint64 * alg5_values;

	uint16 *rng_seed_forward_1;
	uint16 *rng_seed_forward_128;

};

#endif //TM_64_16_TEST_H