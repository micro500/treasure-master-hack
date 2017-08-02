#ifndef TM_128_8_H
#define TM_128_8_H

#include "data_sizes.h"

void alg0(uint8 * working_code, const uint8 * alg0_values, const uint16 rng_seed);

void alg1(uint8 * working_code, const uint8 * regular_rng_values_lo, const uint8 * regular_rng_values_hi, const uint16 rng_seed);

void alg2(uint8 * working_code, const uint8 * alg2_values, const uint16 rng_seed);

void alg3(uint8 * working_code, const uint8 * regular_rng_values, const uint16 rng_seed);

void alg5(uint8 * working_code, const uint8 * alg5_values, const uint16 rng_seed);

void alg6(uint8 * working_code, const uint8 * alg6_values, const uint16 rng_seed);

void alg7(uint8 * working_code);

#endif //TM_128_8_H