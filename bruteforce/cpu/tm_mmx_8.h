#ifndef _WIN64
#ifndef TM_MMX_8_H
#define TM_MMX_8_H
#include <mmintrin.h>  //MMX

#include "data_sizes.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_mmx_8 : public TM_base
{
public:
	tm_mmx_8(RNG* rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	virtual void run_one_map(key_schedule_entry schedule_entry);

	virtual void run_all_maps(key_schedule_entry* schedule_entries);

private:
	void initialize();
	/*
	void alg_5(__m256i& working_code0, __m256i& working_code1, __m256i& working_code2, __m256i& working_code3, uint16* rng_seed, __m256i& mask_top_80, __m256i& mask_alg5, __m256i& mask_7F00, __m256i& mask_00FE, __m256i& mask_0001);
	
	*/
	void alg_0(uint16* rng_seed, __m64& mask_FE);
	void alg_2(uint16* rng_seed, __m64& mask_hi, __m64& mask_lo, __m64& mask_007F, __m64& mask_0080, __m64& mask_FE00, __m64& mask_0100, __m64& mask_top_01);
	void alg_3(uint16* rng_seed);
	void alg_5(uint16* rng_seed, __m64& mask_hi, __m64& mask_lo, __m64& mask_007F, __m64& mask_0080, __m64& mask_FE00, __m64& mask_0100, __m64& mask_top_01);
	void alg_6(uint16* rng_seed, __m64& mask_7F);
	void alg_7(__m64& mask_FF);
	void add_alg(uint16* rng_seed, uint8* rng_start);

	ALIGNED(32) uint8 working_code_data[128];
	static bool initialized;
};
#endif // TM_MMX_8_H
#endif