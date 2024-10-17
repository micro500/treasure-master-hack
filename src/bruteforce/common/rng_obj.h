#ifndef RNG_OBJ_H
#define RNG_OBJ_H

#include "data_sizes.h"

class RNG
{
public:
	RNG();

	void generate_rng_table();
	uint8 run_rng(uint16 * rng_seed);

	void generate_seed_forward_1();
	void generate_seed_forward_128();

	void _split(uint8** rng_values, bool hi);

	void _generate_regular_rng_values(uint8** rng_values, bool shuffle, int bits, bool packing_16);
	void _generate_regular_rng_values_8_split(uint8** rng_values, bool shuffle, int bits, bool hi);

	void generate_regular_rng_values_8();
	void generate_regular_rng_values_128_8_shuffled();
	void generate_regular_rng_values_256_8_shuffled();
	void generate_regular_rng_values_512_8_shuffled();
	void generate_regular_rng_values_8_hi();
	void generate_regular_rng_values_256_8_shuffled_hi();
	void generate_regular_rng_values_8_lo();
	void generate_regular_rng_values_256_8_shuffled_lo();
	void generate_regular_rng_values_16();

	void _generate_expansion_values(uint8** rng_values, bool shuffle, int bits, bool packing_16);
	void generate_expansion_values_8();
	void generate_expansion_values_128_8_shuffled();
	void generate_expansion_values_256_8_shuffled();

	void _generate_alg0_values(uint8** rng_values, bool shuffle, int bits, bool packing_16);
	void generate_alg0_values_8();
	void generate_alg0_values_128_8_shuffled();
	void generate_alg0_values_256_8_shuffled();
	void generate_alg0_values_512_8_shuffled();
	void generate_alg0_values_16();

	void _generate_alg2_values(uint8** rng_values, int bits, bool packing_16);
	void generate_alg2_values_8_8();
	void generate_alg2_values_32_8();
	void generate_alg2_values_32_16();
	void generate_alg2_values_64_8();
	void generate_alg2_values_64_16();
	void generate_alg2_values_128_8();
	void generate_alg2_values_128_16();
	void generate_alg2_values_256_8();
	void generate_alg2_values_256_16();
	void generate_alg2_values_512_8();

	void _generate_alg4_values(uint8** rng_values, bool shuffle, int bits, bool packing_16);
	void _generate_alg4_values_8_split(uint8** rng_values, bool shuffle, int bits, bool hi);
	void generate_alg4_values_8();
	void generate_alg4_values_8_hi();
	void generate_alg4_values_128_8_shuffled();
	void generate_alg4_values_256_8_shuffled();
	void generate_alg4_values_512_8_shuffled();
	void generate_alg4_values_256_8_shuffled_hi();
	void generate_alg4_values_8_lo();
	void generate_alg4_values_256_8_shuffled_lo();
	void generate_alg4_values_16();

	void _generate_alg5_values(uint8** rng_values, int bits, bool packing_16);
	void generate_alg5_values_8_8();
	void generate_alg5_values_32_8();
	void generate_alg5_values_32_16();
	void generate_alg5_values_64_8();
	void generate_alg5_values_64_16();
	void generate_alg5_values_128_8();
	void generate_alg5_values_128_16();
	void generate_alg5_values_256_8();
	void generate_alg5_values_256_16();
	void generate_alg5_values_512_8();

	void _generate_alg6_values(uint8** rng_values, bool shuffle, int bits, bool packing_16);
	void generate_alg6_values_8();
	void generate_alg6_values_128_8_shuffled();
	void generate_alg6_values_256_8_shuffled();
	void generate_alg6_values_512_8_shuffled();
	void generate_alg6_values_16();


	static uint16* rng_table;
	static uint16* seed_forward_1;
	static uint16* seed_forward_128;

	static uint8* regular_rng_values_8;
	static uint8* regular_rng_values_128_8_shuffled;
	static uint8* regular_rng_values_256_8_shuffled;
	static uint8* regular_rng_values_512_8_shuffled;
	static uint8* regular_rng_values_8_hi;
	static uint8* regular_rng_values_256_8_shuffled_hi;
	static uint8* regular_rng_values_8_lo;
	static uint8* regular_rng_values_256_8_shuffled_lo;
	static uint16* regular_rng_values_16;

	static uint8* expansion_values_8;
	static uint8* expansion_values_128_8_shuffled;
	static uint8* expansion_values_256_8_shuffled;

	static uint8* alg0_values_8;
	static uint8* alg0_values_128_8_shuffled;
	static uint8* alg0_values_256_8_shuffled;
	static uint8* alg0_values_512_8_shuffled;
	static uint16* alg0_values_16;

	static uint8* alg2_values_8_8;
	static uint32* alg2_values_32_8;
	static uint32* alg2_values_32_16;
	static uint64* alg2_values_64_8;
	static uint64* alg2_values_64_16;
	static uint8* alg2_values_128_8;
	static uint8* alg2_values_128_16;
	static uint8* alg2_values_256_8;
	static uint8* alg2_values_256_16;
	static uint8* alg2_values_512_8;

	static uint8* alg4_values_8;
	static uint8* alg4_values_8_hi;
	static uint8* alg4_values_128_8_shuffled;
	static uint8* alg4_values_256_8_shuffled;
	static uint8* alg4_values_512_8_shuffled;
	static uint8* alg4_values_256_8_shuffled_hi;
	static uint8* alg4_values_8_lo;
	static uint8* alg4_values_256_8_shuffled_lo;
	static uint16* alg4_values_16;

	static uint8* alg5_values_8_8;
	static uint32* alg5_values_32_8;
	static uint32* alg5_values_32_16;
	static uint64* alg5_values_64_8;
	static uint64* alg5_values_64_16;
	static uint8* alg5_values_128_8;
	static uint8* alg5_values_128_16;
	static uint8* alg5_values_256_8;
	static uint8* alg5_values_256_16;
	static uint8* alg5_values_512_8;

	static uint8* alg6_values_8;
	static uint8* alg6_values_128_8_shuffled;
	static uint8* alg6_values_256_8_shuffled;
	static uint8* alg6_values_512_8_shuffled;
	static uint16* alg6_values_16;
};

#endif // RNG_OBJ_H