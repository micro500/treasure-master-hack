#ifndef TM_AVX_R128_8_H
#define TM_AVX_R128_8_H
#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
#include <pmmintrin.h> //SSE3
#include <tmmintrin.h> //SSSE3
#include <smmintrin.h> //SSE4.1
#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A
#include <immintrin.h> //AVX
//#include <zmmintrin.h> //AVX512

#include <optional>

#include "data_sizes.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

#define WC_ARGS __m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3, __m128i& wc4, __m128i& wc5, __m128i& wc6, __m128i& wc7

#define WC_PASS wc0, wc1, wc2, wc3, wc4, wc5, wc6, wc7

#define WC_VARS __m128i WC_PASS

#define WCXOR_PASS wc_xor0, wc_xor1, wc_xor2, wc_xor3, wc_xor4, wc_xor5, wc_xor6, wc_xor7
#define WCXOR_VARS __m128i WCXOR_PASS
#define WCXOR_COPY wc_xor0 = wc0; wc_xor1 = wc1; wc_xor2 = wc2; wc_xor3 = wc3; wc_xor4 = wc4; wc_xor5 = wc5; wc_xor6 = wc6; wc_xor7 = wc7;

// Like tm_avx_r128s_8 but uses non-shuffled RNG tables with a natural (contiguous)
// memory layout:  wc0 = bytes[0..15], wc1 = bytes[16..31], ..., wc7 = bytes[112..127].
// Tables: expansion_values_8, alg0_values_8, regular_rng_values_8, alg4_values_8,
//         alg6_values_8 (all non-shuffled, stride = seed*128).
// Carry seeds: alg2_values_8_8, alg5_values_8_8 (1 byte per seed, same as tm_8).

class tm_avx_r128_8 : public TM_base
{
public:
	tm_avx_r128_8(RNG* rng);
	tm_avx_r128_8(RNG* rng, uint32_t key);
	tm_avx_r128_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	void test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed);
	void test_expansion(uint32_t data, uint8* result_out);
	void test_bruteforce_data(uint32 data, uint8* result_out);
	bool test_bruteforce_checksum(uint32 data, int world);

	void run_bruteforce_boinc(uint32 start_data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);

	void compute_challenge_flags(uint32 data, uint8& carnival_flags_out, uint8& other_flags_out);

//private:
	void initialize();
	void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	void alg_0(WC_ARGS, uint16* rng_seed);
	void alg_2(WC_ARGS, uint16* rng_seed);
	void alg_3(WC_ARGS, uint16* rng_seed);
	void alg_5(WC_ARGS, uint16* rng_seed);
	void alg_6(WC_ARGS, uint16* rng_seed);
	void alg_7(WC_ARGS);
	void xor_alg(WC_ARGS, uint8* values);
	void add_alg(WC_ARGS, uint16* rng_seed, uint8* rng_start);
	void alg_2_sub(__m128i& wc, __m128i& carry);
	void alg_5_sub(__m128i& wc, __m128i& carry);

	void _load_from_mem(WC_ARGS);
	void _store_to_mem(WC_ARGS);

	void _expand_code(uint32 data, WC_ARGS);

	void _run_alg(WC_ARGS, int algorithm_id, uint16* rng_seed);
	void _run_one_map(WC_ARGS, const key_schedule::key_schedule_entry& schedule_entry);
	void _run_all_maps(WC_ARGS);

	template<bool CHECK_CHECKSUM, int WORLD>
	std::optional<uint8> _decrypt_check(WC_ARGS);

	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(WC_ARGS, uint32 data, uint8* result_data, uint32* result_size);

	void _decrypt_carnival_world(WC_ARGS);
	void _decrypt_other_world(WC_ARGS);

	uint16 _calculate_carnival_world_checksum(WC_ARGS);
	uint16 _calculate_other_world_checksum(WC_ARGS);

	uint16 _fetch_carnival_world_checksum_value(WC_ARGS);
	uint16 _fetch_other_world_checksum_value(WC_ARGS);

	bool check_carnival_world_checksum(WC_ARGS);
	bool check_other_world_checksum(WC_ARGS);

	uint16 masked_checksum(WC_ARGS, uint8* mask);
	void mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask);
	uint16 fetch_checksum_value(WC_ARGS, uint8 code_length);

	ALIGNED(32) uint8 working_code_data[128];

	static const alignas(16) __m128i mask_FF;
	static const alignas(16) __m128i mask_FE;
	static const alignas(16) __m128i mask_7F;
	static const alignas(16) __m128i mask_80;
	static const alignas(16) __m128i mask_01;
	static const alignas(16) __m128i mask_top_01;
	static const alignas(16) __m128i mask_top_80;

	static bool initialized;
};
#endif // TM_AVX_R128_8_H
