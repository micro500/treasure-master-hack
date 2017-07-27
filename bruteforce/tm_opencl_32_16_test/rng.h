#ifndef RNG_H
#define RNG_H

#include "data_sizes.h"

void generate_rng_table(uint16 * rng_table);
uint8 run_rng(uint16 * rng_seed, uint16 * rng_table);

void generate_regular_rng_values_16(uint16 * regular_rng_values, uint16 * rng_table);

void generate_alg0_values_16(uint16 * alg0_values, uint16 * rng_table);
void generate_alg6_values_16(uint16 * alg6_values, uint16 * rng_table);

void generate_alg2_values_32_16(uint32 * alg2_values, uint16 * rng_table);
void generate_alg5_values_32_16(uint32 * alg5_values, uint16 * rng_table);

void generate_seed_forward_1(uint16 * values, uint16 * rng_table);
void generate_seed_forward_128(uint16 * values, uint16 * rng_table);

#endif // RNG_H