#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include "rng_obj.h"
#include "rev_stats.h"

class TM_rev_base
{
public:
	TM_rev_base(RNG* rng);

	virtual void set_working_code(uint8_t* data) = 0;
	virtual void set_trust_mask(uint8_t* data) = 0;

	virtual void set_algorithm_list(std::vector<int>& algorithm_list);
	virtual void set_rng_seed(uint16_t seed) = 0;

	virtual rev_stats run_reverse_process() = 0;
	//virtual void run_alg(int algorithm_id, uint16* rng_seed, int iterations) = 0;

	RNG* rng;
	std::string obj_name;

//protected:
	uint8_t rev_alg_list[16];
	uint8_t rev_alg_list_length;
	uint16_t alg_rng_seed_diff[16];
};
