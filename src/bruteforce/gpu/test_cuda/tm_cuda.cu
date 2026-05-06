#ifndef __CUDACC__
#define __CUDACC__
#endif

#include <cuda_runtime.h>
#include "device_launch_parameters.h"

#include <cstdint>

extern "C" __device__ uint32_t get_code_index()
{
	return threadIdx.x;
}

extern "C" __device__ uint32_t get_data_offset()
{
	return blockIdx.x;
}

extern "C" __device__ uint32_t get_rng_seed_from_key(uint32_t key)
{
	return ((key & 0xFF) << 8) | ((key >> 8) & 0xFF);
}

extern "C" __device__ uint32_t get_init_data(uint32_t data_start)
{
	uint32_t data_offset = get_data_offset();
	uint32_t data = data_start + data_offset;

	return data;
}

extern "C" __device__ uint32_t get_init_value(uint32_t key, uint32_t data_start, uint32_t code_index)
{
	if ((code_index % 2) == 0)
	{
		return key;
	}
	else
	{
		return get_init_data(data_start);
	}
}

extern "C" __device__ uint32_t get_expand_value(uint32_t code_index, uint32_t rng_seed, uint8_t* expansion_values)
{
	return ((uint32_t*)expansion_values)[rng_seed * 32 + code_index];
}

extern "C" __device__ uint32_t expand_value(uint32_t cur_val, uint32_t code_index, uint32_t rng_seed, uint8_t* expansion_values)
{
	uint32_t ex_val = get_expand_value(code_index, rng_seed, expansion_values);

	cur_val = __vadd4(cur_val, ex_val);
	return cur_val;
}

extern "C" __device__ uint32_t get_rng_val(uint32_t rng_seed, uint8_t* rng_mem)
{
	uint32_t code_index = get_code_index();

	uint32_t* s = (uint32_t*)rng_mem;
	return s[rng_seed * 32 + code_index];
}

extern "C" __device__ uint32_t alg(uint32_t cur_val, uint32_t algorithm_id, uint16_t* rng_seed, uint32_t* local_storage, uint8_t* rng_seed_forward_1, uint8_t* rng_seed_forward_128, uint8_t* regular_rng_values, uint8_t* alg0_values, uint8_t* alg2_values, uint8_t* alg5_values, uint8_t* alg6_values)
{
	if (algorithm_id == 0)
	{
		uint32_t rng_val = get_rng_val(*rng_seed, alg0_values);
		cur_val <<= 1;
		cur_val &= 0xFEFEFEFE;
		cur_val |= rng_val;
	}
	if (algorithm_id == 1 || algorithm_id == 4 || algorithm_id == 3)
	{
		uint32_t rng_val = get_rng_val(*rng_seed, regular_rng_values);

		if (algorithm_id == 1)
		{
			cur_val = __vadd4(cur_val, rng_val);
		}
		if (algorithm_id == 4)
		{
			cur_val = __vsub4(cur_val, rng_val);
		}
		if (algorithm_id == 3)
		{
			cur_val ^= rng_val;
		}
	}
	if (algorithm_id == 6)
	{
		uint32_t rng_val = get_rng_val(*rng_seed, alg6_values);
		cur_val >>= 1;
		cur_val &= 0x7F7F7F7F;
		cur_val |= rng_val;
	}
	if (algorithm_id == 7)
	{
		cur_val ^= 0xFFFFFFFF;
	}

	if (algorithm_id == 2 || algorithm_id == 5)
	{
		uint32_t code_index = get_code_index();
		((uint32_t*)local_storage)[code_index] = cur_val;
		__syncthreads();

		if (algorithm_id == 2)
		{
			uint32_t temp = (cur_val & 0x00010000) >> 8;
			if (code_index == 31)
			{
				temp |= ((uint32_t*)alg2_values)[*rng_seed];
			}
			else
			{
				temp |= ((((uint32_t*)local_storage)[code_index + 1] & 0x00000001) << 24);
			}
			temp |= (cur_val >> 1) & 0x007F007F;
			temp |= (cur_val << 1) & 0xFE00FE00;
			temp |= (cur_val >> 8) & 0x00800080;

			cur_val = temp;
		}
		if (algorithm_id == 5)
		{
			uint32_t temp = (cur_val & 0x00800000) >> 8;
			if (code_index == 31)
			{
				temp |= ((uint32_t*)alg5_values)[*rng_seed];
			}
			else
			{
				temp |= ((((uint32_t*)local_storage)[code_index + 1] & 0x000000080) << 24);
			}
			temp |= (cur_val >> 1) & 0x7F007F00;
			temp |= (cur_val << 1) & 0x00FE00FE;
			temp |= (cur_val >> 8) & 0x00010001;

			cur_val = temp;
		}
	}

	if (algorithm_id == 0 || algorithm_id == 1 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 6)
	{
		*rng_seed = ((uint16_t*)rng_seed_forward_128)[*rng_seed];
	}
	if (algorithm_id == 2 || algorithm_id == 5)
	{
		*rng_seed = ((uint16_t*)rng_seed_forward_1)[*rng_seed];
	}

	return cur_val;
}

extern "C" __device__ void store_val(uint32_t val, uint8_t* storage_mem)
{
	uint32_t data_offset = get_data_offset();
	uint32_t code_index = get_code_index();

	uint32_t* s = (uint32_t*)storage_mem;
	s[data_offset * 32 + code_index] = val;
}


extern "C" __global__ void expand_test(uint32_t key, uint32_t data_start, uint8_t* expansion_values, uint8_t* result_storage)
{
	uint32_t code_index = get_code_index();
	uint32_t rng_seed = get_rng_seed_from_key(key);

	uint32_t working_val_uint = get_init_value(key, data_start, code_index);
	working_val_uint = expand_value(working_val_uint, code_index, rng_seed, expansion_values);
	store_val(working_val_uint, result_storage);
}
