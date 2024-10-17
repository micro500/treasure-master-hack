#ifndef WORKING_CODE_H
#define WORKING_CODE_H
#include <vector>

#include "data_sizes.h"
#include "rng.h"
#include "key_schedule.h"

struct rng_info
{
	int byte_pos;
	uint8 bit_pos;

	uint8 bit;
};

class working_code
{
public:
	// Seeded with 8 uint8 values (4 key and 4 data bytes)
	working_code(uint8 value[8]);
	working_code(working_code *);
	void copy(working_code *);

	// Backup the starting value for later
	uint8 starting_value[8];

	// Store/retrieve the working code as different data sizes
	union
	{
		uint8 as_uint8[128];
		uint16 as_uint16[64];
		uint32 as_uint32[32];
		uint64 as_uint64[16];
	} working_code_data;

	// Trust mask
	// 1 means we trust that bit
	// 0 means we don't
	union
	{
		uint8 as_uint8[128];
		uint16 as_uint16[64];
		uint32 as_uint32[32];
		uint64 as_uint64[16];
	} trust_mask;

	RNG rng;

	void process_map_exit(uint8 map_number, key_schedule_entry schedule_entry);

	void process_working_code(uint8 map_number, key_schedule_entry schedule_entry);

	void working_code_algorithm(uint8 algorithm_number, uint8 map_number);
	void inverse_working_code_algorithm(uint8 algorithm_number, int &rng_pos, rng_info rng_bits[], int &bits_used, uint8 reverse_rng_table[]);

	void display_working_code();

	bool operator==(const working_code &other) const;
	bool operator<(const working_code &other) const;
};

#endif //WORKING_CODE_H