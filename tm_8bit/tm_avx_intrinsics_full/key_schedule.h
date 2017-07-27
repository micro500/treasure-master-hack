#ifndef KEY_SCHEDULE_H
#define KEY_SCHEDULE_H
#include "data_sizes.h"

typedef struct 
{
	uint8 rng1;
	uint8 rng2;
	uint16 nibble_selector;
} key_schedule_entry;

typedef union
{
	uint8 as_uint8[4];
	uint16 as_uint16[2];
	uint32 as_uint32;
} key_schedule_data;

key_schedule_entry generate_schedule_entry(uint8 map, key_schedule_data *current_schedule_data, uint8 algorithm);
key_schedule_entry generate_schedule_entry(uint8 map, key_schedule_data *current_schedule_data);

void key_schedule_algorithm(uint8 map, key_schedule_data *current_schedule_data, uint8 algorithm);

void key_schedule_algorithm_0(uint8 map, key_schedule_data *current_schedule_data);
void key_schedule_algorithm_1(uint8 map, key_schedule_data *current_schedule_data);
void key_schedule_algorithm_2(uint8 map, key_schedule_data *current_schedule_data);
void key_schedule_algorithm_3(uint8 map, key_schedule_data *current_schedule_data);
void key_schedule_algorithm_4(uint8 map, key_schedule_data *current_schedule_data);
void key_schedule_algorithm_5(uint8 map, key_schedule_data *current_schedule_data);
void key_schedule_algorithm_6(uint8 map, key_schedule_data *current_schedule_data);
void key_schedule_algorithm_7(uint8 map, key_schedule_data *current_schedule_data);

#endif //KEY_SCHEDULE_H