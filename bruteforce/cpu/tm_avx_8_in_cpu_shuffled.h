#ifndef TM_AVX_8_IN_CPU_SHUFFLED_H
#define TM_AVX_8_IN_CPU_SHUFFLED_H
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

class tm_avx_8_in_cpu_shuffled : public TM_base
{
public:
	tm_avx_8_in_cpu_shuffled(RNG* rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	virtual void run_one_map(key_schedule_entry schedule_entry);

	virtual void run_all_maps(key_schedule_entry* schedule_entries);

private:
	void initialize();
	void alg_0(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_FE);
	void alg_2(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_01, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01);
	void alg_3(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed);
	void alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01);
	void alg_6(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_7F);
	void alg_7(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& mask_FF);
	void add_alg(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, uint8* rng_start);
	void alg_2_sub(__m256i& working_a, __m256i& working_b, __m256i& carry, __m256i& mask_top_01, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01);
	void alg_5_sub(__m256i& working_a, __m256i& working_b, __m256i& carry, __m256i& mask_top_80, __m256i& mask_80, __m256i& mask_7F, __m256i& mask_FE, __m256i& mask_01);
	
	ALIGNED(32) uint8 working_code_data[128];

	static bool initialized;
};
#endif // TM_AVX_8_IN_CPU_SHUFFLED_H