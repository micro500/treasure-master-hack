#ifndef TM_AVX512_R512S_8_H
#define TM_AVX512_R512S_8_H
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

class tm_avx512_r512s_8 : public TM_base
{
public:
	tm_avx512_r512s_8(RNG* rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	virtual void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);

	virtual void run_all_maps(const key_schedule& schedule_entries);

private:
	void initialize();
	void alg_0(__m512i& working_code0, __m512i& working_code1, uint16* rng_seed, __m512i& mask_FE);
	void alg_2(__m512i& working_code0, __m512i& working_code1, uint16* rng_seed, __m512i& mask_80, __m512i& mask_7F, __m512i& mask_FE, __m512i& mask_01);
	void alg_3(__m512i& working_code0, __m512i& working_code1, uint16* rng_seed);
	void alg_5(__m512i& working_code0, __m512i& working_code1, uint16* rng_seed, __m512i& mask_80, __m512i& mask_7F, __m512i& mask_FE, __m512i& mask_01);
	void alg_6(__m512i& working_code0, __m512i& working_code1, uint16* rng_seed, __m512i& mask_7F);
	void alg_7(__m512i& working_code0, __m512i& working_code1, __m512i& mask_FF);
	void add_alg(__m512i& working_code0, __m512i& working_code1, uint16* rng_seed, uint8* rng_start);
	void alg_2_sub(__m512i& working_a, __m512i& working_b, __m512i& carry, __m512i& mask_80, __m512i& mask_7F, __m512i& mask_FE, __m512i& mask_01);
	void alg_5_sub(__m512i& working_a, __m512i& working_b, __m512i& carry, __m512i& mask_80, __m512i& mask_7F, __m512i& mask_FE, __m512i& mask_01);

	int shuffle(int addr);

	ALIGNED(64) uint8 working_code_data[128];

	static bool initialized;
};
#endif // TM_AVX512_R512S_8_H