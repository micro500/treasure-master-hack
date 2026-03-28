#ifdef __INTELLISENSE__
#include "opencl_intellisense.h"
#endif

// -------------------------------------------------------------------
// Hash reduction configuration
// To change the cut point, edit this define and recompile.
// -------------------------------------------------------------------
#define HASH_REDUCTION_CUT_POINT 1

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

// OP_JAM=1, OP_ILLEGAL=2, OP_NOP2=4, OP_NOP_OP=8, OP_JUMP=16
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
// Device utility functions
// -------------------------------------------------------------------
__attribute__((always_inline))
uint get_data_offset()
{
	return get_global_id(1);
}

__attribute__((always_inline))
uint get_code_index()
{
	return get_local_id(0);
}

__attribute__((always_inline))
uint get_expand_value(uint code_index, uint rng_seed, __global unsigned char* expansion_values)
{
	return ((uint*)expansion_values)[rng_seed * 32 + code_index];
}

__attribute__((always_inline))
uint expand_value(uint cur_val, uint code_index, uint rng_seed, __global unsigned char* expansion_values)
{
	uchar4* working_val_uchar4 = (uchar4*)&cur_val;

	uint ex_val = get_expand_value(code_index, rng_seed, expansion_values);
	uchar4* ex_val_uchar4 = (uchar4*)&ex_val;

	*working_val_uchar4 = *working_val_uchar4 + *ex_val_uchar4;

	return cur_val;
}

__attribute__((always_inline))
uint get_init_data(uint data_start)
{
	uint data_offset = get_data_offset();
	uint data = data_start + data_offset;

	return data;
}

__attribute__((always_inline))
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

__attribute__((always_inline))
uint fetch_val(__global uchar* storage_mem)
{
	uint data_offset = get_data_offset();
	uint code_index = get_code_index();

	uint* s = (uint*)storage_mem;
	return s[data_offset * 32 + code_index];
}

__attribute__((always_inline))
void store_val(uint val, __global uchar* storage_mem)
{
	uint data_offset = get_data_offset();
	uint code_index = get_code_index();

	uint* s = (uint*)storage_mem;
	s[data_offset * 32 + code_index] = val;
}

__attribute__((always_inline))
uint get_rng_val(uint rng_seed, __global uchar* rng_mem)
{
	uint code_index = get_code_index();

	uint* s = (uint*)rng_mem;
	return s[rng_seed * 32 + code_index];
}


// -------------------------------------------------------------------
// Algorithm step
// -------------------------------------------------------------------
__attribute__((always_inline))
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

	uint offset = (uint)*rng_seed * 2;

	if (algorithm_id == 0 || algorithm_id == 1 || algorithm_id == 3 || algorithm_id == 4 || algorithm_id == 6)
	{
		__global uchar* x = rng_seed_forward_128 + offset;
		*rng_seed = *((ushort*)x);
	}
	if (algorithm_id == 2 || algorithm_id == 5)
	{
		__global uchar* y = rng_seed_forward_1 + offset;
		*rng_seed = *((ushort*)y);
	}

	return cur_val;
}

// Precomputes all 8 algorithm results and selects, avoiding divergence
__attribute__((always_inline))
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

	uint alg7_val = cur_val ^ 0xFFFFFFFF;

	uint code_index = get_code_index();
	((uint*)local_storage)[code_index] = cur_val;
	barrier(CLK_LOCAL_MEM_FENCE);

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
	if (algorithm_id == 0) return_val = alg0_val;
	if (algorithm_id == 1) return_val = alg1_val;
	if (algorithm_id == 2) return_val = alg2_val;
	if (algorithm_id == 3) return_val = alg3_val;
	if (algorithm_id == 4) return_val = alg4_val;
	if (algorithm_id == 5) return_val = alg5_val;
	if (algorithm_id == 6) return_val = alg6_val;
	if (algorithm_id == 7) return_val = alg7_val;

	uint offset = (uint)*rng_seed * 2;
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

	return return_val;
}

// -------------------------------------------------------------------
// Map execution
// -------------------------------------------------------------------
__attribute__((always_inline))
uint run_one_map(uint cur_val, uint code_index, __global uchar* schedule_data, __local uint* local_storage, __local uchar* alg_byte, __global uchar* rng_seed_forward_1, __global uchar* rng_seed_forward_128, __global unsigned char* regular_rng_values, __global unsigned char* alg0_values, __global unsigned char* alg2_values, __global unsigned char* alg5_values, __global unsigned char* alg6_values)
{
	ushort rng_seed = (*(schedule_data) << 8) | *(schedule_data + 1);
	ushort nibble_selector = (*(schedule_data + 2) << 8) | *(schedule_data + 3);

	uchar* cur_val_uchar = (uchar*)&cur_val;

	for (int i = 0; i < 16; i++)
	{
		if ((int)(i / 4) == (int)code_index)
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

		cur_val = alg(cur_val, algorithm_id, &rng_seed, local_storage, rng_seed_forward_1, rng_seed_forward_128, regular_rng_values, alg0_values, alg2_values, alg5_values, alg6_values);
	}

	return cur_val;
}

// -------------------------------------------------------------------
// Checksum and machine code validation (for tm_bruteforce)
// -------------------------------------------------------------------

// Returns 1 if checksum passes, 0 otherwise. Run by thread 0 only.
__attribute__((always_inline))
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

// Returns machine code flags. Run by thread 0 only.
__attribute__((always_inline))
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
// Batch test kernels
//
// Launch dimensions: global={32, count, 1}, local={32, 1, 1}
// fetch_val/store_val use get_global_id(1) as item index automatically.
// Per-item keys/datas/alg_ids are read from arrays indexed by item_index.
// -------------------------------------------------------------------

__kernel void expand_test_batch(
	__global uchar* keys,
	__global uchar* datas,
	__global uchar* expansion_values,
	__global uchar* result_storage)
{
	uint item_index = get_global_id(1);
	uint code_index = get_code_index();

	__global uchar* key_bytes = keys  + item_index * 4;
	__global uchar* dat_bytes = datas + item_index * 4;

	ushort rng_seed = ((ushort)key_bytes[0] << 8) | key_bytes[1];

	__global uchar* src = (code_index % 2 == 0) ? key_bytes : dat_bytes;
	uint working_val = *((__global uint*)src);
	working_val = expand_value(working_val, code_index, rng_seed, expansion_values);
	store_val(working_val, result_storage);
}

__kernel void alg_test_batch(
	__global uchar*  alg_ids,
	__global ushort* rng_seeds_in,
	__global uchar*  input_data,
	__global uchar*  result_storage,
	__global uchar*  rng_seed_forward_1,
	__global uchar*  rng_seed_forward_128,
	__global uchar*  regular_rng_values,
	__global uchar*  alg0_values,
	__global uchar*  alg2_values,
	__global uchar*  alg5_values,
	__global uchar*  alg6_values,
	__global ushort* rng_seeds_out)
{
	uint item_index = get_global_id(1);
	uint code_index = get_code_index();
	__local uint working_code_storage[32];

	uint  alg_id   = alg_ids[item_index];
	ushort rng_seed = rng_seeds_in[item_index];

	// fetch_val/store_val already use get_global_id(1) to index into flat N*128 buffer
	uint working_val = fetch_val(input_data);
	working_val = alg(working_val, alg_id, &rng_seed, working_code_storage,
	                  rng_seed_forward_1, rng_seed_forward_128, regular_rng_values,
	                  alg0_values, alg2_values, alg5_values, alg6_values);
	store_val(working_val, result_storage);
	if (code_index == 0)
		rng_seeds_out[item_index] = rng_seed;
}

__attribute__((reqd_work_group_size(32, 1, 1)))
__kernel void all_maps_test_batch(
	__global uchar* keys,
	__global uchar* datas,
	__global uchar* schedule_data,    // count * schedule_count * 4 bytes
	__global uchar* result_storage,   // count * 128 bytes
	__global uchar* expansion_values,
	__global uchar* rng_seed_forward_1,
	__global uchar* rng_seed_forward_128,
	__global uchar* regular_rng_values,
	__global uchar* alg0_values,
	__global uchar* alg2_values,
	__global uchar* alg5_values,
	__global uchar* alg6_values,
	int             schedule_count)
{
	uint item_index = get_global_id(1);
	uint code_index = get_code_index();
	__local uint working_code_storage[32];
	__local uchar alg_byte;

	__global uchar* key_bytes = keys  + item_index * 4;
	__global uchar* dat_bytes = datas + item_index * 4;

	ushort rng_seed = ((ushort)key_bytes[0] << 8) | key_bytes[1];

	__global uchar* src = (code_index % 2 == 0) ? key_bytes : dat_bytes;
	uint working_val = *((__global uint*)src);
	working_val = expand_value(working_val, code_index, rng_seed, expansion_values);

	__global uchar* my_schedule = schedule_data + item_index * schedule_count * 4;
	for (int i = 0; i < schedule_count; i++)
	{
		working_val = run_one_map(working_val, code_index, my_schedule + i * 4,
		                         working_code_storage, &alg_byte,
		                         rng_seed_forward_1, rng_seed_forward_128,
		                         regular_rng_values, alg0_values, alg2_values,
		                         alg5_values, alg6_values);
	}

	store_val(working_val, result_storage);
}

// -------------------------------------------------------------------
// Production kernel: tm_bruteforce
//
// Launch dimensions: global={32, chunk, 1}, local={32, 1, 1}
// One workgroup of 32 threads processes one input (code_index 0..31).
// Writes 1 byte per input to result_data: 0 = no hit, else flags|SENTINEL.
//
// Compiled twice from the same source:
//   - Without -DHASH_REDUCTION: full pipeline
//   - With    -DHASH_REDUCTION: inserts dummy hash reduction block at cut point
// -------------------------------------------------------------------
__kernel void tm_bruteforce(
	__global unsigned char*  result_data,
	__global unsigned char*  regular_rng_values,
	__global unsigned char*  alg0_values,
	__global unsigned char*  alg6_values,
	__global unsigned char*  rng_seed_forward_1,
	__global unsigned char*  rng_seed_forward_128,
	__global unsigned char*  alg2_values,
	__global unsigned char*  alg5_values,
	__global unsigned char*  expansion_values,
	__global unsigned char*  schedule_data,
	__global unsigned char*  carnival_data,
	unsigned int             key,
	unsigned int             data_start,
	int                      schedule_count)
{
	__local unsigned int working_code_storage[32];
	__local uchar alg_byte;
	__local unsigned int decrypted_carnival[32];
	__local unsigned int decrypted_other[32];

	unsigned int code_index  = get_local_id(0);
	unsigned int data_offset = get_global_id(1);

	// Expand: use key >> 16 as rng_seed to match CPU behaviour
	unsigned int expand_rng_seed = (key >> 16) & 0xFFFF;
	unsigned int working_val = get_init_value(key, data_start, code_index);
	working_val = expand_value(working_val, code_index, expand_rng_seed, expansion_values);

	barrier(CLK_LOCAL_MEM_FENCE);

	// Segment 1: maps [0..HASH_REDUCTION_CUT_POINT)
	for (int i = 0; i < 4; i++)
	{
		__global uchar* sched_ptr = schedule_data + i * 4;
		working_val = run_one_map(working_val, code_index, sched_ptr, working_code_storage, &alg_byte,
		                          rng_seed_forward_1, rng_seed_forward_128,
		                          regular_rng_values, alg0_values, alg2_values, alg5_values, alg6_values);
	}
	if ((data_offset % 100) < 40) {
		return;
	}
#ifdef HASH_REDUCTION
	barrier(CLK_LOCAL_MEM_FENCE);
	if (code_index == 0)
	{
		// TODO: hash reduction — compute hash of working_code here
		// and compare against seen-hash table; early-exit if seen.
		// For now: no-op (all inputs continue to full pipeline).
	}
	barrier(CLK_LOCAL_MEM_FENCE);
#endif

	// Segment 2: maps [HASH_REDUCTION_CUT_POINT..schedule_count)
	for (int i = HASH_REDUCTION_CUT_POINT; i < schedule_count; i++)
	{
		__global uchar* sched_ptr = schedule_data + i * 4;
		working_val = run_one_map(working_val, code_index, sched_ptr, working_code_storage, &alg_byte,
		                          rng_seed_forward_1, rng_seed_forward_128,
		                          regular_rng_values, alg0_values, alg2_values, alg5_values, alg6_values);
	}

	// Store final state, then decrypt both worlds cooperatively
	working_code_storage[code_index] = working_val;
	barrier(CLK_LOCAL_MEM_FENCE);
	
	decrypted_carnival[code_index] = working_code_storage[code_index]
	                               ^ ((__global unsigned int*)carnival_data)[code_index];
	decrypted_other[code_index]    = working_code_storage[code_index]
	                               ^ ((__constant unsigned int*)other_world_data_k)[code_index];

	barrier(CLK_LOCAL_MEM_FENCE);

	if (code_index == 0)
	{
		unsigned char stats = 0;

		if (checksum_ok(decrypted_carnival, 0x72))
		{
			stats = machine_code_flags(decrypted_carnival, 0x72,
			                           0, 0x2B, 0x33, 0x3E, 0xFF, 0xFF);
			stats |= CHECKSUM_SENTINEL;
		}
		else if (checksum_ok(decrypted_other, 0x53))
		{
			stats = machine_code_flags(decrypted_other, 0x53,
			                           0, 0x05, 0x0A, 0x28, 0x50, 0xFF);
			stats |= CHECKSUM_SENTINEL | OTHER_WORLD;
		}

		result_data[data_offset] = stats;
	}
}
