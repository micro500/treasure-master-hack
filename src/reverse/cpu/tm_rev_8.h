#pragma once
#include <cstdint>

#include "rng_obj.h"
#include "tm_rev_base.h"

class tm_rev_8 : public TM_rev_base
{
public:
	tm_rev_8(RNG* rng);

	virtual void set_working_code(uint8_t* data);
	virtual void set_trust_mask(uint8_t* data);

	virtual void set_rng_seed(uint16_t seed);

	virtual rev_stats run_reverse_process();
	rev_stats check_alg06();

	void rev_alg_0();
	void rev_alg_1(uint16_t rng_seed);
	void rev_alg_2();
	void rev_alg_3(uint16_t rng_seed);
	void rev_alg_4(uint16_t rng_seed);
	void rev_alg_5();
	void rev_alg_6();
	void rev_alg_7();

	void _load_from_mem();

private:
	void initialize();

	void add_alg(uint8_t* addition_values, const uint16_t rng_seed);
	void xor_alg(uint8_t* working_data, uint8_t* xor_values);


	uint8_t init_working_code_data[128];
	uint8_t init_trust_mask[128];

	uint8_t working_code_data[128];
	uint8_t trust_mask[128];

	uint16_t* rng_seed_forward;

	static bool initialized;
};
