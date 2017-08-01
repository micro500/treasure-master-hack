#ifndef TM_8_H
#define TM_8_H

#include "data_sizes.h"

void working_code_alg_0 (uint8 * working_code, const uint8 * alg0_values, const uint16 rng_seed);

void working_code_alg_1(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed);

void working_code_alg_2(uint8 * working_code, const uint8 * alg2_values, const uint16 rng_seed);

void working_code_alg_3(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed);

void working_code_alg_5(uint8 * working_code, const uint8 * alg5_values, const uint16 rng_seed);

void working_code_alg_6(uint8 * working_code, const uint8 * alg6_values, const uint16 rng_seed);

void working_code_alg_7 (uint8 * working_code);

#endif //TM_8_H