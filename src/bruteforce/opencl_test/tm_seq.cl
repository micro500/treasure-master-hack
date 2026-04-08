#ifdef __INTELLISENSE__
#include "opencl_intellisense.h"
#endif

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

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

// Hash reduction constants
//
// Hash slot layout (4 x uint at indices slot*4+0, +1, +2, +3):
//   +0: H2 (first  32-bit validation hash)
//   +1: writer_data (canonical data value, written after claim succeeds)
//   +2: status:
//         0               = empty
//         HASH_IN_PROGRESS = claimed by writer, writer_data not yet valid
//         HASH_CLAIMED     = claimed, writer_data valid, phase 2 in progress
//         HASH_DONE_BIT|.. = done: bits[15:8]=other_stats, bits[7:0]=carnival_stats
//   +3: H3 (second 32-bit validation hash, independent seed — combined 64-bit check)
//
// Deferred list (separate buffer, one entry per IN_PROGRESS miss):
//   deferred_pairs[i*4+0] = reader's data value
//   deferred_pairs[i*4+1] = writer's data value (look up in CPU result list)
//   deferred_pairs[i*4+2] = h2
//   deferred_pairs[i*4+3] = h3
//
// Byte-swap a uint32: maps CPU byte 0 (MSB) to bits 7-0 (LE convention)
#define bswap32(x) \
    (  ((x) & 0xFFu)         << 24  \
     | (((x) >> 8) & 0xFFu)  << 16  \
     | (((x) >> 16) & 0xFFu) <<  8  \
     | (((x) >> 24) & 0xFFu)        )

#define HASH_SPLIT_MAP    5
#define HASH_MAX_PROBE    8
#define HASH_IN_PROGRESS  0xFFFFFFFFu
#define HASH_CLAIMED      0x40000000u
#define HASH_DONE_BIT     0x80000000u
// Must match BATCH_SIZE in tm_opencl_seq.h
#define MAX_DEFERRED      (1u << 20)

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
// Per-schedule RNG table approach.
//
// map_rng[map_idx * 2048 + i] = pre-computed output byte at step i
//   from the map's initial RNG seed.
//
// local_pos tracks absolute offset within the 2048-entry window.
//   Advances +128 for alg0/1/3/4/6, +1 for alg2/5, +0 for alg7.
//
// For 128-step algs, thread code_index reads 4 bytes at offsets:
//   map_base + local_pos + (127 - code_index*4) down to
//   map_base + local_pos + (124 - code_index*4)
// For 1-step algs (alg2/5), carry byte is at map_base + local_pos.
// -------------------------------------------------------------------

// -------------------------------------------------------------------
// Algorithm step
// algorithm_id is uniform across all threads — no divergence.
// -------------------------------------------------------------------
__attribute__((always_inline))
uint alg_seq(uint cur_val, uint algorithm_id, uint* local_pos,
             uint map_base, __global uchar* map_rng)
{
	uint code_index = get_local_id(0);
	uint base       = map_base + *local_pos;

	switch (algorithm_id)
	{
		case 0:
		{
			uint p0 = base + (127 - code_index * 4);
			uint p1 = base + (126 - code_index * 4);
			uint p2 = base + (125 - code_index * 4);
			uint p3 = base + (124 - code_index * 4);
			uint b0 = (map_rng[p0] >> 7) & 1;
			uint b1 = (map_rng[p1] >> 7) & 1;
			uint b2 = (map_rng[p2] >> 7) & 1;
			uint b3 = (map_rng[p3] >> 7) & 1;
			uint rng = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
			cur_val = ((cur_val << 1) & 0xFEFEFEFEu) | rng;
			*local_pos += 128;
			break;
		}
		case 1:
		{
			uint p0 = base + (127 - code_index * 4);
			uint p1 = base + (126 - code_index * 4);
			uint p2 = base + (125 - code_index * 4);
			uint p3 = base + (124 - code_index * 4);
			uint rng = map_rng[p0] | ((uint)map_rng[p1] << 8)
			         | ((uint)map_rng[p2] << 16) | ((uint)map_rng[p3] << 24);
			*((uchar4*)&cur_val) += *((uchar4*)&rng);
			*local_pos += 128;
			break;
		}
		case 2:
		case 5:
		{
			uint neighbor;
			asm volatile("shfl.sync.down.b32 %0, %1, 1, 31, 0xffffffff;"
			             : "=r"(neighbor) : "r"(cur_val));
			neighbor &= -(uint)(code_index != 31u);
			uint carry_byte = map_rng[map_base + *local_pos];

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
			else
			{
				uint carry = (code_index == 31)
				           ? ((carry_byte & 0x80u) << 24)
				           : ((neighbor & 0x00000080u) << 24);
				cur_val = ((cur_val & 0x00800000u) >> 8) | carry
				        | ((cur_val >> 1) & 0x7F007F00u)
				        | ((cur_val << 1) & 0x00FE00FEu)
				        | ((cur_val >> 8) & 0x00010001u);
			}

			*local_pos += 1;
			break;
		}
		case 3:
		{
			uint p0 = base + (127 - code_index * 4);
			uint p1 = base + (126 - code_index * 4);
			uint p2 = base + (125 - code_index * 4);
			uint p3 = base + (124 - code_index * 4);
			uint rng = map_rng[p0] | ((uint)map_rng[p1] << 8)
			         | ((uint)map_rng[p2] << 16) | ((uint)map_rng[p3] << 24);
			cur_val ^= rng;
			*local_pos += 128;
			break;
		}
		case 4:
		{
			uint p0 = base + (127 - code_index * 4);
			uint p1 = base + (126 - code_index * 4);
			uint p2 = base + (125 - code_index * 4);
			uint p3 = base + (124 - code_index * 4);
			uint rng = map_rng[p0] | ((uint)map_rng[p1] << 8)
			         | ((uint)map_rng[p2] << 16) | ((uint)map_rng[p3] << 24);
			*((uchar4*)&cur_val) -= *((uchar4*)&rng);
			*local_pos += 128;
			break;
		}
		case 6:
		{
			// alg6 uses forward RNG indexing (CPU stores step j at position j, not reversed)
			uint p0 = base + (code_index * 4);
			uint p1 = base + (code_index * 4 + 1);
			uint p2 = base + (code_index * 4 + 2);
			uint p3 = base + (code_index * 4 + 3);
			uint rng = (map_rng[p0] & 0x80) | ((uint)(map_rng[p1] & 0x80) << 8)
			         | ((uint)(map_rng[p2] & 0x80) << 16) | ((uint)(map_rng[p3] & 0x80) << 24);
			cur_val = ((cur_val >> 1) & 0x7F7F7F7Fu) | rng;
			*local_pos += 128;
			break;
		}
		case 7:
		{
			cur_val ^= 0xFFFFFFFFu;
			break;
		}
	}

	return cur_val;
}

// -------------------------------------------------------------------
// Map execution
// -------------------------------------------------------------------
__attribute__((always_inline))
uint run_one_map_seq(uint cur_val, uint code_index, uint map_idx,
                     __constant ushort* nibble_sel, __global uchar* map_rng,
                     __local uint* lane_buf)
{
	uint local_pos     = 0;
	uint map_base      = map_idx * 2048;
	ushort nibble_selector = nibble_sel[map_idx];

	for (int i = 0; i < 16; i++)
	{
		uint src_lane   = (uint)(i / 4);
		uint byte_shift = (uint)(i & 3) * 8u;

		lane_buf[code_index] = cur_val;
		barrier(CLK_LOCAL_MEM_FENCE);

		uchar current_byte = (uchar)((lane_buf[src_lane] >> byte_shift) & 0xFFu);

		uchar nibble = (nibble_selector >> 15) & 0x01;
		nibble_selector = nibble_selector << 1;

		if (nibble == 1)
			current_byte = current_byte >> 4;

		uint algorithm_id = (current_byte >> 1) & 0x07;

		cur_val = alg_seq(cur_val, algorithm_id, &local_pos, map_base, map_rng);
	}

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
// Each workgroup processes 64 candidates sequentially, writing results as
// a single coalesced 128-byte write (32 threads * 4 bytes).
//
// Hash reduction: after HASH_SPLIT_MAP maps, hash the 128-byte state and
// look up in a global hash table.
//   - DONE match   → use cached result directly, skip phase 2
//   - IN_PROGRESS  → tiny spin until writer_data is committed (CLAIMED state),
//                    then record {reader_data, writer_data} in the deferred list
//                    and skip phase 2; CPU resolves after kernel completes
//   - Empty        → claim slot, run phase 2, commit result
//
// Result format: 2 bytes per candidate.
//   byte 0: carnival_flags | CHECKSUM_SENTINEL if carnival hit, else 0
//   byte 1: other_flags | CHECKSUM_SENTINEL | OTHER_WORLD if other hit, else 0
// -------------------------------------------------------------------
__attribute__((reqd_work_group_size(32, 1, 1)))
__kernel void tm_bruteforce_seq(
	__global unsigned char*  result_data,     // arg 0
	__global uchar*          map_rng,         // arg 1: schedule_count * 2048 bytes
	__constant ushort*       nibble_sel,      // arg 2: schedule_count ushorts
	unsigned int             key,             // arg 3
	unsigned int             data_start,      // arg 4
	int                      schedule_count,  // arg 5
	unsigned int             chunk,           // arg 6
	__constant uint*         expansion_vals,  // arg 7
	__global volatile uint*  hash_table,      // arg 8: hash_table_size*4 uints, zeroed before run
	uint                     hash_table_mask, // arg 9: hash_table_size - 1
	__global uint*           deferred_pairs,  // arg 10: MAX_DEFERRED*4 uints {reader_data, writer_data, h2, reader_lane0}
	__global volatile uint*  deferred_count)  // arg 11: atomic counter, zeroed before run
{
	__local uint  decrypted_carnival[32];
	__local uint  hash_scratch[32];
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
			working_val = (code_index % 2 == 0) ? bswap32(key) : bswap32(data);
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

		// Phase 1: maps 0..HASH_SPLIT_MAP-1 (or all maps if schedule_count <= HASH_SPLIT_MAP)
		int phase1_end = min(schedule_count, HASH_SPLIT_MAP);
		for (int i = 0; i < phase1_end; i++)
			working_val = run_one_map_seq(working_val, code_index, (uint)i, nibble_sel, map_rng, hash_scratch);

		// Hash lookup / claim (only when maps remain after the split point)
		uint h1 = 0, h2 = 0, h3 = 0;
		int claimed_slot = -1;
		// cache_hit values: 0=miss (run phase 2), 1=direct hit, 2=deferred (skip phase 2)
		int cache_hit = 0;
		unsigned char cached_carnival = 0;
		unsigned char cached_other = 0;

		if (schedule_count > HASH_SPLIT_MAP)
		{
			// All 32 threads write their working_val into shared scratch, then
			// a barrier (directly in kernel body) ensures all writes are visible.
			// Thread 0 reads all 32 values and computes three independent FNV-1a-style
			// hashes over the full 128-byte post-phase-1 state.
			hash_scratch[code_index] = working_val;
			barrier(CLK_LOCAL_MEM_FENCE);

			if (code_index == 0)
			{
				uint acc1 = 2166136261u;
				uint acc2 = 2166136261u ^ 0xDEADBEEFu;
				uint acc3 = 2166136261u ^ 0xCAFEBABEu;
				for (uint i = 0u; i < 32u; i++)
				{
					uint contrib = hash_scratch[i] * (i * 2654435761u + 1u);
					acc1 = (acc1 ^ contrib) * 16777619u;
					acc2 = (acc2 ^ contrib) * 16777619u;
					acc3 = (acc3 ^ contrib) * 16777619u;
				}
				h1 = acc1;
				h2 = acc2;
				h3 = acc3;
			}

			if (code_index == 0)
			{
				uint slot = h1 & hash_table_mask;
				for (int probe = 0; probe < HASH_MAX_PROBE; probe++)
				{
					// Always attempt to claim the slot first.  The CAS return value
					// tells us the slot's status atomically — no separate pre-read.
					uint old = atom_cmpxchg(
						(volatile __global uint*)&hash_table[slot * 4 + 2],
						0u, HASH_IN_PROGRESS);

					if (old == 0u)
					{
						// Won the CAS — we are the writer for this slot.
						hash_table[slot * 4]     = h2;
						hash_table[slot * 4 + 1] = data;
						hash_table[slot * 4 + 3] = h3;
						asm volatile("membar.gl;");
						hash_table[slot * 4 + 2] = HASH_CLAIMED;
						claimed_slot = (int)slot;
						break;
					}

					// Lost the CAS — slot was already occupied.  `old` is the status.
					uint s = old;

					// If the writer hasn't finished committing yet, spin until it does.
					while (s == HASH_IN_PROGRESS)
						s = hash_table[slot * 4 + 2];
					asm volatile("membar.gl;");

					// Now s is either CLAIMED or has HASH_DONE_BIT set.
					// Validate h2+h3 to confirm a true state collision.
					if (hash_table[slot * 4] == h2 && hash_table[slot * 4 + 3] == h3)
					{
						if ((s & HASH_DONE_BIT) != 0u)
						{
							// Writer finished — result is already in the slot.
							cached_carnival = (uchar)(s & 0xFFu);
							cached_other    = (uchar)((s >> 8) & 0xFFu);
							cache_hit = 1;
						}
						else
						{
							// Writer still in phase 2 — defer resolution to CPU.
							uint writer_data = hash_table[slot * 4 + 1];
							uint di = atom_add(
								(volatile __global uint*)deferred_count, 1u);
							if (di < MAX_DEFERRED)
							{
								deferred_pairs[di * 4]     = data;
								deferred_pairs[di * 4 + 1] = writer_data;
								deferred_pairs[di * 4 + 2] = h2;
								deferred_pairs[di * 4 + 3] = h3;
							}
							cache_hit = 2;
						}
						break;
					}
					// H2/H3 mismatch — hash collision, probe next slot.

					slot = (slot + 1u) & hash_table_mask;
				}
			}

			// Broadcast cache_hit from thread 0 to all threads in the warp
			int cache_hit_bcast;
			asm volatile("shfl.sync.idx.b32 %0, %1, 0, 31, 0xffffffff;"
			             : "=r"(cache_hit_bcast) : "r"(cache_hit));
			if (cache_hit_bcast == 1)
			{
				// Direct hit: write cached result and skip phase 2
				if (code_index == 0)
				{
					local_results[local_cand * 2]     = cached_carnival;
					local_results[local_cand * 2 + 1] = cached_other;
				}
				continue;
			}
			else if (cache_hit_bcast == 2)
			{
				// Deferred: pair recorded in deferred list; no direct result here
				if (code_index == 0)
				{
					local_results[local_cand * 2]     = 0;
					local_results[local_cand * 2 + 1] = 0;
				}
				continue;
			}
		}

		// Phase 2: maps HASH_SPLIT_MAP..schedule_count-1
		// (empty loop when schedule_count <= HASH_SPLIT_MAP)
		for (int i = phase1_end; i < schedule_count; i++)
			working_val = run_one_map_seq(working_val, code_index, (uint)i, nibble_sel, map_rng, hash_scratch);

		decrypted_carnival[code_index] = working_val
		                               ^ ((__constant uint*)carnival_world_data_k)[code_index];
		decrypted_other[code_index]    = working_val
		                               ^ ((__constant uint*)other_world_data_k)[code_index];
		barrier(CLK_LOCAL_MEM_FENCE);

		if (code_index == 0)
		{
			unsigned char carnival_stats = 0;
			unsigned char other_stats = 0;

			if (checksum_ok(decrypted_carnival, 0x72))
			{
				carnival_stats = machine_code_flags(decrypted_carnival, 0x72,
				                                    0, 0x2B, 0x33, 0x3E, 0xFF, 0xFF);
				carnival_stats |= CHECKSUM_SENTINEL;
			}
			if (checksum_ok(decrypted_other, 0x53))
			{
				other_stats = machine_code_flags(decrypted_other, 0x53,
				                                 0, 0x05, 0x0A, 0x28, 0x50, 0xFF);
				other_stats |= CHECKSUM_SENTINEL | OTHER_WORLD;
			}

			local_results[local_cand * 2]     = carnival_stats;
			local_results[local_cand * 2 + 1] = other_stats;

			// Commit result to the claimed slot
			if (claimed_slot >= 0)
			{
				asm volatile("membar.gl;"); // release: all prior writes visible before done marker
				hash_table[claimed_slot * 4 + 2] = HASH_DONE_BIT
				                                 | ((uint)other_stats << 8)
				                                 | (uint)carnival_stats;
			}
		}
		// No barrier here: only thread 0 writes local_results,
		// and nothing reads it until the write-out after the loop.
	}

	// One barrier to make all of thread 0's writes to local_results
	// visible to all 32 threads before the coalesced write-out.
	barrier(CLK_LOCAL_MEM_FENCE);

	// Coalesced 128-byte write: 32 threads each write 4 bytes.
	uint write_base = group_id * 32;
	((__global uint*)result_data)[write_base + code_index] =
	    ((__local uint*)local_results)[code_index];
}

// -------------------------------------------------------------------
// Test kernels — exercise alg_seq / run_one_map_seq / expand using
// the same code as the production kernel, with CPU-byte-order I/O.
//
// alg_seq uses little-endian convention: CPU byte N of a 4-byte group
// lives at bits (N%4)*8 of cur_val — i.e. CPU byte 0 at bits 7-0.
//
// I/O boundary: load CPU byte 0 → bits 7-0; write bits 7-0 → CPU byte 0.
//
// Expand/pipeline init: key/data are passed as uint32 in host byte order
// (CPU byte 0 = key>>24 at the MSB). Byte-swap before use so CPU byte 0
// lands at bits 7-0 as alg_seq expects.
// -------------------------------------------------------------------

__attribute__((reqd_work_group_size(32, 1, 1)))
__kernel void tm_test_expand(
    unsigned int key, unsigned int data,
    __constant uint* expansion_vals, __global uchar* output)
{
    uint code_index = get_local_id(0);
    uint my_expansion = expansion_vals[code_index];
    uint working_val = (code_index % 2 == 0) ? bswap32(key) : bswap32(data);
    *((uchar4*)&working_val) += *((uchar4*)&my_expansion);
    uint base = code_index * 4;
    output[base + 0] =  working_val        & 0xFFu;
    output[base + 1] = (working_val >>  8) & 0xFFu;
    output[base + 2] = (working_val >> 16) & 0xFFu;
    output[base + 3] = (working_val >> 24) & 0xFFu;
}

__attribute__((reqd_work_group_size(32, 1, 1)))
__kernel void tm_test_alg(
    __global const uchar* input, uint alg_id,
    __global uchar* map_rng, __global uchar* output)
{
    uint code_index = get_local_id(0);
    uint base = code_index * 4;
    // Load CPU bytes into LE convention: CPU byte 0 → bits 7-0
    uint cur_val =  (uint)input[base+0]
                 | ((uint)input[base+1] <<  8)
                 | ((uint)input[base+2] << 16)
                 | ((uint)input[base+3] << 24);
    uint local_pos = 0;
    cur_val = alg_seq(cur_val, alg_id, &local_pos, 0, map_rng);
    // Write back: bits 7-0 → CPU byte 0
    output[base + 0] =  cur_val        & 0xFFu;
    output[base + 1] = (cur_val >>  8) & 0xFFu;
    output[base + 2] = (cur_val >> 16) & 0xFFu;
    output[base + 3] = (cur_val >> 24) & 0xFFu;
}

__attribute__((reqd_work_group_size(32, 1, 1)))
__kernel void tm_test_pipeline(
    unsigned int key, unsigned int data,
    __constant uint* expansion_vals, __global uchar* map_rng,
    __constant ushort* nibble_sel, int schedule_count,
    __global uchar* output)
{
    __local uint lane_buf[32];
    uint code_index = get_local_id(0);
    uint my_expansion = expansion_vals[code_index];
    uint working_val = (code_index % 2 == 0) ? bswap32(key) : bswap32(data);
    *((uchar4*)&working_val) += *((uchar4*)&my_expansion);
    for (int i = 0; i < schedule_count; i++)
        working_val = run_one_map_seq(working_val, code_index, (uint)i,
                                      nibble_sel, map_rng, lane_buf);
    uint base = code_index * 4;
    output[base + 0] =  working_val        & 0xFFu;
    output[base + 1] = (working_val >>  8) & 0xFFu;
    output[base + 2] = (working_val >> 16) & 0xFFu;
    output[base + 3] = (working_val >> 24) & 0xFFu;
}
