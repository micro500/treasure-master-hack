#ifndef TM_SSE2_8_SHUFFELD_H
#define TM_SSE2_8_SHUFFELD_H
#include <mmintrin.h>  //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2

#include "data_sizes.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_sse2_8_shuffled : public TM_base
{
public:
	tm_sse2_8_shuffled(RNG* rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	virtual void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);

	virtual void run_all_maps(const key_schedule& schedule_entries);


private:
	void initialize();

	void add_alg(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint8* rng_start, uint16* rng_seed);
	void alg_0(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_FE);
	void alg_2_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void alg_2(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_top_01, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void alg_3(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed);
	void alg_5_sub(__m128i& working_a, __m128i& working_b, __m128i& carry, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void alg_5(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_top_80, __m128i& mask_80, __m128i& mask_7F, __m128i& mask_FE, __m128i& mask_01);
	void alg_6(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, uint16* rng_seed, __m128i& mask_7F);
	void alg_7(__m128i& working_code0, __m128i& working_code1, __m128i& working_code2, __m128i& working_code3, __m128i& working_code4, __m128i& working_code5, __m128i& working_code6, __m128i& working_code7, __m128i& mask_FF);

	ALIGNED(32) uint8 working_code_data[128];

	static bool initialized;
};
#endif // TM_SSE2_8_SHUFFELD_H