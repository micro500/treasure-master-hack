#define reverse_offset(x) (127 - (x))

__kernel void tm_process(__global unsigned char* code_space, __global unsigned char * regular_rng_values, __global unsigned char * alg0_values, __global unsigned char * alg6_values, __global unsigned char * rng_forward_1, __global unsigned char * rng_forward_128, __global unsigned char * alg2_values, __global unsigned char * alg5_values)
{
	__local unsigned int working_code[32];
	__local unsigned int * cur_local_int = working_code + get_local_id(0);

	unsigned int temp1 = get_global_id(0) / 32;
	unsigned int temp2 = get_global_id(1);

	__global unsigned char * start_addr = code_space + (temp1 * 10000 + temp2) * 132;
	__global unsigned int * cur_int = (__global unsigned int *)((__global unsigned char *)start_addr + 4 + get_local_id(0) * 4);

	int algorithm_id = *start_addr;
	//unsigned short rng_seed = *(__global unsigned short*)(start_addr + 1);
	unsigned short rng_seed = (*(start_addr + 1) << 8) | *(start_addr + 2);
	
	// Copy the int out of global memory into local
	*cur_local_int = *cur_int;
	
	//*((__global unsigned int*)code_space) = 0x23456789;
	//*((__global unsigned int*)cur_int) = 0x34567890;
	//*start_addr = temp2;
	//return;

	/*
	if (get_local_id(0) == 0)
	{
		*cur_int = (temp1 * 10000 + temp2);
	}
	else
	{
		*cur_int = get_local_id(0);
	}
	*/

	int temp;
	switch (algorithm_id)
	{
		case 0x00:
			temp = ((*cur_local_int << 1) & 0xFEFEFEFE) | *(__global unsigned int*)(alg0_values + rng_seed * 128 +  get_local_id(0) * 4);
			break;
		case 0x01:
			temp = ((*cur_local_int & 0x00FF00FF) + ((*(__global unsigned int*)(regular_rng_values + rng_seed * 128 +  get_local_id(0) * 4)) & 0x00FF00FF)) & 0x00FF00FF;
			temp |= ((*cur_local_int & 0xFF00FF00) + ((*(__global unsigned int*)(regular_rng_values + rng_seed * 128 +  get_local_id(0) * 4)) & 0xFF00FF00)) & 0xFF00FF00;
			break;
		// TODO
		case 0x02:
			temp = (*cur_local_int & 0x00010000) >> 8;
			if (get_local_id(0) == 31)
			{
				temp |= *(__global unsigned int *)(alg2_values + rng_seed * 4);
			}
			else
			{
				temp |= ((*(cur_local_int + 1) & 0x000000001) << 24);
			}
			temp |= (*cur_local_int >> 1) & 0x007F007F;
			temp |= (*cur_local_int << 1) & 0xFE00FE00;
			temp |= (*cur_local_int >> 8) & 0x00800080;
			break;
		case 0x03:
			temp = *cur_local_int ^ *(__global unsigned int*)(regular_rng_values + rng_seed * 128 +  get_local_id(0) * 4);
			break;
	
		// TODO
		case 0x04:
			temp = ((*cur_local_int & 0x00FF00FF) + (((*(__global unsigned int*)(regular_rng_values + rng_seed * 128 +  get_local_id(0) * 4)) & 0x00FF00FF) ^ 0x00FF00FF) + 0x00010001) & 0x00FF00FF;
			temp |= ((*cur_local_int & 0xFF00FF00) + (((*(__global unsigned int*)(regular_rng_values + rng_seed * 128 +  get_local_id(0) * 4)) & 0xFF00FF00) ^ 0xFF00FF00) + 0x01000100) & 0xFF00FF00;
			break;
		// TODO
		case 0x05:
			temp = (*cur_local_int & 0x00800000) >> 8;
			if (get_local_id(0) == 31)
			{
				temp |= *(__global unsigned int *)(alg5_values + rng_seed * 4);
			}
			else
			{
				temp |= ((*(cur_local_int + 1) & 0x000000080) << 24);
			}
			temp |= (*cur_local_int >> 1) & 0x7F007F00;
			temp |= (*cur_local_int << 1) & 0x00FE00FE;
			temp |= (*cur_local_int >> 8) & 0x00010001;
			//temp = 5;
			break;
		case 0x06:
			temp = ((*cur_local_int >> 1) & 0x7F7F7F7F) | *(__global unsigned int*)(alg6_values + rng_seed * 128 +  get_local_id(0) * 4);
			break;
		case 0x07:
			temp = *cur_local_int ^ 0xFFFFFFFF;
			break;
		default:
			break;
	}
	
	// Make sure each WU is done computing
	barrier(CLK_LOCAL_MEM_FENCE);

	// Store the result back into local memory
	*cur_local_int = temp;
	
	if (get_local_id(0) == 0 && (algorithm_id == 3 || algorithm_id == 1 || algorithm_id == 4 || algorithm_id == 0 || algorithm_id == 6))
	{
		*(start_addr + 1+1) = rng_forward_128[rng_seed*2];
		*(start_addr + 1) = rng_forward_128[rng_seed*2+1];
	}
	else if (get_local_id(0) == 0 && (algorithm_id == 2 || algorithm_id == 5))
	{
		*(start_addr + 1+1) = rng_forward_1[rng_seed*2];
		*(start_addr + 1) = rng_forward_1[rng_seed*2+1];
	}
	

	// Copy the result back to global memory
	*cur_int = *cur_local_int;
	//*cur_int = 0x12345678;
	//*code_space = 0x23456789;
}

// expansion
// kernel:
//	take in IV
//  expand it into local memory
//  write local memory to global memory

void copy_from_global(__global unsigned char* code_space, __local unsigned int * working_code, int code_index, int int_index)
{
	working_code[int_index] = ((__global unsigned int*)code_space)[code_index * 32 + int_index];
}

void copy_to_global(__global unsigned char* code_space, __local unsigned int * working_code, int code_index, int int_index)
{
	((__global unsigned int*)code_space)[code_index * 32 + int_index] = working_code[int_index]; 
}

void expand(__local unsigned int * working_code, int int_index, unsigned short rng_seed, __global unsigned char * expansion_values)
{
	unsigned int temp;
	temp = ((working_code[int_index] & 0x00FF00FF) + ((*(__global unsigned int*)(expansion_values + rng_seed * 128 + int_index * 4)) & 0x00FF00FF)) & 0x00FF00FF;
	temp |= ((working_code[int_index] & 0xFF00FF00) + ((*(__global unsigned int*)(expansion_values + rng_seed * 128 + int_index * 4)) & 0xFF00FF00)) & 0xFF00FF00;
	working_code[int_index] = temp;
}

__kernel void test_expand(__global unsigned char* code_space, __global unsigned char* input_ivs, __global unsigned char* expansion_values)
{
	__local unsigned int working_code[32];

	unsigned int code_index = get_global_id(1);
	unsigned int int_index = get_local_id(0);

	unsigned int key_half = int_index % 2;
	working_code[int_index] = *((__global unsigned int *)(input_ivs + code_index * 8 + key_half * 4));
	unsigned short rng_seed = (*(input_ivs + code_index * 8) << 8) | *(input_ivs + code_index * 8 + 1);

	expand(working_code, int_index, rng_seed, expansion_values);
	copy_to_global(code_space, working_code, code_index, int_index);
}

void run_alg(__local unsigned int * working_code, int int_index, int alg_id, unsigned short * rng_seed, __global unsigned char * regular_rng_values, __global unsigned char * alg0_values, __global unsigned char * alg6_values, __global unsigned short * rng_forward_1, __global unsigned short * rng_forward_128, __global unsigned char * alg2_values, __global unsigned char * alg5_values)
{
	__local unsigned int * cur_local_int = working_code + int_index;
		
	int temp;
	switch (alg_id)
	{
		case 0x00:
			temp = ((*cur_local_int << 1) & 0xFEFEFEFE) | *(__global unsigned int*)(alg0_values + *rng_seed * 128 +  get_local_id(0) * 4);
			break;
		case 0x01:
			temp = ((*cur_local_int & 0x00FF00FF) + ((*(__global unsigned int*)(regular_rng_values + *rng_seed * 128 +  get_local_id(0) * 4)) & 0x00FF00FF)) & 0x00FF00FF;
			temp |= ((*cur_local_int & 0xFF00FF00) + ((*(__global unsigned int*)(regular_rng_values + *rng_seed * 128 +  get_local_id(0) * 4)) & 0xFF00FF00)) & 0xFF00FF00;
			break;
		// TODO
		case 0x02:
			temp = (*cur_local_int & 0x00010000) >> 8;
			if (get_local_id(0) == 31)
			{
				temp |= *(__global unsigned int *)(alg2_values + *rng_seed * 4);
			}
			else
			{
				temp |= ((*(cur_local_int + 1) & 0x000000001) << 24);
			}
			temp |= (*cur_local_int >> 1) & 0x007F007F;
			temp |= (*cur_local_int << 1) & 0xFE00FE00;
			temp |= (*cur_local_int >> 8) & 0x00800080;
			break;
		case 0x03:
			temp = *cur_local_int ^ *(__global unsigned int*)(regular_rng_values + *rng_seed * 128 +  get_local_id(0) * 4);
			break;
	
		// TODO
		case 0x04:
			temp = ((*cur_local_int & 0x00FF00FF) + (((*(__global unsigned int*)(regular_rng_values + *rng_seed * 128 +  get_local_id(0) * 4)) & 0x00FF00FF) ^ 0x00FF00FF) + 0x00010001) & 0x00FF00FF;
			temp |= ((*cur_local_int & 0xFF00FF00) + (((*(__global unsigned int*)(regular_rng_values + *rng_seed * 128 +  get_local_id(0) * 4)) & 0xFF00FF00) ^ 0xFF00FF00) + 0x01000100) & 0xFF00FF00;
			break;
		// TODO
		case 0x05:
			temp = (*cur_local_int & 0x00800000) >> 8;
			if (get_local_id(0) == 31)
			{
				temp |= *(__global unsigned int *)(alg5_values + *rng_seed * 4);
			}
			else
			{
				temp |= ((*(cur_local_int + 1) & 0x000000080) << 24);
			}
			temp |= (*cur_local_int >> 1) & 0x7F007F00;
			temp |= (*cur_local_int << 1) & 0x00FE00FE;
			temp |= (*cur_local_int >> 8) & 0x00010001;
			//temp = 5;
			break;
		case 0x06:
			temp = ((*cur_local_int >> 1) & 0x7F7F7F7F) | *(__global unsigned int*)(alg6_values + *rng_seed * 128 +  get_local_id(0) * 4);
			break;
		case 0x07:
			temp = *cur_local_int ^ 0xFFFFFFFF;
			break;
		default:
			temp = alg_id;
			break;
	}
	
	// Make sure each WU is done computing
	barrier(CLK_LOCAL_MEM_FENCE);

	// Store the result back into local memory
	*cur_local_int = temp;
	
	if (alg_id == 3 || alg_id == 1 || alg_id == 4 || alg_id == 0 || alg_id == 6)
	{
		*rng_seed = (*((__global unsigned char *)rng_forward_128 + *rng_seed * 2 + 1) << 8) | *((__global unsigned char *)rng_forward_128 + *rng_seed * 2);
	}
	else if (alg_id == 2 || alg_id == 5)
	{
		*rng_seed = (*((__global unsigned char *)rng_forward_1 + *rng_seed * 2 + 1) << 8) | *((__global unsigned char *)rng_forward_1 + *rng_seed * 2);
	}
}

__kernel void test_alg(__global unsigned char* code_space, __global unsigned char * test_data, __global unsigned char * regular_rng_values, __global unsigned char * alg0_values, __global unsigned char * alg6_values, __global unsigned short * rng_forward_1, __global unsigned short * rng_forward_128, __global unsigned char * alg2_values, __global unsigned char * alg5_values)
{
	__local unsigned int working_code[32];
	unsigned short rng_seed;

	unsigned int code_index = get_global_id(1);
	unsigned int int_index = get_local_id(0);

	__global unsigned char * cur_test_data = test_data + code_index * 3;

	// get alg id, rng seed from test data
	int alg_id = *(cur_test_data);
	
	rng_seed = *(cur_test_data + 1) << 8 | *(cur_test_data + 2);

	copy_from_global(code_space, working_code, code_index, int_index);
	run_alg(working_code, int_index, alg_id, &rng_seed, regular_rng_values, alg0_values, alg6_values, rng_forward_1, rng_forward_128, alg2_values, alg5_values);
	copy_to_global(code_space, working_code, code_index, int_index);
	*(cur_test_data + 1) = (rng_seed >> 8) & 0xFF;
	*(cur_test_data + 2) = rng_seed & 0xFF;
}

void run_one_map(__local unsigned int* working_code, unsigned int int_index, __global unsigned char * regular_rng_values, __global unsigned char * alg0_values, __global unsigned char * alg6_values, __global unsigned short * rng_forward_1, __global unsigned short * rng_forward_128, __global unsigned char * alg2_values, __global unsigned char * alg5_values, __global unsigned char * schedule_data, int schedule_index)
{
	unsigned short rng_seed = (*(schedule_data + schedule_index * 4) << 8) | *(schedule_data + schedule_index * 4 + 1);
	unsigned short nibble_selector = (*(schedule_data + schedule_index * 4 + 2) << 8) | *(schedule_data + schedule_index * 4 + 3);
	
	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		// Get the highest bit of the nibble selector to use as a flag
		unsigned char nibble = (nibble_selector >> 15) & 0x01;
		// Shift the nibble selector up one bit
		nibble_selector = nibble_selector << 1;

		// If the flag is a 1, get the high nibble of the current byte
		// Otherwise use the low nibble
		unsigned char current_byte = *((__local unsigned char*)working_code + i);

		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char alg_id = (current_byte >> 1) & 0x07;

		run_alg(working_code, int_index, alg_id, &rng_seed, regular_rng_values, alg0_values, alg6_values, rng_forward_1, rng_forward_128, alg2_values, alg5_values);
	}

}


__kernel void full_process(__global unsigned char* code_space, __global unsigned char * test_data, __global unsigned char * regular_rng_values, __global unsigned char * alg0_values, __global unsigned char * alg6_values, __global unsigned short * rng_forward_1, __global unsigned short * rng_forward_128, __global unsigned char * alg2_values, __global unsigned char * alg5_values, __global unsigned char * expansion_values, __global unsigned char * schedule_data, unsigned int key, unsigned int data_start)
{
	__local unsigned int working_code[32];
	__local unsigned short rng_seed;

	unsigned int code_index = get_global_id(1);
	unsigned int int_index = get_local_id(0);
	unsigned int cur_data = data_start + code_index;

	if (int_index % 2 == 0)
	{
		working_code[int_index] = key;
	}
	else
	{
		working_code[int_index] = cur_data;
	}

	expand(working_code, int_index, key >> 16, expansion_values);

	for (int i = 0; i < 27; i++)
	{
		run_one_map(working_code, int_index, regular_rng_values, alg0_values, alg6_values, rng_forward_1, rng_forward_128, alg2_values, alg5_values, schedule_data, i);
	}
	copy_to_global(code_space, working_code, code_index, int_index);
}

unsigned char generate_stats(__local unsigned int* working_code, __global unsigned char* encrypted_data, __global unsigned char* checksum_mask, int int_index, unsigned int length)
{
	__local int decrypted_data[32];
	decrypted_data[int_index] = working_code[int_index] ^ ((__global unsigned int*)encrypted_data)[int_index];
	barrier(CLK_LOCAL_MEM_FENCE);

	unsigned short checksum_total = (((__local unsigned char*) decrypted_data)[reverse_offset(length - 1)] << 8) | ((__local unsigned char*) decrypted_data)[reverse_offset(length - 2)];
	unsigned short result = 0;

	if (checksum_total <= (length - 2) * 0xFF)
	{
		unsigned short sum = 0;
		for (int i = 0; i < length - 2; i++)
		{
			sum += ((__local unsigned char*) decrypted_data)[reverse_offset(i)];
		}
		if (sum == checksum_total)
		{
			result |= 0x80;
		}
	}

	return result;
}

__kernel void tm_stats(__global unsigned char* result_data, __global unsigned char* regular_rng_values, __global unsigned char* alg0_values, __global unsigned char* alg6_values, __global unsigned short* rng_forward_1, __global unsigned short* rng_forward_128, __global unsigned char* alg2_values, __global unsigned char* alg5_values, __global unsigned char* expansion_values, __global unsigned char* schedule_data, __global unsigned char* carnival_data, __global unsigned char* carnival_checksum_mask, unsigned int key, unsigned int data_start)
{
	__local unsigned int working_code[32];
	__local unsigned short rng_seed;

	unsigned int code_index = get_global_id(1);
	unsigned int int_index = get_local_id(0);
	unsigned int cur_data = data_start + code_index;

	if (int_index % 2 == 0)
	{
		working_code[int_index] = key;
	}
	else
	{
		working_code[int_index] = cur_data;
	}

	expand(working_code, int_index, key >> 16, expansion_values);

	// make sure expansion values are ready
	barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = 0; i < 27; i++)
	{
		run_one_map(working_code, int_index, regular_rng_values, alg0_values, alg6_values, rng_forward_1, rng_forward_128, alg2_values, alg5_values, schedule_data, i);
	}

	unsigned char stats = generate_stats(working_code, carnival_data, carnival_checksum_mask, int_index, 0x72);
	barrier(CLK_LOCAL_MEM_FENCE);
	if (get_local_id(0) == 0)
	{
		result_data[code_index] = 0;
		result_data[code_index * 2] = stats;
	}
	//copy_to_global(code_space, working_code, code_index, int_index);
}

// decrypt memory
// need carnival code and other world code


// 
// id0: store all 0s to global
// fence

// decrypt: xor current int with data into a new local array
// fence (make sure data is ready)
// check if the sum is too high, skip (all cores do this)
	// mask int with relevant mask
	// add up the 4 bytes, store to a sumation array
	// fence (to have summation ready)
	// id0: add up sumation array, store to local
	// fence
	// if sum matches:
		// id0: 
			// result now indicates checksum sucess
			// run machine code checks
			// store to global

// fence

// same steps for other world, store to a different spot?
