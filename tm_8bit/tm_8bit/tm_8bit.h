#ifndef TM_8BIT_H
#define TM_8BIT_H

#include "data_sizes.h"

void generate_rng_table(uint16 * rng_table);
uint8 run_rng(uint16 * rng_seed, uint16 * rng_table);

void working_code_alg_0 (uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);

void working_code_alg_1(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);

void working_code_alg_2(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);


void working_code_alg_3(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);
void working_code_alg_4(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);

void working_code_alg_5(uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);


void working_code_alg_6 (uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);

void working_code_alg_7 (uint8 * working_code, uint16 * rng_seed, uint16 * rng_table);

#endif //TM_8BIT_H