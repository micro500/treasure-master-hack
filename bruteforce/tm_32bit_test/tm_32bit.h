#ifndef TM_32BIT_H
#define TM_32BIT_H

#include "data_sizes.h"

void generate_rng_table(uint16 * rng_table);

uint8 run_rng(uint16 * rng_seed, uint16 * rng_table);


void generate_regular_rng_values(uint16 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg0_values(uint16 * alg0_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg6_values(uint16 * alg6_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg2_values(uint32 * alg2_values, uint16 * rng_seed, uint16 * rng_table);
void generate_alg5_values(uint32 * alg5_values, uint16 * rng_seed, uint16 * rng_table);

void generate_seed_forward_1(uint16 * values, uint16 * rng_table);

void generate_seed_forward_128(uint16 * values, uint16 * rng_seed, uint16 * rng_table);

void alg0(uint32 * working_code, uint32 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg1(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg2(uint32 * working_code, uint32 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg3(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);
void alg4(uint32 * working_code, uint32 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg5(uint32 * working_code, uint32 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg6(uint32 * working_code, uint32 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg7(uint32 * working_code);

#endif //TM_32BIT_H