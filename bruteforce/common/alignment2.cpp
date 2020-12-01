#include "alignment2.h"

void * aligned_malloc(int byte_count, int align_size)
{
	uint8 * new_array = new uint8[byte_count + align_size];
	int mod = ((uint64)new_array) % align_size;
	int remainder = align_size - mod;
	uint64 result = (uint64)new_array + (uint64)remainder;
	return (void*)(result);
}
