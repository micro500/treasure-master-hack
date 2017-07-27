#ifndef TM_64BIT_H
#define TM_64BIT_H

#include "../tm_8bit/data_sizes.h"

void generate_rng_table(uint16 * rng_table);

uint8 run_rng(uint16 * rng_seed, uint16 * rng_table);


void generate_regular_rng_values(uint16 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg0_values(uint16 * alg0_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg6_values(uint16 * alg6_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg2_values(uint64 * alg2_values, uint16 * rng_seed, uint16 * rng_table);
void generate_alg5_values(uint64 * alg5_values, uint16 * rng_seed, uint16 * rng_table);

void generate_seed_forward_1(uint16 * values, uint16 * rng_table);

void generate_seed_forward_128(uint16 * values, uint16 * rng_seed, uint16 * rng_table);

void alg0(uint64 * working_code, uint64 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg1(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg2(uint64 * working_code, uint64 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg3(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);
void alg4(uint64 * working_code, uint64 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg5(uint64 * working_code, uint64 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg6(uint64 * working_code, uint64 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg7(uint64 * working_code);

#endif //TM_64BIT_H