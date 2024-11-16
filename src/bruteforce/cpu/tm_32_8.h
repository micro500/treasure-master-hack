#ifndef TM_32_8_H
#define TM_32_8_H
#include "data_sizes.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_32_8 : public TM_base
{
public:
	tm_32_8(RNG* rng);

	virtual void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	virtual void expand(uint32 key, uint32 data);

	virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	virtual void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);

	virtual void run_all_maps(const key_schedule& schedule_entries);

private:
	void initialize();

	void add_alg(uint32* addition_values_lo, uint32* addition_values_hi, const uint16 rng_seed);
	void alg_0(const uint16 rng_seed);
	void alg_1(const uint16 rng_seed);
	void alg_2(const uint16 rng_seed);
	void alg_3(const uint16 rng_seed);
	void alg_4(const uint16 rng_seed);
	void alg_5(const uint16 rng_seed);
	void alg_6(const uint16 rng_seed);
	void alg_7();

	uint32 working_code_data[32];

	static bool initialized;
};

#endif // TM_32_8_H