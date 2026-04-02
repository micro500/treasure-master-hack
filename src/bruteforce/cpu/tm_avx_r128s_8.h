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

class tm_avx_r128s_8 : public TM_base
{
public:
	struct reg_masks {
		__m128i vFF = _mm_set1_epi16(0xFFFF);
		__m128i vFE = _mm_set1_epi16(0xFEFE);
		__m128i v7F = _mm_set1_epi16(0x7F7F);
		__m128i v80 = _mm_set1_epi16(0x8080);
		__m128i v01 = _mm_set1_epi16(0x0101);
		__m128i top01 = _mm_set_epi16(0x0100, 0, 0, 0, 0, 0, 0, 0);
		__m128i top80 = _mm_set_epi16(0x8000, 0, 0, 0, 0, 0, 0, 0);
	};

	struct wc_r128 {
		__m128i wc0;
		__m128i wc1;
		__m128i wc2;
		__m128i wc3;
		__m128i wc4;
		__m128i wc5;
		__m128i wc6;
		__m128i wc7;
	};

	tm_avx_r128s_8(RNG* rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	void decrypt_carnival_world();
	void decrypt_other_world();

	uint16 calculate_carnival_world_checksum();
	uint16 calculate_other_world_checksum();

	uint16 fetch_carnival_world_checksum_value();
	uint16 fetch_other_world_checksum_value();

	virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	virtual void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);

	virtual void run_all_maps(const key_schedule& schedule_entries);

	void run_bruteforce_data(uint32 key, uint32 data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);

	virtual void test_expand_and_map(uint32 key, uint32 data, const key_schedule& schedule, uint8* result_out);
	virtual bool test_pipeline_validate(uint32 key, uint32 data, const key_schedule& schedule, int world);

	void run_bruteforce_boinc(uint32 key, uint32 start_data, const key_schedule& schedule_entries, uint32 amount_to_run, void(*report_progress)(double), uint8* result_data, uint32 result_max_size, uint32* result_size);

	void compute_challenge_flags(uint32 key, uint32 data, const key_schedule& schedule_entries, uint8& carnival_flags_out, uint8& other_flags_out);

	void run_first_map(uint32_t key, uint32_t data, const key_schedule& schedule_entries);

//private:
	void initialize();
	void alg_0(wc_r128& wc, uint16* rng_seed, reg_masks& masks);
	void alg_2(wc_r128& wc, uint16* rng_seed, reg_masks& masks);
	void alg_3(wc_r128& wc, uint16* rng_seed);
	void alg_5(wc_r128& wc, uint16* rng_seed, reg_masks& masks);
	void alg_6(wc_r128& wc, uint16* rng_seed, reg_masks& masks);
	void alg_7(wc_r128& wc, reg_masks& masks);
	void xor_alg(wc_r128& wc, uint8* values);
	void add_alg(wc_r128& wc, uint16* rng_seed, uint8* rng_start);
	void alg_0_sub(__m128i& working_code, uint8* rng_start, reg_masks& masks);
	void alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, reg_masks& masks);
	void alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, reg_masks& masks);
	void alg_6_sub(__m128i& working_code, uint8* rng_start, reg_masks& masks);
	
	void _load_from_mem(wc_r128& wc);
	void _store_to_mem(wc_r128& wc);

	void _expand_code(uint32 key, uint32 data, wc_r128& wc);
	
	void _run_all_maps(wc_r128& wc, const key_schedule& schedule_entries, reg_masks& m);

	void _decrypt_carnival_world(wc_r128& wc);
	void _decrypt_other_world(wc_r128& wc);

	uint16 _calculate_carnival_world_checksum(wc_r128& wc);
	uint16 _calculate_other_world_checksum(wc_r128& wc);

	uint16 _fetch_carnival_world_checksum_value(__m128i& working_code0, __m128i& working_code1);
	uint16 _fetch_other_world_checksum_value(__m128i& working_code0, __m128i& working_code1);

	bool check_carnival_world_checksum(wc_r128& wc);
	bool check_other_world_checksum(wc_r128& wc);

	uint16 masked_checksum(wc_r128& wc, uint8* mask);
	void mid_sum(__m128i& sum, __m128i& working_code, __m128i& sum_mask, __m128i& lo_mask);
	uint16 fetch_checksum_value(__m128i& working_code0, __m128i& working_code1, uint8 code_length);

	ALIGNED(32) uint8 working_code_data[128];

	ALIGNED(32) static uint8 carnival_world_checksum_mask_shuffled[128];
	ALIGNED(32) static uint8 carnival_world_data_shuffled[128];

	ALIGNED(32) static uint8 other_world_checksum_mask_shuffled[128];
	ALIGNED(32) static uint8 other_world_data_shuffled[128];

	static bool initialized;
};
#endif // TM_AVX_R128S_8_H