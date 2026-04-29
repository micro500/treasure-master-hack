#ifndef TM_AVX_M256_16_H
#define TM_AVX_M256_16_H
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

#include <memory>
#include <vector>
#include "data_sizes.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_avx_m256_16 : public TM_base
{
public:
	tm_avx_m256_16(RNG* rng);

	void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	void expand(uint32 key, uint32 data);

	void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	void run_all_maps(const key_schedule& schedule_entries);

private:
	void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void initialize();
	void alg_0(uint16* rng_seed);
	void alg_1(uint16* rng_seed);
	void alg_2(uint16* rng_seed);
	void alg_3(uint16* rng_seed);
	void alg_4(uint16* rng_seed);
	void alg_5(uint16* rng_seed);
	void alg_6(uint16* rng_seed);
	void alg_7();
	void add_alg(const uint8* addition_values, uint16* rng_seed);

	ALIGNED(32) uint8 working_code_data[128*2];

	bool _initialized = false;
	std::vector<std::shared_ptr<void>> _table_refs;

	uint8_t* _expansion_8 = nullptr;
	uint16_t* _seed_fwd_1 = nullptr;
	uint16_t* _seed_fwd_128 = nullptr;
	uint16_t* _regular_16 = nullptr;
	uint16_t* _alg0_16 = nullptr;
	uint8_t* _alg2_256_16 = nullptr;
	uint16_t* _alg4_16 = nullptr;
	uint8_t* _alg5_256_16 = nullptr;
	uint16_t* _alg6_16 = nullptr;
};
#endif // TM_AVX_M256_16_H