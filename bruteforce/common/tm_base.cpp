#include <stdio.h>
#include <iostream>
#include "data_sizes.h"
#include "tm_base.h"

TM_base::TM_base(RNG * rng_obj) : rng(rng_obj)
{
}

void TM_base::print_working_code()
{
	uint8 data[128];
	fetch_data(data);
	for (int i = 0; i < 128; i++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");
}

void TM_base::run_bruteforce_data(uint32 key, uint32 data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size) {}

uint8 TM_base::check_machine_code(uint8* data, int world)
{
	uint8 code_length;
	uint8 entry_addrs[6];
	if (world == CARNIVAL_WORLD)
	{
		code_length = CARNIVAL_WORLD_CODE_LENGTH;

		entry_addrs[0] = 0;
		entry_addrs[1] = 0x2B;
		entry_addrs[2] = 0x33;
		entry_addrs[3] = 0x3E;
		entry_addrs[4] = 0xFF;
		entry_addrs[5] = 0xFF;
	}
	else
	{
		code_length = OTHER_WORLD_CODE_LENGTH;

		entry_addrs[0] = 0;
		entry_addrs[1] = 0x05;
		entry_addrs[2] = 0x0A;
		entry_addrs[3] = 0x28;
		entry_addrs[4] = 0x50;
		entry_addrs[5] = 0xFF;
	}

	uint8 active_entries[6] = { 0,0,0,0,0,0 };
	uint8 hit_entries[6] = { 0,0,0,0,0,0 };
	uint8 valid_entries[6] = { 0,0,0,0,0,0 };
	int last_entry = -1;

	uint8 result = 0;
	uint8 next_entry_addr = entry_addrs[0];

	for (int i = 0; i < code_length - 2; i++)
	{
		if (i == next_entry_addr)
		{
			last_entry++;
			hit_entries[last_entry] = 1;
			active_entries[last_entry] = 1;
			next_entry_addr = entry_addrs[last_entry + 1];
		}
		else if (i > next_entry_addr)
		{
			last_entry++;
			next_entry_addr = entry_addrs[last_entry + 1];
		}

		uint8 opcode = data[reverse_offset(i)];

		// illegal/jam: cancel all active, do not mark valid
		// jump: mark all valid, then cancel all active
		if (opcode_type[opcode] & OP_JAM)
		{
			result |= USES_JAM;
			break;
		}
		else if (opcode_type[opcode] & OP_ILLEGAL)
		{
			result |= USES_ILLEGAL_OPCODES;
			break;
		}
		else if (opcode_type[opcode] & OP_NOP2)
		{
			result |= USES_UNOFFICIAL_NOPS;
		}
		else if (opcode_type[opcode] & OP_NOP)
		{
			result |= USES_NOP;
		}
		else if (opcode_type[opcode] & OP_JUMP)
		{
			for (int j = 0; j < 5; j++)
			{
				if (active_entries[j] == 1)
				{
					active_entries[j] = 0;
					valid_entries[j] = 1;
				}
			}
		}

		i += opcode_bytes_used[opcode] - 1;
	}

	bool all_entries_valid = true;

	for (int i = 0; i < 5; i++)
	{
		if (hit_entries[i] == 1)
		{
			if (valid_entries[i] == 1)
			{
				continue;
			}
			else
			{
				all_entries_valid = false;
				break;
			}
		}
		else if (entry_addrs[i] == 255)
		{
			continue;
		}
		else
		{
			for (int j = entry_addrs[i]; j < code_length - 2; j++)
			{
				uint8 opcode = data[reverse_offset(j)];
				if ((opcode_type[opcode] & OP_JAM) || (opcode_type[opcode] & OP_ILLEGAL))
				{
					all_entries_valid = false;
					break;
				}
				else if (opcode_type[opcode] & OP_JUMP)
				{
					break;
				}

				j += opcode_bytes_used[opcode] - 1;
			}
		}
	}

	if (all_entries_valid)
	{
		result |= ALL_ENTRIES_VALID;
	}

	if (valid_entries[0] == 1)
	{
		result |= FIRST_ENTRY_VALID;
	}

	return result;
}

void TM_base::shuffle_mem(uint8* src, uint8* dest, int bits, bool packing_16)
{
	for (int i = 0; i < 128; i++)
	{
		packing_store(dest, shuffle_8(i, bits), src[i], packing_16);
	}
}

void TM_base::unshuffle_mem(uint8* src, uint8* dest, int bits, bool packing_16)
{
	for (int i = 0; i < 128; i++)
	{
		dest[i] = packing_load(src, shuffle_8(i, bits), packing_16);
	}
}

void TM_base::decrypt_carnival_world() {}
uint16 TM_base::calculate_carnival_world_checksum() { return 0; }
uint16 TM_base::fetch_carnival_world_checksum_value() { return 0; }

void TM_base::decrypt_other_world() {}
uint16 TM_base::calculate_other_world_checksum() { return 0; }
uint16 TM_base::fetch_other_world_checksum_value() { return 0; }

uint8 TM_base::opcode_bytes_used[0x100] = { 1,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
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
											2,2,0,0,2,2,2,0,1,3,1,0,2,3,3,0 };

uint8 TM_base::opcode_type[0x100] = { 0, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_JUMP, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_JUMP, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, OP_NOP2, 0, OP_NOP2, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, OP_NOP2, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_ILLEGAL, 0, OP_ILLEGAL, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP, OP_ILLEGAL, 0, 0, 0, OP_ILLEGAL, OP_JUMP, 0, OP_JAM, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL, 0, 0, OP_NOP2, OP_ILLEGAL, OP_NOP2, 0, 0, OP_ILLEGAL };



ALIGNED(64) uint8 TM_base::carnival_world_data[128] = 
{   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x3D, 0x5E, 0xA1, 0xA6, 0xC8, 0x23,
	0xD7, 0x6E, 0x3F, 0x7C, 0xD2, 0x46, 0x1B, 0x9F, 0xAB, 0xD2,
	0x5C, 0x9B, 0x32, 0x43, 0x67, 0x30, 0xA0, 0xA4, 0x23, 0xF3,
	0x27, 0xBF, 0xEA, 0x21, 0x0F, 0x13, 0x31, 0x1A, 0x15, 0xA1,
	0x39, 0x34, 0xE4, 0xD2, 0x52, 0x6E, 0xA6, 0xF7, 0xF6, 0x43,
	0xD1, 0x28, 0x41, 0xD8, 0xDC, 0x55, 0xE1, 0xC5, 0x49, 0xF5,
	0xD4, 0x84, 0x52, 0x1F, 0x90, 0xAB, 0x26, 0xE4, 0x2A, 0xC3,
	0xC2, 0x59, 0xAC, 0x81, 0x58, 0x35, 0x7A, 0xC3, 0x51, 0x9A,
	0x01, 0x04, 0xF5, 0xE2, 0xFB, 0xA7, 0xAE, 0x8B, 0x46, 0x9A,
	0x27, 0x41, 0xFA, 0xDD, 0x63, 0x72, 0x23, 0x7E, 0x1B, 0x44,
	0x5A, 0x0B, 0x2A, 0x3C, 0x09, 0xFA, 0xA3, 0x59, 0x3C, 0xA1,
	0xF0, 0x90, 0x4F, 0x46, 0x9E, 0xD1, 0xD7, 0xF4 };

ALIGNED(64) uint8 TM_base::carnival_world_checksum_mask[128] =
{   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

ALIGNED(64) uint8 TM_base::other_world_data[128] = 
{   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xCA, 0x68, 0xC1, 0x66, 0x44,
	0xD2, 0x04, 0x0B, 0x90, 0x81, 0x86, 0xC7, 0xF4, 0xD2, 0xE2,
	0xF1, 0x22, 0xE3, 0x0C, 0xD9, 0x54, 0xFB, 0xFF, 0x0A, 0xCF,
	0x81, 0x72, 0x0A, 0x94, 0x9A, 0x98, 0xD3, 0xFF, 0xAB, 0x80,
	0x9A, 0xE5, 0xB7, 0x45, 0x6E, 0x8F, 0xD2, 0xF0, 0x67, 0xFF,
	0xB3, 0xAE, 0x49, 0xBB, 0x9C, 0x06, 0x12, 0x40, 0x49, 0xA3,
	0x9A, 0xDB, 0x32, 0x7B, 0x58, 0xA1, 0x5A, 0xB9, 0x2B, 0x2B,
	0x2D, 0x6E, 0x36, 0x93, 0x1C, 0x1A, 0x52, 0x03, 0x18, 0xE4,
	0x5E, 0xB1, 0xC1, 0xBD, 0x44, 0xFB, 0xF1, 0x50 };

ALIGNED(64) uint8 TM_base::other_world_checksum_mask[128] = 
{   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };