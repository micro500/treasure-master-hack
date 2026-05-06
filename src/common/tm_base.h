#ifndef TM_BASE_H
#define TM_BASE_H
#include <string>
#include <optional>
#include "alignment2.h"
#include "rng_obj.h"
#include "key_schedule.h"

#ifdef __unix__
#define __forceinline __attribute__((always_inline)) inline
#endif

#define WC_LIST_128(T, ...) T wc##__VA_ARGS__##0, T wc##__VA_ARGS__##1, T wc##__VA_ARGS__##2, T wc##__VA_ARGS__##3, T wc##__VA_ARGS__##4, T wc##__VA_ARGS__##5, T wc##__VA_ARGS__##6, T wc##__VA_ARGS__##7
#define WC_LIST_256(T, ...) T wc##__VA_ARGS__##0, T wc##__VA_ARGS__##1, T wc##__VA_ARGS__##2, T wc##__VA_ARGS__##3
#define WC_LIST_512(T, ...) T wc##__VA_ARGS__##0, T wc##__VA_ARGS__##1

#define WC_PASS_128       WC_LIST_128(,)
#define WC_PASS_256       WC_LIST_256(,)
#define WC_PASS_512       WC_LIST_512(,)

#define WC_VARS_128       __m128i WC_PASS_128
#define WC_VARS_256       __m256i WC_PASS_256
#define WC_VARS_512       __m512i WC_PASS_512

#define WC_ARGS_128       WC_LIST_128(__m128i&,)
#define WC_ARGS_256       WC_LIST_256(__m256i&,)
#define WC_ARGS_512       WC_LIST_512(__m512i&,)

#define WC_PASSx_128(S)   WC_LIST_128(, S)
#define WC_PASSx_256(S)   WC_LIST_256(, S)
#define WC_PASSx_512(S)   WC_LIST_512(, S)

#define WC_VARSx_128(S)   __m128i WC_PASSx_128(S)
#define WC_VARSx_256(S)   __m256i WC_PASSx_256(S)
#define WC_VARSx_512(S)   __m512i WC_PASSx_512(S)

#define WC_ARGSx_128(S)   WC_LIST_128(__m128i&, S)
#define WC_ARGSx_256(S)   WC_LIST_256(__m256i&, S)
#define WC_ARGSx_512(S)   WC_LIST_512(__m512i&, S)

#define WC_COPY_128(D, ...) wc##D##0 = wc##__VA_ARGS__##0; wc##D##1 = wc##__VA_ARGS__##1; wc##D##2 = wc##__VA_ARGS__##2; wc##D##3 = wc##__VA_ARGS__##3; wc##D##4 = wc##__VA_ARGS__##4; wc##D##5 = wc##__VA_ARGS__##5; wc##D##6 = wc##__VA_ARGS__##6; wc##D##7 = wc##__VA_ARGS__##7;
#define WC_COPY_256(D, ...) wc##D##0 = wc##__VA_ARGS__##0; wc##D##1 = wc##__VA_ARGS__##1; wc##D##2 = wc##__VA_ARGS__##2; wc##D##3 = wc##__VA_ARGS__##3;
#define WC_COPY_512(D, ...) wc##D##0 = wc##__VA_ARGS__##0; wc##D##1 = wc##__VA_ARGS__##1;

#define WC_XOR_PASS_128 WC_PASSx_128(_xor)
#define WC_XOR_PASS_256 WC_PASSx_256(_xor)
#define WC_XOR_PASS_512 WC_PASSx_512(_xor)

#define WC_XOR_VARS_128 WC_VARSx_128(_xor)
#define WC_XOR_VARS_256 WC_VARSx_256(_xor)
#define WC_XOR_VARS_512 WC_VARSx_512(_xor)

#define WC_COPY_XOR_128 WC_COPY_128(_xor)
#define WC_COPY_XOR_256 WC_COPY_256(_xor)
#define WC_COPY_XOR_512 WC_COPY_512(_xor)

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

	//virtual void run_bruteforce_boinc(uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size) {}
	//virtual void compute_challenge_flags(uint32 data, const key_schedule& schedule_entries, uint8& carnival_flags_out, uint8& other_flags_out) {}

	virtual void run_bruteforce_boinc(uint32_t start_data, uint32_t amount_to_run, void(*report_progress)(double), uint8_t* result_data, uint32_t result_max_size, uint32_t* result_size) = 0;

	virtual void compute_challenge_flags(uint32_t data, uint8_t& carnival_flags_out, uint8_t& other_flags_out) = 0;

	// Run a chain of algorithms with shared RNG state from a single initial seed.
	// algorithm_ids points to chain_length bytes, each in [0, 7]. data is the
	// 128-byte buffer (in-place). *rng_seed is updated to the post-chain RNG
	// state, but is only meaningful when tracks_rng_state() == true. The impl
	// is responsible for sizing/generating any internal RNG window.
	virtual void test_algorithm_chain(const uint8_t* algorithm_ids, int chain_length,
	                                  uint8_t* data, uint16_t* rng_seed) = 0;

	// Convenience wrapper: chain of length 1.
	void test_algorithm(int algorithm_id, uint8_t* data, uint16_t* rng_seed) {
		uint8_t id = static_cast<uint8_t>(algorithm_id);
		test_algorithm_chain(&id, 1, data, rng_seed);
	}

	// Tight loop used by the bench harness. Map variants intentionally do not
	// thread RNG state across iterations (they cycle a small cached window for
	// cache-friendly throughput measurement), so the post-call *rng_seed is
	// not meaningful even when tracks_rng_state() == true. Use
	// test_algorithm_chain when correctness across calls matters.
	virtual void test_algorithm_n(int algorithm_id, uint8_t* data, uint16_t* rng_seed, int iterations) = 0;

	// True if test_algorithm / test_algorithm_chain advance *rng_seed to the
	// post-call RNG state. Map variants pre-generate a fixed-length RNG window
	// per call and do not thread state across calls, so the returned seed is
	// meaningless for them.
	virtual bool tracks_rng_state() const { return true; }
	virtual void test_expansion(uint32_t data, uint8_t* result_out) = 0;
	virtual void test_bruteforce_data(uint32_t data, uint8_t* result_out) = 0;
	virtual bool test_bruteforce_checksum(uint32_t data, int world) = 0;

	// Update the key + schedule and refresh any cached per-key state. Default
	// only updates the public key/schedule fields; map variants override to
	// also regenerate their cached RNG tables.
	virtual void set_key(uint32_t new_key) {
		key = new_key;
		schedule_entries = key_schedule(new_key, key_schedule::ALL_MAPS);
	}

	uint8_t check_machine_code(uint8_t* data, int world);

	RNG * rng;
	std::string obj_name;
	uint32_t key;
	std::optional<key_schedule> schedule_entries;

	void shuffle_mem(uint8_t* src, uint8_t* dest, int bits, bool packing_16);
	void unshuffle_mem(uint8_t* src, uint8_t* dest, int bits, bool packing_16);

	ALIGNED(128) uint8_t working_code_data[128];

	ALIGNED(128) static uint8_t carnival_world_data[128];
	ALIGNED(128) static uint8_t carnival_world_checksum_mask[128];
	ALIGNED(128) static uint8_t other_world_data[128];
	ALIGNED(128) static uint8_t other_world_checksum_mask[128];

	static uint8_t opcode_bytes_used[0x100];
	static uint8_t opcode_type[0x100];

	ALIGNED(128) uint8_t carnival_world_checksum_mask_shuffled[128];
	ALIGNED(128) uint8_t carnival_world_data_shuffled[128];
	ALIGNED(128) uint8_t other_world_checksum_mask_shuffled[128];
	ALIGNED(128) uint8_t other_world_data_shuffled[128];
};
#endif // TM_BASE_H