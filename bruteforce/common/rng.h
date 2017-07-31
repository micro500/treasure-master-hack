#ifndef RNG_H
#define RNG_H

#include "data_sizes.h"

void generate_rng_table(uint16 * rng_table);
uint8 run_rng(uint16 * rng_seed, uint16 * rng_table);

void generate_regular_rng_values_8(uint8 * regular_rng_values, uint16 * rng_table);
void generate_regular_rng_values_16(uint16 * regular_rng_values, uint16 * rng_table);

void generate_regular_rng_values_8_lo(uint8 * regular_rng_values_lo, uint16 * rng_table);
void generate_regular_rng_values_8_hi(uint8 * regular_rng_values_hi, uint16 * rng_table);

void generate_alg0_values_8(uint8 * alg0_values, uint16 * rng_table);
void generate_alg0_values_16(uint16 * alg0_values, uint16 * rng_table);

void generate_alg6_values_8(uint8 * alg6_values, uint16 * rng_table);
void generate_alg6_values_16(uint16 * alg6_values, uint16 * rng_table);

void generate_alg4_values_8_lo(uint8 * alg4_values_lo, uint16 * rng_table);
void generate_alg4_values_8_hi(uint8 * alg4_values_hi, uint16 * rng_table);

void generate_alg4_values_16(uint16 * alg4_values, uint16 * rng_table);

void generate_alg2_values_32_8(uint32 * alg2_values, uint16 * rng_table);
void generate_alg2_values_32_16(uint32 * alg2_values, uint16 * rng_table);
void generate_alg2_values_64_8(uint64 * alg2_values, uint16 * rng_table);
void generate_alg2_values_64_16(uint64 * alg2_values, uint16 * rng_table);
void generate_alg2_values_128_8(uint8 * alg2_values, uint16 * rng_table);
void generate_alg2_values_128_16(uint8 * alg2_values, uint16 * rng_table);
void generate_alg2_values_256_16(uint8 * alg2_values, uint16 * rng_table);

void generate_alg5_values_32_8(uint32 * alg5_values, uint16 * rng_table);
void generate_alg5_values_32_16(uint32 * alg5_values, uint16 * rng_table);
void generate_alg5_values_64_8(uint64 * alg5_values, uint16 * rng_table);
void generate_alg5_values_64_16(uint64 * alg5_values, uint16 * rng_table);
void generate_alg5_values_128_8(uint8 * alg5_values, uint16 * rng_table);
void generate_alg5_values_128_16(uint8 * alg5_values, uint16 * rng_table);
void generate_alg5_values_256_16(uint8 * alg5_values, uint16 * rng_table);

void generate_seed_forward_1(uint16 * values, uint16 * rng_table);
void generate_seed_forward_128(uint16 * values, uint16 * rng_table);

#endif // RNG_H