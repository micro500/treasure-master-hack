#ifndef TM_32_16_H
#define TM_32_16_H
#include <memory>
#include <vector>
#include "data_sizes.h"
#include "rng_obj.h"
#include "tm_base.h"

class tm_32_16 : public TM_base
{
public:
	tm_32_16(RNG* rng);

	void load_data(uint8* new_data);
	void fetch_data(uint8* new_data);

	void expand(uint32 key, uint32 data);

	void run_alg(int algorithm_id, uint16* rng_seed, int iterations);

	void run_all_maps(const key_schedule& schedule_entries);

private:
	void run_one_map(const key_schedule::key_schedule_entry& schedule_entry);
	void initialize();

	void add_alg(uint32* addition_values, const uint16 rng_seed);
	void alg_0(const uint16 rng_seed);
	void alg_1(const uint16 rng_seed);
	void alg_2(const uint16 rng_seed);
	void alg_3(const uint16 rng_seed);
	void alg_4(const uint16 rng_seed);
	void alg_5(const uint16 rng_seed);
	void alg_6(const uint16 rng_seed);
	void alg_7();

	uint32 working_code_data[64];

	bool _initialized = false;
	std::vector<std::shared_ptr<void>> _table_refs;

	uint8_t* _expansion_8 = nullptr;
	uint16_t* _seed_fwd_1 = nullptr;
	uint16_t* _seed_fwd_128 = nullptr;
	uint16_t* _regular_16 = nullptr;
	uint16_t* _alg0_16 = nullptr;
	uint32_t* _alg2_32_16 = nullptr;
	uint16_t* _alg4_16 = nullptr;
	uint32_t* _alg5_32_16 = nullptr;
	uint16_t* _alg6_16 = nullptr;
};

#endif // TM_32_16_H