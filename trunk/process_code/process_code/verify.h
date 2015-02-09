#ifndef VERIFY_H
#define VERIFY_H
#include "data_sizes.h"

extern uint8 carnival_world_working_code[];
extern uint8 carnival_code_decrypted_machine_code[];

extern int carnival_code_length;
extern uint8 carnival_code[];

extern int other_world_code_length;
extern uint8 other_world_code[];

uint8 * decrypt_memory(uint8 * working_code, uint8 * memory, int length);
bool verify_checksum(uint8 * memory, int length);

bool compare_working_code(uint8 * block1, uint8 * block2);

int check_for_machine_code(uint8 * memory, int length);

#endif //VERIFY_H