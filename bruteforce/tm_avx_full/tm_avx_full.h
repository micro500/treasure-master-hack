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

void run_alg(int alg_id, int iterations, uint8 * working_code, uint8 * regular_rng_values, uint8* alg0_values, uint8* alg2_values, uint8* alg5_values, uint8* alg6_values, uint16 * rng_seed, uint16 * rng_forward_1, uint16 * rng_forward_128);

#endif //TM_AVX_INTRINSICS_H