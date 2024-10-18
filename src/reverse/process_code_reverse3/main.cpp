#include "tm_rev_base.h"
#include "tm_rev_8.h"
#include "rng_obj.h"
#include "rev_stats.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cstdint>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int carnival_code_length = 0x72;
uint8_t carnival_world_working_code[0x80] = { 0xFD, 0x22, 0x3C, 0x40, 0x77, 0xEB, 0xD4, 0xEF, 0x9C, 0x44,
											0x93, 0x1C, 0xD7, 0xF8, 0x10, 0x97, 0x14, 0x93, 0x84, 0x22,
											0xDD, 0xE3, 0x3E, 0x77, 0x5C, 0x47, 0x11, 0x31, 0xAA, 0xD9,
											0xF1, 0x97, 0xE2, 0x44, 0x4E, 0x78, 0x05, 0x25, 0xCD, 0xBF,
											0xAE, 0xED, 0xCA, 0xD6, 0x1F, 0xD9, 0x30, 0x4D, 0x88, 0x18,
											0xB2, 0x89, 0xF6, 0x70, 0x43, 0xFE, 0x56, 0x3E, 0xF3, 0x1B,
											0x7C, 0xA0, 0xF7, 0xF8, 0xDF, 0xF5, 0x3C, 0xC7, 0xE9, 0xD5,
											0x24, 0x0E, 0xDA, 0xA9, 0xB0, 0xAA, 0x86, 0x51, 0x1F, 0x8F,
											0x4A, 0xEF, 0x8C, 0x81, 0xF8, 0x80, 0x4F, 0x8F, 0x54, 0xF2,
											0x8C, 0x14, 0x9C, 0xFA, 0xFE, 0xCF, 0x03, 0x82, 0x96, 0x4E,
											0x82, 0x4C, 0x4A, 0x72, 0x1C, 0x52, 0x2C, 0xDE, 0x0F, 0x94,
											0x58, 0xC2, 0xD6, 0x99, 0x36, 0x7F, 0xA3, 0xF0, 0xD1, 0x29,
											0xD0, 0x93, 0xBF, 0x42, 0xCF, 0x3D, 0xD2, 0x56 };

int other_world_code_length = 0x53;


void attack(std::vector<int> reverse_path, int depth_limit, TM_rev_base &tm)
{
	if (reverse_path.size() >= depth_limit)
	{
		for (auto it = reverse_path.begin(); it != reverse_path.end(); it++)
		{
			std::cout << *it << "-";
		}
		std::cout << std::endl;

		tm.set_algorithm_list(reverse_path);

		for (int seed = 0; seed < 0x10000; seed++)
		{
			tm.set_rng_seed(seed);

			rev_stats res = tm.run_reverse_process();

			if (res.alg0_mismatch_bits == 0 || res.alg0_mismatch_bits == res.alg0_available_bits || res.alg6_mismatch_bits == 0 || res.alg6_mismatch_bits == res.alg6_available_bits)
			{
				std::cout << seed << "   " << res.alg0_mismatch_bits << " / " << res.alg0_available_bits << "   " << res.alg6_mismatch_bits << " / " << res.alg6_available_bits << std::endl;

				/*
				if (alg0_res.mismatch_bits == 0)
				{
					tm.reverse_algorithm(0, tm.rng_table);
					tm.print_code();
				}
				else if (alg0_res.mismatch_bits == alg0_res.available_bits)
				{
					tm.reverse_algorithm(7, tm.rng_table);
					tm.reverse_algorithm(0, tm.rng_table);
					tm.print_code();
				}
				else if (alg6_res.mismatch_bits == 0)
				{
					tm.reverse_algorithm(6, tm.rng_table);
					tm.print_code();
				}
				else if (alg6_res.mismatch_bits == alg6_res.available_bits)
				{
					tm.reverse_algorithm(7, tm.rng_table);
					tm.reverse_algorithm(6, tm.rng_table);
					tm.print_code();
				}
				*/
			}
		}

	}
	else
	{
		int alg_options[] = { 2,5,3,1,4 };
		for (int i = 0; i < 5; i++)
		{
			int next_alg = alg_options[i];

			std::vector<int> next_reverse_path = reverse_path;
			next_reverse_path.push_back(next_alg);
			attack(next_reverse_path, depth_limit, tm);

			if (next_alg == 1 || next_alg == 4)
			{
				std::vector<int> next_reverse_path_w_rev = reverse_path;
				next_reverse_path_w_rev.push_back(7);
				next_reverse_path_w_rev.push_back(next_alg);
				attack(next_reverse_path_w_rev, depth_limit, tm);
			}
		}
	}
}

int main()
{
	RNG rng;

	tm_rev_8 _tm(&rng);

	std::vector<int> reverse_path = { 2, 3, 7, 2, 3, 7, 4, 3 };
	/*reverse_path.push_back(2);
	reverse_path.push_back(3);

	reverse_path.push_back(2);
	reverse_path.push_back(3);*/

	uint8_t carnival_machine_code_trust[128];

	for (int i = 0; i < 128; i++)
	{
		if (i > (127 - carnival_code_length))
		{
			carnival_machine_code_trust[i] = 0xFF;
		}
		else
		{
			carnival_machine_code_trust[i] = 0x00;
		}
	}
	
	_tm.set_working_code(carnival_world_working_code);
	_tm.set_trust_mask(carnival_machine_code_trust);

	/*
	std::vector<int> alg_list = { 2, 3, 7, 2, 3, 7, 4, 3 };
	_tm.set_algorithm_list(alg_list);
	
	_tm.set_rng_seed(0x6f0f);

	_tm._load_from_mem();
	_tm.rev_alg_2();
	_tm.rev_alg_3(_tm.rng_seed_forward[_tm.alg_rng_seed_diff[1]]);
	_tm.rev_alg_7();
	_tm.rev_alg_2();
	_tm.rev_alg_3(_tm.rng_seed_forward[_tm.alg_rng_seed_diff[4]]);
	_tm.rev_alg_7();
	_tm.rev_alg_4(_tm.rng_seed_forward[_tm.alg_rng_seed_diff[6]]);
	_tm.rev_alg_3(_tm.rng_seed_forward[_tm.alg_rng_seed_diff[7]]);

	rev_stats x = _tm.check_alg06();
	*/

	attack(reverse_path, 6, _tm);

	return 0;
}
