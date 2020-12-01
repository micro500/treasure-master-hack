#ifndef TM_AVX_8_H
#define TM_AVX_8_H
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

class tm_avx_8 : public TM_base
{
public:
	tm_avx_8(RNG* rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	virtual void run_one_map(key_schedule_entry schedule_entry);

	virtual void run_all_maps(key_schedule_entry* schedule_entries);

private:
	void initialize();
	void alg_0(uint16* rng_seed);
	void alg_1(uint16* rng_seed);
	void alg_2(uint16* rng_seed);
	void alg_3(uint16* rng_seed);
	void alg_4(uint16* rng_seed);
	void alg_5(uint16* rng_seed);
	void alg_6(uint16* rng_seed);
	void alg_7();
	void add_alg(const uint8* addition_values_lo, const uint8* addition_values_hi, uint16* rng_seed);

	ALIGNED(32) uint8 working_code_data[128];
	static bool initialized;
};
#endif // TM_AVX_8_H