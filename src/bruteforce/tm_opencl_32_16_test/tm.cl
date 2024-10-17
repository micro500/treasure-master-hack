__kernel void tm_process(__global unsigned char* code_space, __global unsigned char * regular_rng_values, __global unsigned char * alg0_values, __global unsigned char * alg6_values, __global unsigned char * rng_forward_1, __global unsigned char * rng_forward_128, __global unsigned char * alg2_values, __global unsigned char * alg5_values)
{
	__local unsigned char working_code[128*2];
	__local unsigned int * cur_local_int = working_code + get_local_id(0) * 4;

	unsigned int temp1 = get_global_id(0) / 64;
	unsigned int temp2 = get_global_id(1);

	__global unsigned char * start_addr = code_space + (temp1 * 10000 + temp2) * (128*2+2+1);
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
/*		case 0x00:
			working_code2[int_addr] = ((working_code1[int_addr] << 1) & 0xfefefefe) | rng_space[rng_seed].alg0_values[int_id];
			break;
		case 0x01:
			working_code2[int_addr] = (((working_code1[int_addr] & 0xFF00FF00) + rng_space[rng_seed].arth_high_values[int_id]) & 0xFF00FF00) | (((working_code1[int_addr] & 0x00FF00FF) + rng_space[rng_seed].arth_low_values[int_id]) & 0x00FF00FF);
			break;
		case 0x02:
			blah = (working_code1[int_addr] & 0x00010000) >> 8;
			if (int_id == 31)
			{
				blah = blah | ((rng_space[rng_seed].alg2_value & 0x000000001) << 24);
			}
			else
			{
				blah = blah | ((working_code1[int_addr+1] & 0x000000001) << 24);
			}
			working_code2[int_addr] = ((working_code1[int_addr] >> 1) & 0x007f007f) | ((working_code1[int_addr] >> 8) & 0x00800080) | ((working_code1[int_addr] << 1) & 0xfe00fe00) | (blah & 0x01000100);
			break;
		case 0x03:
			working_code2[int_addr] = working_code1[int_addr] ^ rng_space[rng_seed].regular_values[int_id];
			break;
		case 0x04:
			working_code2[int_addr] = (((working_code1[int_addr] | 0x00FF00FF) -  rng_space[rng_seed].arth_high_values[int_id]) & 0xFF00FF00) | (((working_code1[int_addr] | 0xFF00FF00) -  rng_space[rng_seed].arth_low_values[int_id]) & 0x00FF00FF);
			break;
		case 0x05:
			blah = (working_code1[int_addr] & 0x00800000) >> 8;
			if (int_id == 31)
			{
				blah = blah | ((rng_space[rng_seed].alg5_value & 0x000000080) << 24);
			}
			else
			{
				blah = blah | ((working_code1[int_addr+1] & 0x000000080) << 24);
			}

			working_code2[int_addr] = ((working_code1[int_addr] << 1) & 0x00fe00fe) | ((working_code1[int_addr] >> 8) & 0x00010001) | ((working_code1[int_addr] >> 1) & 0x7f007f00) | (blah & 0x80008000);

			break;
		case 0x06:
			working_code2[int_addr] = ((working_code1[int_addr] >> 1) & 0x7f7f7f7f) | rng_space[rng_seed].alg6_values[int_id];
			break;
*/
		case 0x00:
			temp = ((*cur_local_int << 1) & 0x00FE00FE) | *(__global unsigned int*)(alg0_values + rng_seed * 128 * 2 +  get_local_id(0) * 4);
			break;
		case 0x01:
			temp = *cur_local_int + *(__global unsigned int*)(regular_rng_values + rng_seed * 128 * 2 +  get_local_id(0) * 4);
			temp = temp & 0x00FF00FF;
			break;
		case 0x02:
			if (get_local_id(0) == 63)
			{
				temp = *(__global unsigned int *)(alg2_values + rng_seed * 4);
			}
			else
			{
				temp = ((*(cur_local_int + 1) & 0x000000001) << 16);
			}
			temp |= (*cur_local_int >> 1) & 0x0000007F;
			temp |= (*cur_local_int << 1) & 0x00FE0000;
			temp |= (*cur_local_int >> 16) & 0x00000080;
			break;
		case 0x03:
			temp = *cur_local_int ^ *(__global unsigned int*)(regular_rng_values + rng_seed * 128 * 2 +  get_local_id(0) * 4);
			break;
		case 0x04:
			temp = *cur_local_int + (*(__global unsigned int*)(regular_rng_values + rng_seed * 128 * 2 +  get_local_id(0) * 4) ^ 0x00FF00FF);
			temp = temp + 0x00010001;
			temp = temp & 0x00FF00FF;
			break;
		case 0x05:
			if (get_local_id(0) == 63)
			{
				temp = *(__global unsigned int *)(alg5_values + rng_seed * 4);
			}
			else
			{
				temp = ((*(cur_local_int + 1) & 0x000000080) << 16);
			}
			temp |= (*cur_local_int >> 1) & 0x007F0000;
			temp |= (*cur_local_int << 1) & 0x000000FE;
			temp |= (*cur_local_int >> 16) & 0x00000001;
			//temp = 5;
			break;
		case 0x06:
			temp = ((*cur_local_int >> 1) & 0x007F007F) | *(__global unsigned int*)(alg6_values + rng_seed * 128 * 2 +  get_local_id(0) * 4);
			break;
		case 0x07:
			temp = *cur_local_int ^ 0x00FF00FF;
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
