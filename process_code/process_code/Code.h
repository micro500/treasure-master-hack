#ifndef CODE_H
#define CODE_H
#include "data_sizes.h"

class Code
{
public:
	Code(uint8 value[8]);

	uint8 starting_value[8];
	union
	{
		uint8 as_uint8[128];
		uint16 as_uint16[64];
		uint32 as_uint32[32];
		uint64 as_uint64[16];
	} working_code;

	union
	{
		uint8 as_uint8[4];
		uint16 as_uint16[2];
		uint32 as_uint32;
	} code_backup;

	// RNG values
	uint8 rng1;
	uint8 rng2;

	uint8 rng();
	uint8 rng_real();

	void process_map_exit(uint8 map_number);

	void process_working_code(uint8 map_number);

	void working_code_algorithm(uint8 algorithm_number, uint8 map_number);

	void code_backup_algorithm(uint8 algorithm_number, uint8 map_number);
	void code_backup_algorithm_0(uint8 map_number);
	void code_backup_algorithm_1(uint8 map_number);
	void code_backup_algorithm_2(uint8 map_number);
	void code_backup_algorithm_3(uint8 map_number);
	void code_backup_algorithm_4(uint8 map_number);
	void code_backup_algorithm_5(uint8 map_number);
	void code_backup_algorithm_6(uint8 map_number);
	void code_backup_algorithm_7(uint8 map_number);

	void display_working_code();

	bool operator==(const Code &other) const;
	bool operator<(const Code &other) const;
};

#endif //CODE_H