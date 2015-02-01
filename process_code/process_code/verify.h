#ifndef VERIFY_H
#define VERIFY_H
#include "data_sizes.h"

extern int carnival_code_length;
extern uint8 carnival_code[];

extern int other_world_code_length;
extern uint8 other_world_code[];

uint8 * decrypt_memory(uint8 * working_code, uint8 * memory, int length);
bool verify_checksum(uint8 * memory, int length);

int check_for_machine_code(uint8 * memory, int length);

#endif //VERIFY_H