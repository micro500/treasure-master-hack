#ifndef TM_AVX_INTRINSICS_H
#define TM_AVX_INTRINSICS_H

#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A
#include <immintrin.h> //AVX
//#include <zmmintrin.h> //AVX512

#include "data_sizes.h"

void alg0(uint8 * working_code, uint8 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg1(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg2(uint8 * working_code, uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg3(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg5(uint8* working_code, uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg6(uint8 * working_code, uint8 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg7(uint8 * working_code);

#endif //TM_AVX_INTRINSICS_H