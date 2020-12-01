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
#endif // ALIGNMENT2_H