#include "alignment2.h"
#include <stdint.h>

void * aligned_malloc(int byte_count, int align_size)
{
	uint8 * raw = new uint8[byte_count + align_size + sizeof(void*)];
	uintptr_t ptr = (uintptr_t)raw + sizeof(void*);
	uintptr_t aligned = (ptr + align_size - 1) & ~((uintptr_t)(align_size - 1));
	((void**)aligned)[-1] = raw;
	return (void*)aligned;
}

void aligned_free(void* ptr)
{
	if (ptr)
		delete[] (uint8*)(((void**)ptr)[-1]);
}

int shuffle_8(int offset, int bits)
{
	return (offset / (bits / 4)) * (bits / 4) + (offset % 2) * (bits / 8) + ((offset / 2) % (bits / 8));
}

uint8* packing_alloc(int size, bool packing_16, int align_size)
{
	return (uint8*)aligned_malloc(size * (packing_16 ? 2 : 1), align_size);
}

void packing_store(uint8* dest, int offset, uint8 value, bool packing_16)
{
	if (packing_16)
	{
		((uint16*)dest)[offset] = value;
	}
	else
	{
		((uint8*)dest)[offset] = value;
	}
}

uint8 packing_load(uint8* src, int offset, bool packing_16)
{
	if (packing_16)
	{
		return ((uint16*)src)[offset] & 0xFF;
	}
	else
	{
		return ((uint8*)src)[offset];
	}
}

