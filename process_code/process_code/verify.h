#ifndef VERIFY_H
#define VERIFY_H
#include "data_sizes.h"
#include "working_code.h"

#define OP_JAM 0x01
#define OP_ILLEGAL 0x02
#define OP_NOP2 0x04
#define OP_NOP 0x08
#define OP_JUMP 0x10

extern uint8 carnival_world_working_code[];
extern uint8 carnival_code_decrypted_machine_code[];

extern int carnival_code_length;
extern uint8 carnival_code[];

extern int other_world_code_length;
extern uint8 other_world_code[];

extern uint8 opcode_bytes_used[];
extern uint8 opcode_type[];

uint8 * decrypt_memory(uint8 * working_code, uint8 * memory, int length);
bool verify_checksum(uint8 * memory, int length);

bool compare_working_code(uint8 * block1, uint8 * block2);


struct machine_code_properties {
	bool first_byte_is_valid;
	bool first_bytes_to_jump_valid;
	bool all_bytes_valid;

	bool uses_illegal_opcodes;
	bool uses_nop;
	bool uses_unofficial_nops;
	bool uses_jam;

	machine_code_properties() : first_byte_is_valid(false),
		first_bytes_to_jump_valid(false),
		all_bytes_valid(false),
		uses_illegal_opcodes(false),
		uses_nop(false),
		uses_unofficial_nops(false),
		uses_jam(false){}
};

machine_code_properties check_machine_code(uint8 * memory, int length);

void output_stats(working_code * in_progress);
void output_machine_code_stats(uint8 * decrypted_memory, int length);

#endif //VERIFY_H