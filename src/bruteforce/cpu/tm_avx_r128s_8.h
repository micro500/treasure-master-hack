#ifndef TM_AVX_R128S_8_H
#define TM_AVX_R128S_8_H
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

#include "data_sizes.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

#define WC_ARGS __m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3, __m128i& wc4, __m128i& wc5, __m128i& wc6, __m128i& wc7

#define WC_PASS wc0, wc1, wc2, wc3, wc4, wc5, wc6, wc7

#define WC_VARS __m128i WC_PASS


class tm_avx_r128s_8 : public TM_base
{
public:
	tm_avx_r128s_8(RNG* rng);
	tm_avx_r128s_8(RNG* rng, uint32_t key);
	tm_avx_r128s_8(RNG* rng, const uint32_t key, const key_schedule& schedule_entries);

	void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	void test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed);
	void test_expansion(uint32_t data, uint8* result_out);
	void test_bruteforce_data(uint32 data, uint8* result_out);
	bool test_bruteforce_checksum(uint32 data, int world);

	//void expand(uint32 data);

	void decrypt_carnival_world();
	void decrypt_other_world();

	uint16 calculate_carnival_world_checksum();
	uint16 calculate_other_world_checksum();

	uint16 fetch_carnival_world_checksum_value();
	uint16 fetch_other_world_checksum_value();

	void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	void run_all_maps(const key_schedule& schedule_entries);

	void run_bruteforce_data(uint32 data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);

	void run_bruteforce_boinc(uint32 start_data, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);

	void compute_challenge_flags(uint32 data, uint8& carnival_flags_out, uint8& other_flags_out);

	//void run_first_map(uint32_t data);

//private:
	void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void initialize();
	void alg_0(WC_ARGS, uint16* rng_seed);
	void alg_2(WC_ARGS, uint16* rng_seed);
	void alg_3(WC_ARGS, uint16* rng_seed);
	void alg_5(WC_ARGS, uint16* rng_seed);
	void alg_6(WC_ARGS, uint16* rng_seed);
	void alg_7(WC_ARGS);
	void xor_alg(WC_ARGS, uint8* values);
	void add_alg(WC_ARGS, uint16* rng_seed, uint8* rng_start);
	void alg_0_sub(__m128i& working_code, uint8* rng_start);
	void alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry);
	void alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry);
	void alg_6_sub(__m128i& working_code, uint8* rng_start);
	
	void _load_from_mem(WC_ARGS);
	void _store_to_mem(WC_ARGS);

	void _expand_code(uint32 data, WC_ARGS);
	
	void _run_alg(WC_ARGS, int algorithm_id, uint16* rng_seed);
	void _run_one_map(WC_ARGS, const key_schedule::key_schedule_entry& schedule_entry);
	void _run_all_maps(WC_ARGS);
	template<bool CHECK_CHECKSUMS>
	void _run_bruteforce(WC_ARGS, uint32 data, uint8* result_data, uint32* result_size);

	void _decrypt_carnival_world(WC_ARGS);
	void _decrypt_other_world(WC_ARGS);

	uint16 _calculate_carnival_world_checksum(WC_ARGS);
	uint16 _calculate_other_world_checksum(WC_ARGS);

	uint16 _fetch_carnival_world_checksum_value(__m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3);
	uint16 _fetch_other_world_checksum_value(__m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3);

	bool check_carnival_world_checksum(WC_ARGS);
	bool check_other_world_checksum(WC_ARGS);

	uint16 masked_checksum(WC_ARGS, uint8* mask);
	void mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask);
	uint16 fetch_checksum_value(__m128i& wc0, __m128i& wc1, __m128i& wc2, __m128i& wc3, uint8 code_length);

	void test_expand_and_map(uint32 key, uint32 data, const key_schedule& schedule, uint8* result_out);
	bool test_pipeline_validate(uint32 key, uint32 data, const key_schedule& schedule, int world);

	ALIGNED(32) uint8 working_code_data[128];

	ALIGNED(32) static uint8 carnival_world_checksum_mask_shuffled[128];
	ALIGNED(32) static uint8 carnival_world_data_shuffled[128];

	ALIGNED(32) static uint8 other_world_checksum_mask_shuffled[128];
	ALIGNED(32) static uint8 other_world_data_shuffled[128];

	static const alignas(16) __m128i mask_FF;
	static const alignas(16) __m128i mask_FE;
	static const alignas(16) __m128i mask_7F;
	static const alignas(16) __m128i mask_80;
	static const alignas(16) __m128i mask_01;
	static const alignas(16) __m128i mask_top_01;
	static const alignas(16) __m128i mask_top_80;

	static bool initialized;
};
#endif // TM_AVX_R128S_8_H