#ifndef ALIGNMENT2_H
#define ALIGNMENT2_H

#include <stdint.h>

#if defined(_MSC_VER)
#define ALIGNED(x) __declspec(align(x))
#define MEMBER_ALIGNAS(x) alignas(x)
#else
#if defined(__GNUC__)
#define ALIGNED(x) __attribute__ ((aligned(x)))
#define MEMBER_ALIGNAS(x)
#endif
#endif

void * aligned_malloc(int byte_count, int align_size);
void aligned_free(void* ptr);

#ifdef _MSC_VER
__forceinline int shuffle_8(int offset, int bits)
#else
__attribute__((always_inline)) inline int shuffle_8(int offset, int bits)
#endif
{
	return (offset / (bits / 4)) * (bits / 4) + (offset % 2) * (bits / 8) + ((offset / 2) % (bits / 8));
}
uint8_t* packing_alloc(int size, bool packing_16, int align_size = 64);
void packing_store(uint8_t* dest, int offset, uint8_t value, bool packing_16);
uint8_t packing_load(uint8_t* src, int offset, bool packing_16);

#endif // ALIGNMENT2_H