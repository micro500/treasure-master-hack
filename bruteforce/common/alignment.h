#ifndef ALIGNMENT_H
#define ALIGNMENT_H
#include "data_sizes.h"

#if defined(_MSC_VER)
#define ALIGNED(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED(x) __attribute__ ((aligned(x)))
#endif
#endif

//void * aligned_malloc(int byte_count, int align_size);
void * aligned_malloc(int byte_count, int align_size)
{
	uint8 * new_array = new uint8[byte_count + align_size];
	int mod = ((uint64)new_array) % align_size;
	int remainder = align_size - mod;
	uint64 result = (uint64)new_array + (uint64)remainder;
	return (void*)(result);
}

#endif // ALIGNMENT_H