#include <cstdint>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <intrin.h>
#include "minterm.h"

uint16_t process_rng(uint16_t rngval_in)
{
	uint16_t rng_val = rngval_in;
	rng_val = rng_val + ((rng_val & 0xFF) << 8);
	rng_val = rng_val + 0x6daa + (rng_val > 0xd576 ? 1 : 0);
	return rng_val;
}

uint8_t get_rng_result(uint16_t rngval_in)
{
	return ((rngval_in >> 8) & 0xFF) ^ (rngval_in & 0xFF);
}

std::pair< std::vector<minterm>, std::vector<minterm>> get_minterms(std::vector<minterm> &init_minterms)
{
	std::vector<std::unordered_map <uint32_t, std::unordered_set<uint32_t>>> minterm_cat(18);
	std::unordered_map <uint32_t, std::unordered_set<uint32_t>> unique_minterms;

	for (auto it = init_minterms.begin(); it != init_minterms.end(); it++)
	{
		int bitcount = __popcnt(it->minterm_val);

		if (minterm_cat[bitcount].find(it->minterm_mask) == minterm_cat[bitcount].end())
		{
			minterm_cat[bitcount][it->minterm_mask] = std::unordered_set<uint32_t>();
		}

		minterm_cat[bitcount][it->minterm_mask].insert(it->minterm_val);

		if (unique_minterms.find(it->minterm_mask) == unique_minterms.end())
		{
			unique_minterms[it->minterm_mask] = std::unordered_set<uint32_t>();
		}

		unique_minterms[it->minterm_mask].insert(it->minterm_val);
	}


	std::vector<minterm> new_minterms;
	for (int cat_id = 0; cat_id < minterm_cat.size() - 1; cat_id++)
	{
		for (auto mask_it = minterm_cat[cat_id].begin(); mask_it != minterm_cat[cat_id].end(); mask_it++)
		{
			if (minterm_cat[cat_id + 1].find(mask_it->first) != minterm_cat[cat_id + 1].end())
			{
				for (auto val_it = mask_it->second.begin(); val_it != mask_it->second.end(); val_it++)
				{
					for (int i = 0; i < 17; i++)
					{
						uint32_t flip_bit = 0x01 << i;
						uint32_t bit_mask = flip_bit ^ 0x1FFFF;
						uint32_t search_val = (*val_it) ^ flip_bit;
						if (minterm_cat[cat_id + 1][mask_it->first].find(search_val) != minterm_cat[cat_id + 1][mask_it->first].end())
						{
							new_minterms.push_back(minterm((*val_it) & bit_mask, mask_it->first & bit_mask));

							unique_minterms[mask_it->first].erase((*val_it));
						}
					}
				}
			}
		}
	}

	std::vector<minterm> old_minterms;
	for (auto mask_it = unique_minterms.begin(); mask_it != unique_minterms.end(); mask_it++)
	{
		for (auto val_it = mask_it->second.begin(); val_it != mask_it->second.end(); val_it++)
		{
			old_minterms.push_back(minterm((*val_it), mask_it->first));
		}
	}

	std::pair< std::vector<minterm>, std::vector<minterm>> x;
	x.first = new_minterms;
	x.second = old_minterms;

	return x;
}

std::vector<minterm> qmc(std::vector<minterm>& init_minterms)
{
	std::vector<minterm> final_minterms;
	std::vector<minterm> last_minterms = init_minterms;

	while (1)
	{
		std::cout << "get_minterms" << std::endl;
		auto x = get_minterms(last_minterms);
		std::cout << "got: " << x.first.size() << " " << x.second.size() << std::endl;
		final_minterms.insert(final_minterms.end(), x.second.begin(), x.second.end());
		if (x.first.size() == 0)
		{
			break;
		}

		last_minterms = x.first;
	}

	std::unordered_map <uint32_t, std::unordered_map<uint32_t, uint32_t>> f_cat;

	for (int f_id = 0; f_id < final_minterms.size(); f_id++)
	{
		if (f_cat.find(final_minterms[f_id].minterm_mask) == f_cat.end())
		{
			f_cat[final_minterms[f_id].minterm_mask] = std::unordered_map<uint32_t, uint32_t>();
		}

		f_cat[final_minterms[f_id].minterm_mask][final_minterms[f_id].minterm_val] = f_id;
	}

	std::unordered_map <uint32_t, std::unordered_set<uint32_t>> chart(init_minterms.size());
	std::unordered_map <uint32_t, std::unordered_set<uint32_t>> chart2(final_minterms.size());

	for (auto mask_it = f_cat.begin(); mask_it != f_cat.end(); mask_it++)
	{
		for (int m_id = 0; m_id < init_minterms.size(); m_id++)
		{
			uint32_t masked_val = init_minterms[m_id].minterm_val & mask_it->first;
			if (mask_it->second.find(masked_val) != mask_it->second.end())
			{
				int f_id = f_cat[mask_it->first][masked_val];
				chart[m_id].insert(f_id);
				chart2[f_id].insert(m_id);
			}
		}
	}

	std::unordered_set<uint32_t> new_primes;
	std::unordered_set<uint32_t> final_primes;

	for (auto m_it = chart.begin(); m_it != chart.end(); m_it++)
	{
		if (m_it->second.size() == 1)
		{
			new_primes.insert((*m_it->second.begin()));
		}
	}

	while (new_primes.size() > 0)
	{
		final_primes.insert(new_primes.begin(), new_primes.end());

		std::unordered_set<uint32_t> m_todelete;
		for (auto f_it = new_primes.begin(); f_it != new_primes.end(); f_it++)
		{
			for (auto m_it = chart2[(*f_it)].begin(); m_it != chart2[(*f_it)].end(); m_it++)
			{
				m_todelete.insert((*m_it));
			}
		}

		std::unordered_set<uint32_t> f_id_tocheck;
		for (auto m_it = m_todelete.begin(); m_it != m_todelete.end(); m_it++)
		{
			f_id_tocheck.insert(chart[(*m_it)].begin(), chart[(*m_it)].end());
		}

		for (auto f_it = f_id_tocheck.begin(); f_it != f_id_tocheck.end(); f_it++)
		{
			for (auto m_it = m_todelete.begin(); m_it != m_todelete.end(); m_it++)
			{
				chart2[(*f_it)].erase((*m_it));
			}
		}

		int max_len = 0;
		int max_f_id = 0;

		for (int f_id = 0; f_id < chart2.size(); f_id++)
		{
			if (chart2[f_id].size() > max_len)
			{
				max_len = chart2[f_id].size();
				max_f_id = f_id;
			}
		}

		new_primes = std::unordered_set<uint32_t>();
		if (max_len > 0)
		{
			new_primes.insert(max_f_id);
		}
	}

	std::vector<minterm> result;
	for (auto f_it = final_primes.begin(); f_it != final_primes.end(); f_it++)
	{
		result.push_back(final_minterms[(*f_it)]);
	}

	return result;
}


int main()
{
	bool rng_fwd = true;

	for (int depth = 1; depth <= 128; depth++)
	{
		if (rng_fwd && depth != 1 && depth != 128)
		{
			continue;
		}
		int max_bit = 8;
		if (rng_fwd)
		{
			max_bit = 16;
		}
		for (int bit_index = 0; bit_index < max_bit; bit_index++)
		{
			std::vector<minterm> init_minterms;

			for (int i = 0; i < 256; i++)
			{
				for (int j = 0; j < 256; j++)
				{
					uint16_t init_rng_val = (j << 8) | i;

					uint16_t rng_val = init_rng_val;
					for (int depth_run = 0; depth_run < depth; depth_run++)
					{
						rng_val = process_rng(rng_val);
					}

					uint8_t rng_result = get_rng_result(rng_val);

					uint8_t bit_val = (rng_result >> bit_index) & 0x01;
					if (rng_fwd)
					{
						bit_val = (rng_val >> bit_index) & 0x01;
					}
					init_minterms.push_back(minterm((init_rng_val ^ 0xFFFF) | (bit_val << 16), 0x1FFFF));
				}
			}
			std::vector<minterm> primes = qmc(init_minterms);

			std::cout << "Final count: " << primes.size() << std::endl;

			std::ostringstream os;
			if (rng_fwd)
			{
				os << "rng_fwd_";
			}
			else
			{
				os << "rng_res_";
			}
			os << depth << "_" << bit_index << ".txt";
			std::string s = os.str();

			std::ofstream myfile;
			myfile.open(s);

			for (auto f_it = primes.begin(); f_it != primes.end(); f_it++)
			{
				for (int i = 0; i < 17; i++)
				{
					minterm cur = (*f_it);
					int bit_val = (cur.minterm_val >> i) & 0x01;
					int mask_val = (cur.minterm_mask >> i) & 0x01;

					if (mask_val == 1)
					{
						if (bit_val == 0) {
							myfile << "-";
						}
						myfile << (i + 1) << " ";
					}
				}
				myfile << "0" << std::endl;
			}
			myfile.close();
		}
	}
	/*
	std::cout << final_primes.size();

	std::ofstream myfile;
	myfile.open("example.txt");

	for (auto f_it = final_primes.begin(); f_it != final_primes.end(); f_it++)
	{
		for (int i = 0; i < 17; i++)
		{
			minterm cur = final_minterms[(*f_it)];
			int bit_val = (cur.minterm_val >> i) & 0x01;
			int mask_val = (cur.minterm_mask >> i) & 0x01;

			if (mask_val == 1)
			{
				if (bit_val == 0) {
					myfile << "-";
				}
				myfile << (i + 1) << " ";
			}
		}
		myfile << "0" << std::endl;
	}
	myfile.close();
	*/


	return 0;
}