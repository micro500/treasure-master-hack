#include "alignment2.h"
#include <stdint.h>

void * aligned_malloc(int byte_count, int align_size)
{
	uint8_t * raw = new uint8_t[byte_count + align_size + sizeof(void*)];
	uintptr_t ptr = (uintptr_t)raw + sizeof(void*);
	uintptr_t aligned = (ptr + align_size - 1) & ~((uintptr_t)(align_size - 1));
	((void**)aligned)[-1] = raw;
	return (void*)aligned;
}

void aligned_free(void* ptr)
{
	if (ptr)
		delete[] (uint8_t*)(((void**)ptr)[-1]);
}

uint8_t* packing_alloc(int size, bool packing_16, int align_size)
{
	return (uint8_t*)aligned_malloc(size * (packing_16 ? 2 : 1), align_size);
}

void packing_store(uint8_t* dest, int offset, uint8_t value, bool packing_16)
{
	if (packing_16)
	{
		((uint16_t*)dest)[offset] = value;
	}
	else
	{
		((uint8_t*)dest)[offset] = value;
	}
}

uint8_t packing_load(uint8_t* src, int offset, bool packing_16)
{
	if (packing_16)
	{
		return static_cast<uint8_t>((reinterpret_cast<uint16_t*>(src))[offset] & 0xFF);
	}
	else
	{
		return src[offset];
	}
}

