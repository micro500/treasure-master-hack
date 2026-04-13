#ifndef TM_BASE_H
#define TM_BASE_H
#include <string>
#include <optional>
#include "alignment2.h"
#include "data_sizes.h"
#include "rng_obj.h"
#include "key_schedule.h"

#ifdef __unix__
#define __forceinline __attribute__((always_inline)) inline
#endif

#define reverse_offset(x) (127 - (x))

#define OP_JAM 0x01
#define OP_ILLEGAL 0x02
#define OP_NOP2 0x04
#define OP_NOP 0x08
#define OP_JUMP 0x10

#define FIRST_ENTRY_VALID 0x02
#define ALL_ENTRIES_VALID 0x04
#define USES_NOP 0x10
#define USES_UNOFFICIAL_NOPS 0x20
#define USES_ILLEGAL_OPCODES 0x40
#define USES_JAM 0x80

#define CARNIVAL_WORLD 0
#define OTHER_WORLD 1

#define CARNIVAL_WORLD_CODE_LENGTH 0x72
#define OTHER_WORLD_CODE_LENGTH 0x53

class TM_base
{
public:
	TM_base(RNG *rng);
	virtual ~TM_base() = default;

	virtual void run_bruteforce_boinc(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size) {}
	virtual void compute_challenge_flags(uint32 key, uint32 data, const key_schedule& schedule_entries, uint8& carnival_flags_out, uint8& other_flags_out) {}

	uint8 check_machine_code(uint8* data, int world);

	RNG * rng;
	std::string obj_name;
	uint32_t key;
	std::optional<key_schedule> schedule_entries;

	void shuffle_mem(uint8* src, uint8* dest, int bits, bool packing_16);
	void unshuffle_mem(uint8* src, uint8* dest, int bits, bool packing_16);

	ALIGNED(64) static uint8 carnival_world_data[128];
	ALIGNED(64) static uint8 carnival_world_checksum_mask[128];

	ALIGNED(64) static uint8 other_world_data[128];
	ALIGNED(64) static uint8 other_world_checksum_mask[128];

	static uint8 opcode_bytes_used[0x100];
	static uint8 opcode_type[0x100];
};
#endif // TM_BASE_H