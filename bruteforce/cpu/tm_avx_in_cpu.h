#ifndef TM_AVX_IN_CPU_H
#define TM_AVX_IN_CPU_H
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

class tm_avx_in_cpu : public TM_base
{
public:
	tm_avx_in_cpu(RNG *rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16 * rng_seed, int iterations);

	virtual void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);

	virtual void run_all_maps(const key_schedule& schedule_entries);

private:
	void initialize();
	void alg_0(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF);
	void alg_1(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF);
	void alg_2(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_top_01, __m256i& mask_alg2, __m256i& mask_007F, __m256i& mask_FE00, __m256i& mask_0080);
	void alg_3(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed);
	void alg_4(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF);
	void alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_alg5, __m256i& mask_7F00, __m256i& mask_00FE, __m256i& mask_0001);
	void alg_6(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, uint16* rng_seed, __m256i& mask_FF);
	void alg_7(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, __m256i& working_code4, __m256i& working_code5, __m256i& working_code6, __m256i& working_code7, __m256i& mask_FF);

	ALIGNED(32) uint8 working_code_data[128 * 2];

	static bool initialized;
};
#endif // TM_AVX_IN_CPU_H