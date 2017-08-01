#ifndef TM_32_8_H
#define TM_32_8_H

#include "data_sizes.h"

void alg0(uint32 * working_code, const uint32 * alg0_values, const uint16 rng_seed);

void alg1(uint32 * working_code, const uint32 * regular_rng_values_lo, const uint32 * regular_rng_values_hi, const uint16 rng_seed);

void alg2(uint32 * working_code, const uint32 * alg2_values, const uint16 rng_seed);

void alg3(uint32 * working_code, const uint32 * regular_rng_values, const uint16 rng_seed);

void alg5(uint32 * working_code, const uint32 * alg5_values, const uint16 rng_seed);

void alg6(uint32 * working_code, const uint32 * alg6_values, const uint16 rng_seed);

void alg7(uint32 * working_code);

#endif //TM_32_8_H