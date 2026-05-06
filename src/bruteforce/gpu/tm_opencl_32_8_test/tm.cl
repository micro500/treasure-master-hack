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

// -------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------
#define CHECKSUM_SENTINEL    0x08
#define OP_JAM               0x01
#define OP_ILLEGAL           0x02
#define OP_NOP2              0x04
#define OP_NOP_OP            0x08
#define OP_JUMP              0x10

#define OTHER_WORLD          0x01
#define FIRST_ENTRY_VALID    0x02
#define ALL_ENTRIES_VALID    0x04
#define USES_NOP             0x10
#define USES_UNOFFICIAL_NOPS 0x20
#define USES_ILLEGAL_OPCODES 0x40
#define USES_JAM             0x80

__constant unsigned char other_world_data_k[128] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0xCA,0x68,0xC1,0x66,0x44,
	0xD2,0x04,0x0B,0x90,0x81,0x86,0xC7,0xF4,0xD2,0xE2,
	0xF1,0x22,0xE3,0x0C,0xD9,0x54,0xFB,0xFF,0x0A,0xCF,
	0x81,0x72,0x0A,0x94,0x9A,0x98,0xD3,0xFF,0xAB,0x80,
	0x9A,0xE5,0xB7,0x45,0x6E,0x8F,0xD2,0xF0,0x67,0xFF,
	0xB3,0xAE,0x49,0xBB,0x9C,0x06,0x12,0x40,0x49,0xA3,
	0x9A,0xDB,0x32,0x7B,0x58,0xA1,0x5A,0xB9,0x2B,0x2B,
	0x2D,0x6E,0x36,0x93,0x1C,0x1A,0x52,0x03,0x18,0xE4,
	0x5E,0xB1,0xC1,0xBD,0x44,0xFB,0xF1,0x50
};

__constant unsigned char opcode_bytes_used_k[256] = {
	1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
	3,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
	1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
	1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
	2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,0,3,0,0,
	2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,3,3,3,0,
	2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0,
	2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
	2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0
};

// OP_JAM=1,OP_ILLEGAL=2,OP_NOP2=4,OP_NOP=8,OP_JUMP=16
__constant unsigned char opcode_type_k[256] = {
	 0, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2, 4, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
	16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2,16, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 0, 2,16, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
	 4, 0, 4, 2, 0, 0, 0, 2, 0, 4, 0, 2, 0, 0, 0, 2,
	16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 0, 2, 2,
	 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
	16, 0, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 4, 2,
	 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2,
	 0, 0, 4, 2, 0, 0, 0, 2, 0, 0, 8, 2, 0, 0, 0, 2,
	16, 0, 1, 2, 4, 0, 0, 2, 0, 0, 4, 2, 4, 0, 0, 2
};

// -------------------------------------------------------------------
// checksum_ok: run by thread 0 only
// -------------------------------------------------------------------
int checksum_ok(__local unsigned int* data_i, int length)
{
	__local unsigned char* data = (__local unsigned char*)data_i;
	unsigned int checksum_total = ((unsigned int)data[127 - (length - 1)] << 8)
	                            |  (unsigned int)data[127 - (length - 2)];
	if (checksum_total > (unsigned int)(length - 2) * 255u)
		return 0;
	unsigned int sum = 0;
	for (int i = 0; i < length - 2; i++)
		sum += data[127 - i];
	return (sum == checksum_total);
}

// -------------------------------------------------------------------
// machine_code_flags: run by thread 0 only
// e0..e5 are entry addresses (0xFF = unused)
// -------------------------------------------------------------------
unsigned char machine_code_flags(__local unsigned int* data_i, int code_length,
                                  unsigned char e0, unsigned char e1, unsigned char e2,
                                  unsigned char e3, unsigned char e4, unsigned char e5)
{
	__local unsigned char* data = (__local unsigned char*)data_i;
	unsigned char entry_addrs[6] = { e0, e1, e2, e3, e4, e5 };
	unsigned char active_entries[6]  = { 0,0,0,0,0,0 };
	unsigned char hit_entries[6]     = { 0,0,0,0,0,0 };
	unsigned char valid_entries[6]   = { 0,0,0,0,0,0 };
	int last_entry = -1;
	unsigned char result = 0;
	unsigned char next_entry_addr = e0;

	for (int i = 0; i < code_length - 2; i++)
	{
		if (i == (int)next_entry_addr)
		{
			last_entry++;
			hit_entries[last_entry] = 1;
			active_entries[last_entry] = 1;
			next_entry_addr = entry_addrs[last_entry + 1];
		}
		else if (i > (int)next_entry_addr)
		{
			last_entry++;
			next_entry_addr = entry_addrs[last_entry + 1];
		}

		unsigned char opcode = data[127 - i];
		unsigned char otype  = opcode_type_k[opcode];

		if (otype & OP_JAM)
		{
			result |= USES_JAM;
			break;
		}
		else if (otype & OP_ILLEGAL)
		{
			result |= USES_ILLEGAL_OPCODES;
			break;
		}
		else if (otype & OP_NOP2)
		{
			result |= USES_UNOFFICIAL_NOPS;
		}
		else if (otype & OP_NOP_OP)
		{
			result |= USES_NOP;
		}
		else if (otype & OP_JUMP)
		{
			for (int j = 0; j < 5; j++)
			{
				if (active_entries[j] == 1)
				{
					active_entries[j] = 0;
					valid_entries[j]  = 1;
				}
			}
		}

		i += (int)opcode_bytes_used_k[opcode] - 1;
	}

	int all_entries_valid = 1;
	for (int i = 0; i < 5; i++)
	{
		if (hit_entries[i] == 1)
		{
			if (valid_entries[i] != 1)
			{
				all_entries_valid = 0;
				break;
			}
		}
		else if (entry_addrs[i] == 255)
		{
			continue;
		}
		else
		{
			for (int j = (int)entry_addrs[i]; j < code_length - 2; j++)
			{
				unsigned char opcode = data[127 - j];
				unsigned char otype  = opcode_type_k[opcode];
				if ((otype & OP_JAM) || (otype & OP_ILLEGAL))
				{
					all_entries_valid = 0;
					break;
				}
				else if (otype & OP_JUMP)
				{
					break;
				}
				j += (int)opcode_bytes_used_k[opcode] - 1;
			}
		}
		if (!all_entries_valid) break;
	}

	if (all_entries_valid)       result |= ALL_ENTRIES_VALID;
	if (valid_entries[0] == 1)   result |= FIRST_ENTRY_VALID;

	return result;
}

// -------------------------------------------------------------------
// tm_stats kernel
// -------------------------------------------------------------------
__kernel void tm_stats(
	__global unsigned char*  result_data,
	__global unsigned char*  regular_rng_values,
	__global unsigned char*  alg0_values,
	__global unsigned char*  alg6_values,
	__global unsigned short* rng_forward_1,
	__global unsigned short* rng_forward_128,
	__global unsigned char*  alg2_values,
	__global unsigned char*  alg5_values,
	__global unsigned char*  expansion_values,
	__global unsigned char*  schedule_data,
	__global unsigned char*  carnival_data,
	__global unsigned char*  unused_param,
	unsigned int key,
	unsigned int data_start)
{
	__local unsigned int working_code[32];
	__local unsigned int decrypted_carnival[32];
	__local unsigned int decrypted_other[32];

	unsigned int code_index = get_global_id(1);
	unsigned int int_index  = get_local_id(0);
	unsigned int cur_data   = data_start + code_index;

	if (int_index % 2 == 0)
		working_code[int_index] = key;
	else
		working_code[int_index] = cur_data;

	expand(working_code, int_index, key >> 16, expansion_values);
	barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = 0; i < 27; i++)
	{
		run_one_map(working_code, int_index,
		            regular_rng_values, alg0_values, alg6_values,
		            rng_forward_1, rng_forward_128,
		            alg2_values, alg5_values,
		            schedule_data, i);
	}

	// All 32 threads cooperatively decrypt both worlds
	decrypted_carnival[int_index] = working_code[int_index]
	                              ^ ((__global unsigned int*)carnival_data)[int_index];
	decrypted_other[int_index]    = working_code[int_index]
	                              ^ ((__constant unsigned int*)other_world_data_k)[int_index];

	barrier(CLK_LOCAL_MEM_FENCE);

	if (int_index == 0)
	{
		unsigned char stats = 0;

		if (checksum_ok(decrypted_carnival, 0x72))
		{
			// Carnival world checksum passed
			stats = machine_code_flags(decrypted_carnival, 0x72,
			                           0, 0x2B, 0x33, 0x3E, 0xFF, 0xFF);
			stats |= CHECKSUM_SENTINEL;
		}
		else if (checksum_ok(decrypted_other, 0x53))
		{
			// Other world checksum passed
			stats = machine_code_flags(decrypted_other, 0x53,
			                           0, 0x05, 0x0A, 0x28, 0x50, 0xFF);
			stats |= CHECKSUM_SENTINEL | OTHER_WORLD;
		}

		result_data[code_index] = stats;
	}
}
