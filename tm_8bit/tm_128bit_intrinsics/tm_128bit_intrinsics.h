#ifndef TM_128BIT_INTRINSICS_H
#define TM_128BIT_INTRINSICS_H

#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A

#include "..\tm_8bit\data_sizes.h"

void generate_rng_table(uint16 * rng_table);

uint8 run_rng(uint16 * rng_seed, uint16 * rng_table);


void generate_regular_rng_values(uint16 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg0_values(uint16 * alg0_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg6_values(uint16 * alg6_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg2_values(uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table);

void generate_alg5_values(uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table);
void generate_seed_forward_1(uint16 * values, uint16 * rng_table);

void generate_seed_forward_128(uint16 * values, uint16 * rng_seed, uint16 * rng_table);

void alg0(uint8 * working_code, uint8 * alg0_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg1(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg2(uint8 * working_code, uint8 * alg2_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg3(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg4(uint8 * working_code, uint8 * regular_rng_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);

void alg5(uint8* working_code, uint8 * alg5_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);


void alg6(uint8 * working_code, uint8 * alg6_values, uint16 * rng_seed, uint16 * rng_table, uint16 * rng_forward);


void alg7(uint8 * working_code);
void * aligned_malloc(int byte_count, int align_size);
#endif //TM_128BIT_INTRINSICS_H