#ifndef RNG_OBJ_H
#define RNG_OBJ_H

#include <stdint.h>

class RNG
{
public:
	RNG();

	void generate_rng_table();
	uint8_t run_rng(uint16_t* rng_seed);

	static void cleanup();

	void generate_seed_forward();

	void generate_seed_forward_1();
	void generate_seed_forward_128();

	void _split(uint8_t** rng_values, bool hi);

	void _generate_regular_rng_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16);
	void _generate_regular_rng_values(uint8_t** rng_values, bool shuffle, int bits, bool packing_16);
	void _generate_regular_rng_values_8_split(uint8_t** rng_values, bool shuffle, int bits, bool hi);
	void generate_regular_rng_values_8();
	void generate_regular_rng_values_128_8_shuffled();
	void generate_regular_rng_values_for_seeds(uint8_t** out, uint16_t* seeds, int seed_count, bool packing_16);
	void generate_regular_rng_values_for_seeds_8(uint8_t** out, uint16_t* seeds, int seed_count);
	void generate_regular_rng_values_256_8_shuffled();
	void generate_regular_rng_values_512_8_shuffled();
	void generate_regular_rng_values_8_hi();
	void generate_regular_rng_values_256_8_shuffled_hi();
	void generate_regular_rng_values_8_lo();
	void generate_regular_rng_values_256_8_shuffled_lo();
	void generate_regular_rng_values_16();

	void _generate_expansion_values(uint8_t** rng_values, bool shuffle, int bits, bool packing_16);
	void generate_expansion_values_8();
	void generate_expansion_values_128_8_shuffled();
	void generate_expansion_values_256_8_shuffled();
	void generate_expansion_values_512_8_shuffled();
	void _generate_expansion_values_for_seed(uint8_t* rng_values, uint16_t rng_seed, bool shuffle, int bits, bool packing_16);
	void _generate_expansion_values_for_seed_8(uint8_t** rng_values, uint16_t rng_seed, bool shuffle, int bits);

	void _generate_alg0_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16);
	void generate_alg0_values_for_seeds(uint8_t** out, uint16_t* seeds, int seed_count, bool packing_16);
	void generate_alg0_values_for_seeds_8(uint8_t** out, uint16_t* seeds, int seed_count);
	void _generate_alg0_values(uint8_t** rng_values, bool shuffle, int bits, bool packing_16);
	void generate_alg0_values_8();
	void generate_alg0_values_128_8_shuffled();
	void generate_alg0_values_256_8_shuffled();
	void generate_alg0_values_512_8_shuffled();
	void generate_alg0_values_16();

	void _generate_alg2_values_for_seed(uint8_t* out, uint16_t seed, int entries, int bits, bool packing_16);
	void generate_alg2_values_for_seeds(uint8_t** out, uint16_t* seeds, int seed_count, int bits, bool packing_16);
	void generate_alg2_values_for_seeds_128_8(uint8_t** out, uint16_t* seeds, int seed_count);
	void _generate_alg2_values(uint8_t** rng_values, int bits, bool packing_16);
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

	void _generate_alg4_values(uint8_t** rng_values, bool shuffle, int bits, bool packing_16);
	void _generate_alg4_values_8_split(uint8_t** rng_values, bool shuffle, int bits, bool hi);
	void generate_alg4_values_8();
	void generate_alg4_values_8_hi();
	void generate_alg4_values_128_8_shuffled();
	void generate_alg4_values_256_8_shuffled();
	void generate_alg4_values_512_8_shuffled();
	void generate_alg4_values_256_8_shuffled_hi();
	void generate_alg4_values_8_lo();
	void generate_alg4_values_256_8_shuffled_lo();
	void generate_alg4_values_16();

	void _generate_alg5_values_for_seed(uint8_t* out, uint16_t seed, int entries, int bits, bool packing_16);
	void generate_alg5_values_for_seeds(uint8_t** out, uint16_t* seeds, int seed_count, int bits, bool packing_16);
	void generate_alg5_values_for_seeds_128_8(uint8_t** out, uint16_t* seeds, int seed_count);
	void _generate_alg5_values(uint8_t** rng_values, int bits, bool packing_16);
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

	void _generate_alg6_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16);
	void generate_alg6_values_for_seeds(uint8_t** out, uint16_t* seeds, int seed_count, bool packing_16);
	void generate_alg6_values_for_seeds_8(uint8_t** out, uint16_t* seeds, int seed_count);
	void _generate_alg6_values(uint8_t** rng_values, bool shuffle, int bits, bool packing_16);
	void generate_alg6_values_8();
	void generate_alg6_values_128_8_shuffled();
	void generate_alg6_values_256_8_shuffled();
	void generate_alg6_values_512_8_shuffled();
	void generate_alg6_values_16();

	void _generate_alg06_values(uint8_t** rng_values, bool shuffle, int bits, bool packing_16);
	void generate_alg06_values_8();
	void generate_alg06_values_128_8_shuffled();


	void generate_rng_seq_tables();

	static uint16_t* rng_table;
	static uint16_t* rng_seq_table;
	static uint32_t* rng_pos_table;
	static uint32_t  rng_seq_table_size;
	static uint16_t* seed_forward;
	static uint16_t* seed_forward_1;
	static uint16_t* seed_forward_128;

	static uint8_t* regular_rng_values_8;
	static uint8_t* regular_rng_values_128_8_shuffled;
	uint8_t* regular_rng_values_for_seeds_8;
	uint16_t* regular_rng_values_for_seeds_16;
	static uint8_t* regular_rng_values_256_8_shuffled;
	static uint8_t* regular_rng_values_512_8_shuffled;
	static uint8_t* regular_rng_values_8_hi;
	static uint8_t* regular_rng_values_256_8_shuffled_hi;
	static uint8_t* regular_rng_values_8_lo;
	static uint8_t* regular_rng_values_256_8_shuffled_lo;
	static uint16_t* regular_rng_values_16;

	static uint8_t* expansion_values_8;
	static uint8_t* expansion_values_128_8_shuffled;
	static uint8_t* expansion_values_256_8_shuffled;
	static uint8_t* expansion_values_512_8_shuffled;

	static uint8_t* alg0_values_8;
	static uint8_t* alg0_values_128_8_shuffled;
	static uint8_t* alg0_values_256_8_shuffled;
	static uint8_t* alg0_values_512_8_shuffled;
	static uint16_t* alg0_values_16;
	uint8_t* alg0_values_for_seeds_8;
	uint16_t* alg0_values_for_seeds_16;

	static uint8_t* alg2_values_8_8;
	static uint32_t* alg2_values_32_8;
	static uint32_t* alg2_values_32_16;
	static uint64_t* alg2_values_64_8;
	static uint64_t* alg2_values_64_16;
	static uint8_t* alg2_values_128_8;
	static uint8_t* alg2_values_128_16;
	static uint8_t* alg2_values_256_8;
	static uint8_t* alg2_values_256_16;
	static uint8_t* alg2_values_512_8;
	uint8_t* alg2_values_for_seeds_8_8;
	uint32_t* alg2_values_for_seeds_32_8;
	uint32_t* alg2_values_for_seeds_32_16;
	uint64_t* alg2_values_for_seeds_64_8;
	uint64_t* alg2_values_for_seeds_64_16;
	uint8_t* alg2_values_for_seeds_128_8;
	uint8_t* alg2_values_for_seeds_128_16;
	uint8_t* alg2_values_for_seeds_256_8;
	uint8_t* alg2_values_for_seeds_256_16;
	uint8_t* alg2_values_for_seeds_512_8;

	static uint8_t* alg4_values_8;
	static uint8_t* alg4_values_8_hi;
	static uint8_t* alg4_values_128_8_shuffled;
	static uint8_t* alg4_values_256_8_shuffled;
	static uint8_t* alg4_values_512_8_shuffled;
	static uint8_t* alg4_values_256_8_shuffled_hi;
	static uint8_t* alg4_values_8_lo;
	static uint8_t* alg4_values_256_8_shuffled_lo;
	static uint16_t* alg4_values_16;

	static uint8_t* alg5_values_8_8;
	static uint32_t* alg5_values_32_8;
	static uint32_t* alg5_values_32_16;
	static uint64_t* alg5_values_64_8;
	static uint64_t* alg5_values_64_16;
	static uint8_t* alg5_values_128_8;
	static uint8_t* alg5_values_128_16;
	static uint8_t* alg5_values_256_8;
	static uint8_t* alg5_values_256_16;
	static uint8_t* alg5_values_512_8;
	uint8_t* alg5_values_for_seeds_8_8;
	uint32_t* alg5_values_for_seeds_32_8;
	uint32_t* alg5_values_for_seeds_32_16;
	uint64_t* alg5_values_for_seeds_64_8;
	uint64_t* alg5_values_for_seeds_64_16;
	uint8_t* alg5_values_for_seeds_128_8;
	uint8_t* alg5_values_for_seeds_128_16;
	uint8_t* alg5_values_for_seeds_256_8;
	uint8_t* alg5_values_for_seeds_256_16;
	uint8_t* alg5_values_for_seeds_512_8;

	static uint8_t* alg6_values_8;
	static uint8_t* alg6_values_128_8_shuffled;
	static uint8_t* alg6_values_256_8_shuffled;
	static uint8_t* alg6_values_512_8_shuffled;
	static uint16_t* alg6_values_16;
	uint8_t* alg6_values_for_seeds_8;
	uint16_t* alg6_values_for_seeds_16;

	static uint8_t* alg06_values_8;
	static uint8_t* alg06_values_128_8_shuffled;
};

#endif // RNG_OBJ_H