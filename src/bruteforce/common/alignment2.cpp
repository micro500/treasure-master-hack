#include "alignment2.h"

void * aligned_malloc(int byte_count, int align_size)
{
	uint8 * new_array = new uint8[byte_count + align_size];
	int mod = ((uint64)new_array) % align_size;
	int remainder = align_size - mod;
	uint64 result = (uint64)new_array + (uint64)remainder;
	return (void*)(result);
}

int shuffle_8(int offset, int bits)
{
	return (offset / (bits / 4)) * (bits / 4) + (offset % 2) * (bits / 8) + ((offset / 2) % (bits / 8));
}

uint8* packing_alloc(int size, bool packing_16)
{
	return (uint8*)aligned_malloc(size * (packing_16 ? 2 : 1), 64);
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

