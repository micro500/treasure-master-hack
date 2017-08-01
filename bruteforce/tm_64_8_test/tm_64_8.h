#ifndef TM_64_8_H
#define TM_64_8_H

#include "data_sizes.h"

void alg0(uint64 * working_code, const uint64 * alg0_values, const uint16 rng_seed);

void alg1(uint64 * working_code, const uint64 * regular_rng_values_lo, const uint64 * regular_rng_values_hi, const uint16 rng_seed);

void alg2(uint64 * working_code, const uint64 * alg2_values, const uint16 rng_seed);

void alg3(uint64 * working_code, const uint64 * regular_rng_values, const uint16 rng_seed);

void alg5(uint64 * working_code, const uint64 * alg5_values, const uint16 rng_seed);

void alg6(uint64 * working_code, const uint64 * alg6_values, const uint16 rng_seed);

void alg7(uint64 * working_code);

#endif //TM_64_8_H