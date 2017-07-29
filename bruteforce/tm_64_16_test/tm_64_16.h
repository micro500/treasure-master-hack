#ifndef TM_64_16_H
#define TM_64_16_H

#include "data_sizes.h"

void alg0(uint64 * working_code, uint64 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg1(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg2(uint64 * working_code, uint64 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg3(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);
void alg4(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg5(uint64 * working_code, uint64 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg6(uint64 * working_code, uint64 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg7(uint64 * working_code);

#endif //TM_64_16_H