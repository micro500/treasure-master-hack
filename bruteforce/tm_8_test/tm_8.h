#ifndef TM_8_H
#define TM_8_H

#include "data_sizes.h"

void working_code_alg_0 (uint8 * working_code, uint8 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void working_code_alg_1(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void working_code_alg_2(uint8 * working_code, uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void working_code_alg_3(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void working_code_alg_5(uint8 * working_code, uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void working_code_alg_6(uint8 * working_code, uint8 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void working_code_alg_7 (uint8 * working_code);

#endif //TM_8_H