#include "tm_rev_base.h"

TM_rev_base::TM_rev_base(RNG* rng_obj) : rng(rng_obj), alg_rng_seed_diff{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, rev_alg_list{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, rev_alg_list_length(0)
{
}

void TM_rev_base::set_algorithm_list(std::vector<int>& algorithm_list)
{
	rev_alg_list_length = algorithm_list.size();

	uint16_t rng_count = 128;

	for (int i = 15; i >= 0; i--)
	{
		int alg = -1;

		if (i < rev_alg_list_length)
		{
			alg = algorithm_list[i];
			alg_rng_seed_diff[i] = rng_count;

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

		rev_alg_list[i] = alg;
	}
}