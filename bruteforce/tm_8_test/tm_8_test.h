#ifndef TM_8_TEST_H
#define TM_8_TEST_H
#include "data_sizes.h"

class tm_8_test
{
public:
	// Seeded with 8 uint8 values (4 key and 4 data bytes)
	tm_8_test();

	void process_test_case(uint8 * test_case, uint16 *rng_seed, int algorithm);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);

	uint16 * rng_table;
	uint16 rng_seed;


};

#endif //TM_8_TEST_H