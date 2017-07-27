#ifndef TM_AVX2_INTRINSICS_H
#define TM_AVX2_INTRINSICS_H

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

#include "../tm_8bit/data_sizes.h"

void generate_rng_table(uint16 * rng_table);

uint8 run_rng(uint16 * rng_seed, uint16 * rng_table);


void generate_regular_rng_values(uint16 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg0_values(uint16 * alg0_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg6_values(uint16 * alg6_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg2_values(uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg5_values(uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table);
void generate_seed_forward_1(uint16 * values, uint16 * rng_table);

void generate_seed_forward_128(uint16 * values, uint16 * rng_seed, uint16 * rng_table);

void run_alg(int alg_id, int iterations, uint8 * working_code, uint8 * regular_rng_values, uint8* alg0_values, uint8* alg2_values, uint8* alg5_values, uint8* alg6_values, uint16 * rng_seed, uint16 * rng_forward_1, uint16 * rng_forward_128);

void * aligned_malloc(int byte_count, int align_size);
#endif //TM_AVX2_INTRINSICS_H