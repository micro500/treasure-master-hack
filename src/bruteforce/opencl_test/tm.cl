uint get_data_offset()
{
	return get_global_id(1);
}

uint get_code_index()
{
	return get_local_id(0);
}

uint get_expand_value(uint code_index, uint rng_seed, __global unsigned char* expansion_values)
{
	return ((uint*)expansion_values)[rng_seed * 32 + code_index];
}

uint expand_value(uint cur_val, uint code_index, uint rng_seed, __global unsigned char* expansion_values)
{
	uchar4* working_val_uchar4 = (uchar4*)&cur_val;

	uint ex_val = get_expand_value(code_index, rng_seed, expansion_values);
	uchar4* ex_val_uchar4 = (uchar4*)&ex_val;

	*working_val_uchar4 = *working_val_uchar4 + *ex_val_uchar4;

	return cur_val;
}

uint get_init_data(uint data_start)
{
	uint data_offset = get_data_offset();
	uint data = data_start + data_offset;

	return data;
}

uint get_init_value(uint key, uint data_start, uint code_index)
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

uint fetch_val(__global uchar* storage_mem)
{
	uint data_offset = get_data_offset();
	uint code_index = get_code_index();

	uint* s = (uint*)storage_mem;
	return s[data_offset * 32 + code_index];
}

void store_val(uint val, __global uchar* storage_mem)
{
	uint data_offset = get_data_offset();
	uint code_index = get_code_index();

	uint* s = (uint*)storage_mem;
	s[data_offset * 32 + code_index] = val;
}

uint get_rng_val(uint rng_seed, __global uchar* rng_mem)
{
	uint code_index = get_code_index();

	uint* s = (uint*)rng_mem;
	return s[rng_seed * 32 + code_index];
}

uint get_rng_seed_from_key(uint key)
{
	return ((key & 0xFF) << 8) | ((key >> 8) & 0xFF);
}

uint alg(uint cur_val, uint algorithm_id, ushort* rng_seed, __local uint* local_storage, __global uchar* rng_seed_forward_1, __global uchar* rng_seed_forward_128, __global uchar* regular_rng_values, __global uchar* alg0_values, __global uchar* alg2_values, __global uchar* alg5_values, __global uchar* alg6_values)
{
	uchar4* working_val_uchar4 = (uchar4*)&cur_val;

	if (algorithm_id == 0)
	{
		uint rng_val = get_rng_val(*rng_seed, alg0_values);
		cur_val <<= 1;
		cur_val &= 0xFEFEFEFE;
		cur_val |= rng_val;
	} 
	if (algorithm_id == 1 || algorithm_id == 4 || algorithm_id == 3)
	{
		uint rng_val = get_rng_val(*rng_seed, regular_rng_values);
		uchar4* rng_val_uchar4 = (uchar4*)&rng_val;

		if (algorithm_id == 1)
		{
			*working_val_uchar4 += *rng_val_uchar4;
		}
		if (algorithm_id == 4)
		{
			*working_val_uchar4 -= *rng_val_uchar4;
		}
		if (algorithm_id == 3)
		{
			cur_val ^= rng_val;
		}
	}
	if (algorithm_id == 6)
	{
		uint rng_val = get_rng_val(*rng_seed, alg6_values);
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
		uint code_index = get_code_index();
		((uint*)local_storage)[code_index] = cur_val;
		barrier(CLK_LOCAL_MEM_FENCE);

		if (algorithm_id == 2)
		{
			uint temp = (cur_val & 0x00010000) >> 8;
			if (code_index == 31)
			{
				temp |= ((uint*)alg2_values)[*rng_seed];
			}
			else
			{
				temp |= ((((uint*)local_storage)[code_index + 1] & 0x00000001) << 24);
			}
			temp |= (cur_val >> 1) & 0x007F007F;
			temp |= (cur_val << 1) & 0xFE00FE00;
			temp |= (cur_val >> 8) & 0x00800080;

			cur_val = temp;
		}
		if (algorithm_id == 5)
		{
			uint temp = (cur_val & 0x00800000) >> 8;
			if (code_index == 31)
			{
				temp |= ((uint*)alg5_values)[*rng_seed];
			}
			else
			{
				temp |= ((((uint*)local_storage)[code_index + 1] & 0x000000080) << 24);
			}
			temp |= (cur_val >> 1) & 0x7F007F00;
			temp |= (cur_val << 1) & 0x00FE00FE;
			temp |= (cur_val >> 8) & 0x00010001;

			cur_val = temp;
		}
	}

	ushort offset = *rng_seed << 1;
	__global uchar* x = rng_seed_forward_128 + offset;
	__global uchar* y = rng_seed_forward_1 + offset;

	__global uchar* final_ptr;

	if (algorithm_id == 0 || algorithm_id == 1 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 6)
	{
		final_ptr = x;
	}
	if (algorithm_id == 2 || algorithm_id == 5)
	{
		final_ptr = y;
	}
	*rng_seed = *((ushort*)final_ptr);

	//if (algorithm_id == 0 || algorithm_id == 1 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 6)
	//{
	//	*rng_seed = ((ushort*)rng_seed_forward_128)[*rng_seed];
	//}
	//if (algorithm_id == 2 || algorithm_id == 5)
	//{
	//	*rng_seed = ((ushort*)rng_seed_forward_1)[*rng_seed];
	//}

	return cur_val;
}

uint alg2(uint cur_val, uint algorithm_id, ushort* rng_seed, __local uint* local_storage, __global uchar* rng_seed_forward_1, __global uchar* rng_seed_forward_128, __global uchar* regular_rng_values, __global uchar* alg0_values, __global uchar* alg2_values, __global uchar* alg5_values, __global uchar* alg6_values)
{
	uchar4* working_val_uchar4 = (uchar4*)&cur_val;

	uint alg0_rng_val = get_rng_val(*rng_seed, alg0_values);
	uint alg0_val = cur_val << 1;
	alg0_val &= 0xFEFEFEFE;
	alg0_val |= alg0_rng_val;

	uint reg_rng_val = get_rng_val(*rng_seed, regular_rng_values);
	uchar4* reg_rng_val_uchar4 = (uchar4*)&reg_rng_val;

	uint alg1_val = cur_val;
	uchar4* alg1_val_uchar4 = (uchar4*)&alg1_val;
	*alg1_val_uchar4 += *reg_rng_val_uchar4;

	uint alg4_val = cur_val;
	uchar4* alg4_val_uchar4 = (uchar4*)&alg4_val;
	*alg4_val_uchar4 -= *reg_rng_val_uchar4;

	uint alg3_val = cur_val;
	uchar4* alg3_val_uchar4 = (uchar4*)&alg3_val;
	*alg3_val_uchar4 ^= *reg_rng_val_uchar4;

	uint alg6_rng_val = get_rng_val(*rng_seed, alg6_values);
	uint alg6_val = cur_val >> 1;
	alg6_val &= 0x7F7F7F7F;
	alg6_val |= alg6_rng_val;

	uint alg7_val = cur_val ^= 0xFFFFFFFF;

	uint code_index = get_code_index();
	((uint*)local_storage)[code_index] = cur_val;
	//barrier(CLK_LOCAL_MEM_FENCE);

	uint neighbor_val = ((uint*)local_storage)[code_index + 1];

	uint alg2_neighbor_val = ((neighbor_val & 0x00000001) << 24);
	uint alg2_rng_val = ((uint*)alg2_values)[*rng_seed];
	uint alg2_carry;
	if (code_index == 31)
	{
		alg2_carry = alg2_rng_val;
	}
	else
	{
		alg2_carry = alg2_neighbor_val;
	}
	uint alg2_val = (cur_val & 0x00010000) >> 8;
	alg2_val |= alg2_carry;
	alg2_val |= (cur_val >> 1) & 0x007F007F;
	alg2_val |= (cur_val << 1) & 0xFE00FE00;
	alg2_val |= (cur_val >> 8) & 0x00800080;


	uint alg5_neighbor_val = ((neighbor_val & 0x000000080) << 24);
	uint alg5_rng_val = ((uint*)alg5_values)[*rng_seed];
	uint alg5_carry;
	if (code_index == 31)
	{
		alg5_carry = alg5_rng_val;
	}
	else
	{
		alg5_carry = alg5_neighbor_val;
	}
	uint alg5_val = (cur_val & 0x00800000) >> 8;
	alg5_val |= alg5_carry;
	alg5_val |= (cur_val >> 1) & 0x7F007F00;
	alg5_val |= (cur_val << 1) & 0x00FE00FE;
	alg5_val |= (cur_val >> 8) & 0x00010001;


	uint return_val = 0;
	if (algorithm_id == 0)
	{
		return_val = alg0_val;
	}
	if (algorithm_id == 1)
	{
		return_val = alg1_val;
	}
	if (algorithm_id == 2)
	{
		return_val = alg2_val;
	}
	if (algorithm_id == 3)
	{
		return_val = alg3_val;
	}
	if (algorithm_id == 4)
	{
		return_val = alg4_val;
	}
	if (algorithm_id == 5)
	{
		return_val = alg5_val;
	}
	if (algorithm_id == 6)
	{
		return_val = alg6_val;
	}
	if (algorithm_id == 7)
	{
		return_val = alg7_val;
	}


	ushort offset = *rng_seed << 1;
	__global uchar* x = rng_seed_forward_128 + offset;
	__global uchar* y = rng_seed_forward_1 + offset;

	__global uchar* final_ptr;

	if (algorithm_id == 0 || algorithm_id == 1 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 6)
	{
		final_ptr = x;
	}
	if (algorithm_id == 2 || algorithm_id == 5)
	{
		final_ptr = y;
	}
	*rng_seed = *((ushort*)final_ptr);

	//if (algorithm_id == 0 || algorithm_id == 1 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 6)
	//{
	//	*rng_seed = ((ushort*)rng_seed_forward_128)[*rng_seed];
	//}
	//if (algorithm_id == 2 || algorithm_id == 5)
	//{
	//	*rng_seed = ((ushort*)rng_seed_forward_1)[*rng_seed];
	//}

	return return_val;
}


uint run_one_map(uint cur_val, uint code_index, __global uchar* schedule_data, __local uint* local_storage, __local uchar* alg_byte, __global uchar* rng_seed_forward_1, __global uchar* rng_seed_forward_128, __global unsigned char* regular_rng_values, __global unsigned char* alg0_values, __global unsigned char* alg2_values, __global unsigned char* alg5_values, __global unsigned char* alg6_values)
{
	ushort rng_seed = (*(schedule_data) << 8) | *(schedule_data + 1);
	ushort nibble_selector = (*(schedule_data + 2) << 8) | *(schedule_data + 3);

	uchar* cur_val_uchar = (uchar*)&cur_val;

	for (int i = 0; i < 16; i++)
	{
		if ((int)(i / 4) == code_index)
		{
			*alg_byte = cur_val_uchar[i % 4];
		}

		barrier(CLK_LOCAL_MEM_FENCE);

		uchar current_byte = *alg_byte;

		uchar nibble = (nibble_selector >> 15) & 0x01;
		nibble_selector = nibble_selector << 1;

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		uint algorithm_id = (current_byte >> 1) & 0x07;

		cur_val = alg2(cur_val, algorithm_id, &rng_seed, local_storage, rng_seed_forward_1, rng_seed_forward_128, regular_rng_values, alg0_values, alg2_values, alg5_values, alg6_values);
	}

	return cur_val;
}

__kernel void expand_test(uint key, uint data_start, __global uchar* expansion_values, __global uchar* result_storage)
{
	uint code_index = get_code_index();
	uint rng_seed = get_rng_seed_from_key(key);

	uint working_val_uint = get_init_value(key, data_start, code_index);
	working_val_uint = expand_value(working_val_uint, code_index, rng_seed, expansion_values);
	store_val(working_val_uint, result_storage);
}

__kernel void alg_test(uint algorithm_id, ushort rng_seed, __global uchar* input_data, __global uchar* result_storage, __global uchar* rng_seed_forward_1, __global uchar* rng_seed_forward_128, __global uchar* regular_rng_values, __global uchar* alg0_values, __global uchar* alg2_values, __global uchar* alg5_values, __global uchar* alg6_values)
{
	uint code_index = get_code_index();
	__local uint working_code_storage[32];


	uint working_val_uint = fetch_val(input_data);
	working_val_uint = alg(working_val_uint, algorithm_id, &rng_seed, working_code_storage, rng_seed_forward_1, rng_seed_forward_128, regular_rng_values, alg0_values, alg2_values, alg5_values, alg6_values);
	store_val(working_val_uint, result_storage);
} 

__kernel void all_maps_test(uint key, uint data_start, __global uchar* schedule_data, __global uchar* result_storage, __global uchar* expansion_values, __global uchar* rng_seed_forward_1, __global uchar* rng_seed_forward_128, __global uchar* regular_rng_values, __global uchar* alg0_values, __global uchar* alg2_values, __global uchar* alg5_values, __global uchar* alg6_values)
{ 
	uint code_index = get_code_index();
	__local uint working_code_storage[32];
	__local uchar alg_byte;

	
	uint rng_seed = get_rng_seed_from_key(key);
	uint working_val_uint = get_init_value(key, data_start, code_index);
	working_val_uint = expand_value(working_val_uint, code_index, rng_seed, expansion_values);
	
	for (int i = 0; i < 27; i++)
	{
		__global uchar* sched_ptr = schedule_data + i * 4;
		working_val_uint = run_one_map(working_val_uint, code_index, sched_ptr, working_code_storage, &alg_byte, rng_seed_forward_1, rng_seed_forward_128, regular_rng_values, alg0_values, alg2_values, alg5_values, alg6_values);
	}
	
	//store_val(working_val_uint, result_storage);
}