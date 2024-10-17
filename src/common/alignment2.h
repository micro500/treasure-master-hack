#ifndef ALIGNMENT2_H
#define ALIGNMENT2_H
#include "data_sizes.h"

#if defined(_MSC_VER)
#define ALIGNED(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED(x) __attribute__ ((aligned(x)))
#endif
#endif

void * aligned_malloc(int byte_count, int align_size);

int shuffle_8(int offset, int bits);
uint8* packing_alloc(int size, bool packing_16);
void packing_store(uint8* dest, int offset, uint8 value, bool packing_16);
uint8 packing_load(uint8* src, int offset, bool packing_16);

#endif // ALIGNMENT2_H