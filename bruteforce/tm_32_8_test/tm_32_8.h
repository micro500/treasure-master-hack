#ifndef TM_32_8_H
#define TM_32_8_H

#include "data_sizes.h"

#define TM_32_8_ALG0(rng_seed) for (int i = 0; i < 32; i++) \
{ \
	((uint32*)working_code)[i] = ((((uint32*)working_code)[i] << 1) & 0xFEFEFEFE) | ((uint32*)alg0_values)[(rng_seed * 128) / 4 + i]; \
} \
rng_seed = rng_seed_forward_128[rng_seed];



#define TM_32_8_ALG1(rng_values_lo, rng_values_hi, rng_seed) for (int i = 0; i < 32; i++) \
{ \
    ((uint32*)working_code)[i] = (((((uint32*)working_code)[i] & 0x00FF00FF) + ((uint32*)rng_values_lo)[(rng_seed * 128) / 4 + i]) & 0x00FF00FF) | (((((uint32*)working_code)[i] & 0xFF00FF00) + ((uint32*)rng_values_hi)[(rng_seed * 128) / 4 + i]) & 0xFF00FF00); \
} \
rng_seed = rng_seed_forward_128[rng_seed];



#define TM_32_8_ALG2(rng_seed) uint32 carry = alg2_values[rng_seed]; \
for (int i = 31; i >= 0; i--) \
{ \
    uint32 next_carry = (((uint32*)working_code)[i] & 0x00000001) << 24; \
    ((uint32*)working_code)[i] = ((((uint32*)working_code)[i] & 0x00010000) >> 8) | ((((uint32*)working_code)[i] >> 1) & 0x007F007F) | ((((uint32*)working_code)[i] << 1) & 0xFE00FE00) | ((((uint32*)working_code)[i] >> 8) & 0x00800080) | carry; \
    carry = next_carry; \
} \
rng_seed = rng_seed_forward_1[rng_seed];



#define TM_32_8_ALG3(rng_seed) for (int i = 0; i < 32; i++) \
{ \
    ((uint32*)working_code)[i] = ((uint32*)working_code)[i] ^ ((uint32*)regular_rng_values)[(rng_seed * 128) / 4 + i]; \
} \
rng_seed = rng_seed_forward_128[rng_seed];



#define TM_32_8_ALG5(rng_seed)  uint32 carry = alg5_values[rng_seed]; \
for (int i = 31; i >= 0; i--) \
{ \
    uint32 next_carry = (((uint32*)working_code)[i] & 0x00000080) << 24; \
    ((uint32*)working_code)[i] = ((((uint32*)working_code)[i] & 0x00800000) >> 8) | ((((uint32*)working_code)[i] << 1) & 0x00FE00FE) | ((((uint32*)working_code)[i] >> 1) & 0x7F007F00) | ((((uint32*)working_code)[i] >> 8) & 0x00010001) | carry; \
    carry = next_carry; \
} \
rng_seed = rng_seed_forward_1[rng_seed];



#define TM_32_8_ALG6(rng_seed) for (int i = 0; i < 32; i++) \
{ \
    ((uint32*)working_code)[i] = ((((uint32*)working_code)[i] >> 1) &0x7F7F7F7F) | ((uint32*)alg6_values)[(rng_seed * 128) / 4 + i]; \
} \
rng_seed = rng_seed_forward_128[rng_seed];



#define TM_32_8_ALG7 for (int i = 0; i < 32; i++) \
{ \
    ((uint32*)working_code)[i] = ((uint32*)working_code)[i] ^ 0xFFFFFFFF; \
}

#endif //TM_32_8_H