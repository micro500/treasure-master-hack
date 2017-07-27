#ifndef CMD_ARGS_H
#define CMD_ARGS_H

#include <string>

#include "data_sizes.h"

enum attack_type 
{
	INDIVIDUAL,
	VECTOR,
	VECTOR_CONSTANT_SIZE,
	HASH
};

struct cli_options
{
	//  -k, --key "XXXXXXXX": supply 8 hex values to use as the key value, defaults to all 0's?
	uint32 start_key;

	//  -d, --data "XXXXXXXX": supply 8 hex values to use as the starting data value, defaults to all 0's
	uint32 start_data;

	//  -c, --count 
	uint64 iv_count;

	//  -e, --vector_size
	uint64 vector_size;

	//  -a, --attack attack_type:
	//               "individual": each IV is processed and checked individually
	//               "vector":     each working code is added to a std::vector, processed one round, sorted, then std::unique'd
	//               "hash":       each working code is processed a round, then checked against a hash table. collisions are not processed any further
	attack_type attack;

	//  -b, --big-registers: use 128bit registers as an optimization
	bool big_registers;

	//  -s, --key-schedule: pre-process the key schedule as an optimization instead of processing it for every IV
	bool key_schedule;

	//  -f, --file filename: read a list of IVs to process from a file. Overrides -k and -d
	bool from_file;
	std::string filename;

	cli_options() : start_key(0),
		start_data(0),
		iv_count(1),
		vector_size(0x1000),
		attack(INDIVIDUAL),
		big_registers(false),
		key_schedule(true),
		from_file(false),
		filename(""){}

};

extern cli_options command_line_options;

void process_command_line(int argc, char **argv);

#endif //CMD_ARGS_H