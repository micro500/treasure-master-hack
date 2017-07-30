#ifndef TM_64_8_TEST_H
#define TM_64_8_TEST_H
#include "data_sizes.h"

class tm_64_8_test
{
public:
	tm_64_8_test();

	void process_test_case(uint8 * test_case, uint16 *rng_seed, int algorithm);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);

	uint16 * rng_table;
	uint16 rng_seed;

	uint8 * regular_rng_values;
	uint8 * regular_rng_values_lo;
	uint8 * regular_rng_values_hi;

	uint8 * alg0_values;

	uint8 * alg6_values;

	uint8 * alg4_values_lo;
	uint8 * alg4_values_hi;

	uint64 * alg2_values_64;

	uint64 * alg5_values_64;

	uint16 *rng_seed_forward_1;
	uint16 *rng_seed_forward_128;

};

#endif //TM_64_8_TEST_H