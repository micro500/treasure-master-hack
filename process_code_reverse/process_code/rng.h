#ifndef RNG_H
#define RNG_H
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
	void seed(uint8 rng1, uint8 rng2);
};

#endif //RNG_H