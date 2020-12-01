#ifndef TM_BASE_H
#define TM_BASE_H
#include <string>
#include "alignment2.h"
#include "data_sizes.h"
#include "rng_obj.h"
#include "key_schedule.h"

#define reverse_offset(x) (127 - (x))

#define OP_JAM 0x01
#define OP_ILLEGAL 0x02
#define OP_NOP2 0x04
#define OP_NOP 0x08
#define OP_JUMP 0x10

#define FIRST_BYTE_VALID 0x01
#define FIRST_BYTES_TO_JUMP_VALID 0x02
#define USES_ILLEGAL_OPCODES 0x04
#define USES_NOP 0x08
#define USES_UNOFFICIAL_NOPS 0x10
#define USES_JAM 0x20

class TM_base
{
public:
	TM_base(RNG *rng);

	virtual void load_data(uint8* new_data) = 0;
	virtual void fetch_data(uint8* new_data) = 0;

	virtual void expand(uint32 key, uint32 data) = 0;

	virtual void run_alg(int algorithm_id, uint16 * rng_seed, int iterations) = 0;

	virtual void run_one_map(key_schedule_entry schedule_entry) = 0;

	virtual void run_all_maps(key_schedule_entry* schedule_entries) = 0;

	virtual uint16 generate_stats(uint32 key, uint32 data, key_schedule_entry* schedule_entries, bool use_hashing);

	void print_working_code();
	uint8 check_machine_code(uint8* data, int length);

	RNG * rng;
	std::string obj_name;

	ALIGNED(64) static uint8 carnival_data[128];
	ALIGNED(64) static uint8 carnival_checksum_mask[128];

	ALIGNED(64) static uint8 other_world_data[128];
	ALIGNED(64) static uint8 other_world_checksum_mask[128];

	static uint8 opcode_bytes_used[0x100];
	static uint8 opcode_type[0x100];
};
#endif // TM_BASE_H