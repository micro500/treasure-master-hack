#ifdef __INTELLISENSE__
#include "opencl_intellisense.h"
#endif

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

__constant unsigned char carnival_world_data_k[128] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x3D,0x5E,0xA1,0xA6,0xC8,0x23,
	0xD7,0x6E,0x3F,0x7C,0xD2,0x46,0x1B,0x9F,0xAB,0xD2,
	0x5C,0x9B,0x32,0x43,0x67,0x30,0xA0,0xA4,0x23,0xF3,
	0x27,0xBF,0xEA,0x21,0x0F,0x13,0x31,0x1A,0x15,0xA1,
	0x39,0x34,0xE4,0xD2,0x52,0x6E,0xA6,0xF7,0xF6,0x43,
	0xD1,0x28,0x41,0xD8,0xDC,0x55,0xE1,0xC5,0x49,0xF5,
	0xD4,0x84,0x52,0x1F,0x90,0xAB,0x26,0xE4,0x2A,0xC3,
	0xC2,0x59,0xAC,0x81,0x58,0x35,0x7A,0xC3,0x51,0x9A,
	0x01,0x04,0xF5,0xE2,0xFB,0xA7,0xAE,0x8B,0x46,0x9A,
	0x27,0x41,0xFA,0xDD,0x63,0x72,0x23,0x7E,0x1B,0x44,
	0x5A,0x0B,0x2A,0x3C,0x09,0xFA,0xA3,0x59,0x3C,0xA1,
	0xF0,0x90,0x4F,0x46,0x9E,0xD1,0xD7,0xF4
};

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
// RNG sequence table helpers
//
// rng_seq[pos] = the next seed value after one RNG step from the seed
//               at that position in the chain.
// rng_pos[seed] = absolute index into rng_seq where seed's chain starts.
//
// Output byte from a stored next-seed value:
//   output = ((next_seed >> 8) ^ next_seed) & 0xFF
//
// Seed advancement:
//   1-step  (alg2/5): rng_seq[rng_pos[seed] + 0]
//   128-step (others): rng_seq[rng_pos[seed] + 127]
// -------------------------------------------------------------------

// Compute the 4-byte RNG output pack for thread code_index.
// Each byte is the full output byte at the corresponding position.
// pos_base = rng_pos[current_rng_seed].
// The existing LUT layout stores position (127 - byte_offset) in reverse,
// so thread t reads positions pos_base+(127-t*4) down to pos_base+(124-t*4).
__attribute__((always_inline))
uint rng_full_bytes(uint pos_base, uint code_index, __global uchar* rng_out)
{
	uint p0 = pos_base + (127 - code_index * 4);
	uint p1 = pos_base + (126 - code_index * 4);
	uint p2 = pos_base + (125 - code_index * 4);
	uint p3 = pos_base + (124 - code_index * 4);
	return rng_out[p0] | ((uint)rng_out[p1] << 8) | ((uint)rng_out[p2] << 16) | ((uint)rng_out[p3] << 24);
}

// MSB-only version for alg0: each byte is 0 or 1 (bit 0).
__attribute__((always_inline))
uint rng_msb_lo_bytes(uint pos_base, uint code_index, __global uchar* rng_out)
{
	uint p0 = pos_base + (127 - code_index * 4);
	uint p1 = pos_base + (126 - code_index * 4);
	uint p2 = pos_base + (125 - code_index * 4);
	uint p3 = pos_base + (124 - code_index * 4);
	uint b0 = (rng_out[p0] >> 7) & 1;
	uint b1 = (rng_out[p1] >> 7) & 1;
	uint b2 = (rng_out[p2] >> 7) & 1;
	uint b3 = (rng_out[p3] >> 7) & 1;
	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

// MSB-in-bit7 version for alg6: each byte is 0 or 0x80.
__attribute__((always_inline))
uint rng_msb_hi_bytes(uint pos_base, uint code_index, __global uchar* rng_out)
{
	uint p0 = pos_base + (127 - code_index * 4);
	uint p1 = pos_base + (126 - code_index * 4);
	uint p2 = pos_base + (125 - code_index * 4);
	uint p3 = pos_base + (124 - code_index * 4);
	return (rng_out[p0] & 0x80) | ((uint)(rng_out[p1] & 0x80) << 8) | ((uint)(rng_out[p2] & 0x80) << 16) | ((uint)(rng_out[p3] & 0x80) << 24);
}

// -------------------------------------------------------------------
// Algorithm step
// Mirrors the original branchy alg(): algorithm_id is uniform across
// all threads (broadcast via alg_byte), so branches are not divergent.
// Only the needed RNG values are read; barrier only for alg2/5.
// -------------------------------------------------------------------
__attribute__((always_inline))
uint alg_seq(uint cur_val, uint algorithm_id, ushort* rng_seed,
             __global ushort* rng_seq, __global uint* rng_pos, __global uchar* rng_out)
{
	uint code_index = get_local_id(0);
	uint pos_base   = rng_pos[*rng_seed];

	if (algorithm_id == 0)
	{
		uint rng = rng_msb_lo_bytes(pos_base, code_index, rng_out);
		cur_val = ((cur_val << 1) & 0xFEFEFEFEu) | rng;
	}
	else if (algorithm_id == 1)
	{
		uint rng = rng_full_bytes(pos_base, code_index, rng_out);
		asm volatile("vadd.u32.u32.u32 %0, %1, %2;" : "=r"(cur_val) : "r"(cur_val), "r"(rng));
	}
	else if (algorithm_id == 3)
	{
		uint rng = rng_full_bytes(pos_base, code_index, rng_out);
		cur_val ^= rng;
	}
	else if (algorithm_id == 4)
	{
		uint rng = rng_full_bytes(pos_base, code_index, rng_out);
		asm volatile("vsub.u32.u32.u32 %0, %1, %2;" : "=r"(cur_val) : "r"(cur_val), "r"(rng));
	}
	else if (algorithm_id == 6)
	{
		uint rng = rng_msb_hi_bytes(pos_base, code_index, rng_out);
		cur_val = ((cur_val >> 1) & 0x7F7F7F7Fu) | rng;
	}
	else if (algorithm_id == 7)
	{
		cur_val ^= 0xFFFFFFFFu;
	}
	else if (algorithm_id == 2 || algorithm_id == 5)
	{
		uint neighbor;
		asm volatile("shfl.sync.down.b32 %0, %1, 1, 31, 0xffffffff;"
		             : "=r"(neighbor) : "r"(cur_val));
		neighbor &= -(uint)(code_index != 31u);
		uint one_step_seed = rng_seq[pos_base];
		uint carry_byte    = rng_out[pos_base];

		if (algorithm_id == 2)
		{
			uint carry = (code_index == 31)
			           ? (((carry_byte >> 7) & 1u) << 24)
			           : ((neighbor & 0x00000001u) << 24);
			cur_val = ((cur_val & 0x00010000u) >> 8) | carry
			        | ((cur_val >> 1) & 0x007F007Fu)
			        | ((cur_val << 1) & 0xFE00FE00u)
			        | ((cur_val >> 8) & 0x00800080u);
		}
		else if (algorithm_id == 5)
		{
			uint carry = (code_index == 31)
			           ? ((carry_byte & 0x80u) << 24)
			           : ((neighbor & 0x00000080u) << 24);
			cur_val = ((cur_val & 0x00800000u) >> 8) | carry
			        | ((cur_val >> 1) & 0x7F007F00u)
			        | ((cur_val << 1) & 0x00FE00FEu)
			        | ((cur_val >> 8) & 0x00010001u);
		}

		*rng_seed = (ushort)one_step_seed; // 1-step advance
		return cur_val;
	}

	// alg7 (NOT): no seed advance — seed is unchanged.
	if (algorithm_id == 7)
		return cur_val;

	*rng_seed = (ushort)rng_seq[pos_base + 127]; // 128-step advance (alg0/1/3/4/6)
	return cur_val;
}

// -------------------------------------------------------------------
// Map execution
// -------------------------------------------------------------------
__attribute__((always_inline))
uint run_one_map_seq(uint cur_val, uint code_index,
                     __constant uchar* schedule_data,
                     __global ushort* rng_seq, __global uint* rng_pos, __global uchar* rng_out)
{
	ushort rng_seed        = (*(schedule_data) << 8) | *(schedule_data + 1);
	ushort nibble_selector = (*(schedule_data + 2) << 8) | *(schedule_data + 3);

	for (int i = 0; i < 16; i++)
	{
		uint src_lane    = (uint)(i / 4);
		uint byte_shift  = (uint)(i & 3) * 8u;
		uint my_byte     = (cur_val >> byte_shift) & 0xFFu;
		uint bcast_byte;
		asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, 0xffffffff;"
		             : "=r"(bcast_byte) : "r"(my_byte), "r"(src_lane));
		uchar current_byte = (uchar)bcast_byte;

		uchar nibble = (nibble_selector >> 15) & 0x01;
		nibble_selector = nibble_selector << 1;

		if (nibble == 1)
			current_byte = current_byte >> 4;

		uint algorithm_id = (current_byte >> 1) & 0x07;

		cur_val = alg_seq(cur_val, algorithm_id, &rng_seed,
		                  rng_seq, rng_pos, rng_out);
	}

	return cur_val;
}

// -------------------------------------------------------------------
// Expansion: computed inline from rng_seq_table.
//
// Byte at position b = (code_index*4 + byte_idx) has value:
//   sum of RNG outputs at sequence steps k, k+8, k+16, ..., k+(j-1)*8
//   where j = b/8, k = b%8  (0-indexed steps into rng_seq from pos_base)
// First 16 bytes (j=0) are always 0.
// -------------------------------------------------------------------
__attribute__((always_inline))
uint expand_inline(uint cur_val, uint pos_base, uint code_index,
                   __global ushort* rng_seq)
{
	uint expansion_val = 0;

	for (int byte_idx = 0; byte_idx < 4; byte_idx++)
	{
		uint b = code_index * 4 + (uint)byte_idx;
		uint j = b / 8;
		uint k = b % 8;
		uint accum = 0;
		for (uint i = 0; i < j; i++)
		{
			uint step = k + i * 8;
			uint sv   = rng_seq[pos_base + step];
			accum    += ((sv >> 8) ^ sv) & 0xFF;
		}
		expansion_val |= ((accum & 0xFF) << ((uint)byte_idx * 8));
	}

	uchar4* working_uchar4   = (uchar4*)&cur_val;
	uchar4* expansion_uchar4 = (uchar4*)&expansion_val;
	*working_uchar4 += *expansion_uchar4;

	return cur_val;
}

// -------------------------------------------------------------------
// Checksum and machine code validation (thread 0 only)
// -------------------------------------------------------------------
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

__attribute__((always_inline))
unsigned char machine_code_flags(__local unsigned int* data_i, int code_length,
                                  unsigned char e0, unsigned char e1, unsigned char e2,
                                  unsigned char e3, unsigned char e4, unsigned char e5)
{
	__local unsigned char* data = (__local unsigned char*)data_i;
	unsigned char entry_addrs[6]  = { e0, e1, e2, e3, e4, e5 };
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
			hit_entries[last_entry]   = 1;
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

	if (all_entries_valid)     result |= ALL_ENTRIES_VALID;
	if (valid_entries[0] == 1) result |= FIRST_ENTRY_VALID;

	return result;
}

// -------------------------------------------------------------------
// Production kernel: tm_bruteforce_seq
//
// Launch dimensions: global={32, num_wg, 1}, local={32, 1, 1}
// Each workgroup of 32 threads processes CANDIDATES_PER_WG=64 candidates
// sequentially, then writes results as a single coalesced 128-byte write
// (32 threads * 4 bytes each = 128 bytes = 64 candidates * 2 bytes).
//
// Result format: 2 bytes per candidate.
//   byte 0: flags | CHECKSUM_SENTINEL if hit, else 0
//   byte 1: 0 (reserved)
// -------------------------------------------------------------------
__attribute__((reqd_work_group_size(32, 1, 1)))
__kernel void tm_bruteforce_seq(
	__global unsigned char* result_data,
	__global ushort*        rng_seq,
	__global uint*          rng_pos,
	__constant unsigned char* schedule_data,
	unsigned int            key,
	unsigned int            data_start,
	int                     schedule_count,
	unsigned int            chunk,
	__constant uint*        expansion_vals,
	__global uchar*         rng_out)
{
	__local uint  working_code_storage[32];
	__local uint  decrypted_carnival[32];
	__local uint  decrypted_other[32];
	__local uchar local_results[128];    // 64 candidates * 2 bytes

	unsigned int code_index = get_local_id(0);
	unsigned int group_id   = get_global_id(1);
	unsigned int data_base  = group_id * 64;

	uint my_expansion = expansion_vals[code_index];

	for (int local_cand = 0; local_cand < 64; local_cand++)
	{
		unsigned int data_idx = data_base + (unsigned int)local_cand;
		unsigned int data     = data_start + data_idx;

		unsigned int working_val;
		if (data_idx < chunk)
		{
			working_val = (code_index % 2 == 0) ? key : data;
			*((uchar4*)&working_val) += *((uchar4*)&my_expansion);
		}
		else
		{
			// Out-of-bounds candidate: write 0 result and skip
			if (code_index == 0)
			{
				local_results[local_cand * 2]     = 0;
				local_results[local_cand * 2 + 1] = 0;
			}
			continue;
		}

		for (int i = 0; i < schedule_count; i++)
		{
			__constant uchar* sched_ptr = schedule_data + i * 4;
			working_val = run_one_map_seq(working_val, code_index, sched_ptr,
			                             rng_seq, rng_pos, rng_out);
		}

		// Store final state for decryption
		working_code_storage[code_index] = working_val;
		barrier(CLK_LOCAL_MEM_FENCE);
		/*
		decrypted_carnival[code_index] = working_code_storage[code_index]
		                               ^ ((__constant uint*)carnival_world_data_k)[code_index];
		decrypted_other[code_index]    = working_code_storage[code_index]
		                               ^ ((__constant uint*)other_world_data_k)[code_index];
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

			local_results[local_cand * 2]     = stats;
			local_results[local_cand * 2 + 1] = 0;
		}
		// No barrier here: only thread 0 writes local_results,
		// and nothing reads it until the write-out after the loop.
		*/
	}

	// One barrier to make all of thread 0's writes to local_results
	// visible to all 32 threads before the coalesced write-out.
	barrier(CLK_LOCAL_MEM_FENCE);

	// Coalesced 128-byte write: 32 threads each write 4 bytes.
	uint write_base = group_id * 32;
	((__global uint*)result_data)[write_base + code_index] =
	    ((__local uint*)local_results)[code_index];
}
