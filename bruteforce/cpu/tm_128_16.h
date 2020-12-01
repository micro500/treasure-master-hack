#ifndef TM_128_16_H
#define TM_128_16_H
#include "data_sizes.h"
#include "alignment2.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_128_16 : public TM_base
{
public:
	tm_128_16(RNG *rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16 * rng_seed, int iterations);

	virtual void run_one_map(key_schedule_entry schedule_entry);

	virtual void run_all_maps(key_schedule_entry* schedule_entries);


private:
	void initialize();

	void add_alg(uint8* addition_values, const uint16 rng_seed);
	void alg_0(const uint16 rng_seed);
	void alg_1(const uint16 rng_seed);
	void alg_2(const uint16 rng_seed);
	void alg_3(const uint16 rng_seed);
	void alg_4(const uint16 rng_seed);
	void alg_5(const uint16 rng_seed);
	void alg_6(const uint16 rng_seed);
	void alg_7();

	ALIGNED(32) uint8 working_code_data[128*2];

	static bool initialized;
};
#endif // TM_128_16_H