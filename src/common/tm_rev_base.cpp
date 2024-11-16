#include "tm_rev_base.h"

TM_rev_base::TM_rev_base(RNG* rng_obj) : rng(rng_obj), alg_rng_seed_diff{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, rev_alg_list{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, rev_alg_list_length(0)
{
}

void TM_rev_base::set_algorithm_list(std::vector<int>& algorithm_list)
{
	rev_alg_list_length = algorithm_list.size();

	uint16_t rng_count = 128;

	for (int i = rev_alg_list_length - 1; i >= 0; i--)
	{
		alg_rng_seed_diff[i] = rng_count;
		int alg = algorithm_list[i];
		rev_alg_list[i] = alg;

		switch (alg)
		{
		case 1:
		case 3:
		case 4:
			rng_count += 128;
			break;
		case 2:
		case 5:
			rng_count += 1;
			break;
		}
	}
}