#ifndef TM_32BIT_TEST_H
#define TM_32BIT_TEST_H
#include "data_sizes.h"

class tm_32bit_test
{
public:
	// Seeded with 8 uint8 values (4 key and 4 data bytes)
	tm_32bit_test();

	void process_test_case(uint8 * test_case, uint16 *rng_seed, int algorithm);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);

	uint16 * rng_table;
	uint16 rng_seed;

	uint16 * regular_rng_values;

	uint16 * alg0_values;

	uint16 * alg6_values;

	uint32 * alg2_values_32;

	uint32 * alg5_values_32;

	uint16 *rng_seed_forward_1;
	uint16 *rng_seed_forward_128;

};

#endif //TM_32BIT_TEST_H