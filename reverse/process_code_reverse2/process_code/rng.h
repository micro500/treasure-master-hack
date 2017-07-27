#ifndef RNG_H
#define RNG_H
#include <vector>

#include "data_sizes.h"

extern uint16 rng_table[0x100][0x100];

unsigned char rng_real(unsigned char *rng1, unsigned char *rng2);

void generate_rng_table();

class RNG
{
public:
	RNG() : rng1(0), rng2(0) {};
	RNG(uint8 a, uint8 b) : rng1(a), rng2(b) {};
	
	uint8 rng1;
	uint8 rng2;

	uint8 run();
	uint8 run_back(int index);
	void seed(uint8 rng1, uint8 rng2);

	bool error;
};

struct reverse_rng_seed_info
{
	// The seed at the end of the processing
	uint8 rng1;
	uint8 rng2;

	// The branch option, only really matters if the seed will hit 0xAA6D (the branch point)
	// 0 will return to the loop
	// 1 will head towards 0x2143
	uint8 branch_option;

	// The reversed RNG table for those initial conditions
	uint8 * table;

	// If the initial conditions will hit 2143 not all the values in the table will be useful
	// This value tells how many are useable
	int table_length;

	// Number of bytes needed from this seed to make it useful
	// Ex: if you are 3 from the branch in Z1 and you only need 2 bytes, you would get the same results from Z0
	int useful_bytes;
};

extern std::vector<reverse_rng_seed_info> reverse_rng;

bool reverse_rng_seed_sort(reverse_rng_seed_info a, reverse_rng_seed_info b);

#endif //RNG_H