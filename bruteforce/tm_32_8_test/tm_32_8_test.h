#ifndef TM_32_8_TEST_H
#define TM_32_8_TEST_H
#include "data_sizes.h"

class tm_32_8_test
{
public:
	// Seeded with 8 uint8 values (4 key and 4 data bytes)
	tm_32_8_test();

	void process_test_case(uint8 * test_case, uint16 *rng_seed, int algorithm);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);

	uint16 * rng_table;
	uint16 rng_seed;

	uint8 * regular_rng_values;
	uint8 * regular_rng_values_lo;
	uint8 * regular_rng_values_hi;

	uint8 * alg0_values;

	uint8 * alg4_values_lo;
	uint8 * alg4_values_hi;

	uint8 * alg6_values;

	uint32 * alg2_values;

	uint32 * alg5_values;

	uint16 *rng_seed_forward_1;
	uint16 *rng_seed_forward_128;

};

#endif //TM_32_8_TEST_H