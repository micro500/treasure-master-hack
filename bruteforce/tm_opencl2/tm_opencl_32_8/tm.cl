__kernel void tm_process(__global unsigned char* code_space, __global unsigned char * regular_rng_values, __global unsigned char * alg0_values, __global unsigned char * alg6_values, __global unsigned char * rng_forward_1, __global unsigned char * rng_forward_128, __global unsigned char * alg2_values, __global unsigned char * alg5_values)
{
	__local unsigned char working_code[128];
	__local unsigned int * cur_local_int = working_code + get_local_id(0) * 4;

	unsigned int temp1 = get_global_id(0) / 32;
	unsigned int temp2 = get_global_id(1);

	__global unsigned char * start_addr = code_space + (temp1 * 10000 + temp2) * (128+2+1);
	__global unsigned int * cur_int = start_addr + 3 + get_local_id(0) * 4;

	int algorithm_id = *start_addr;
	//unsigned short rng_seed = *(__global unsigned short*)(start_addr + 1);
	unsigned short rng_seed = (*(start_addr + 1) << 8) | *(start_addr + 2);

	// Copy the int out of global memory into local
	*cur_local_int = *cur_int;

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
}
