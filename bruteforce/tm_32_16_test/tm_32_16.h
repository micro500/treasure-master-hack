#ifndef TM_32_16_H
#define TM_32_16_H

#include "data_sizes.h"

void alg0(uint32 * working_code, uint32 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg1(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg2(uint32 * working_code, uint32 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg3(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);
void alg4(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg5(uint32 * working_code, uint32 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg6(uint32 * working_code, uint32 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg7(uint32 * working_code);

#endif //TM_32_16_H