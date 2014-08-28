#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

int count_ones(unsigned int x) {
        int o = 0;
        while (x) {
                o += x % 2;
                x >>= 1;
        }
        return o;
}

struct Implicant {
	unsigned int implicant;
	unsigned int dont_care_mask;
	vector<unsigned int> minterms;
	int ones;
	bool used;

	Implicant (int vars=0, unsigned int i=0, vector<unsigned int> min_list = vector<unsigned int>(), unsigned int mask = 0, bool u = false)
		: dont_care_mask(mask), ones(0), used(false)
	{
		if (min_list.empty()) minterms.push_back(i);
		else minterms = min_list;

		// set any dont care bits to 0
		implicant = i & (mask ^ ((1 << vars) - 1));

		ones = count_ones(implicant);
	}

	/*
        int implicant;
        string minterms;
        vector<int> mints;
        int mask;
        string bits;
        int ones;
        bool used;
        Implicant(int i = 0, vector<int> min = vector<int>(), string t = "", int m = 0, bool u = false)
                : implicant(i), mask(m), ones(0), used(u)
        {
                if (t == "") {
                        stringstream ss;
                        ss << 'm' << i;
                        minterms = ss.str();
                }
                else minterms = t;
                if (min.empty()) mints.push_back(i);
                else mints = min;
                int bit = 1 << vars;
                while (bit >>= 1)
                        if (m & bit) bits += '-';
                        else if (i & bit) { bits += '1'; ++ones; }
                        else bits += '0';
        }
        bool operator<(const Implicant& b) const { return ones < b.ones; }
        vector<int> cat(const Implicant &b) {
                vector<int> v = mints;
                v.insert(v.end(), b.mints.begin(), b.mints.end());
                return v;
        }
        friend ostream &operator<<(ostream &out, const Implicant &im);
		*/
};

vector<Implicant> reduce_table(int var_count, vector<unsigned int> minterms)
{
	sort(minterms.begin(), minterms.end());
	minterms.erase(unique(minterms.begin(), minterms.end()),minterms.end());

	vector<Implicant> implicants;
	for (size_t i = 0; i < minterms.size(); i++)
		implicants.push_back(Implicant(var_count, minterms[i]));

	vector<Implicant> aux;
	vector<Implicant> primes;
	while (implicants.size() > 1)
	{
		for (size_t i = 0; i < implicants.size() - 1; i++)
		{
			for (size_t j = implicants.size() - 1; j > i; j--)
			{
				if (implicants[i].dont_care_mask == implicants[j].dont_care_mask && implicants[i].implicant == implicants[j].implicant)
				{
					// remove the duplicate?
					implicants.erase(implicants.begin() + j);
				}
			}
		}

		aux.clear();
		for (size_t i = 0; i < implicants.size() - 1; i++)
		{
			for (size_t j = i + 1; j < implicants.size(); j++)
			{
				// If the second has only one more 1, and the masks match, and the implicants only differ by one bit
				if (implicants[j].ones == implicants[i].ones + 1 &&
					implicants[j].dont_care_mask == implicants[i].dont_care_mask &&
					count_ones(implicants[i].implicant ^ implicants[j].implicant) == 1)
				{
					// Mark both as used
					implicants[i].used = true;
					implicants[j].used = true;

					// Concatenate their minterms
					vector<unsigned int> min_list(implicants[i].minterms);
					min_list.insert(min_list.end(),implicants[j].minterms.begin(),implicants[j].minterms.end());

					// Make a new implicant based on those minterms, with the don't care mask updated
					aux.push_back(Implicant(
						var_count,
						implicants[i].implicant,
						min_list,
						(implicants[i].implicant ^ implicants[j].implicant) | implicants[i].dont_care_mask
					));
				}
			}
		}

		// We've done all we can with that list. If any are unused they are prime
		for (size_t i = 0; i < implicants.size(); i++)
		{
			if (!implicants[i].used) primes.push_back(implicants[i]);
		}
		implicants = aux;
		// sort the implicants?

	}

	for (size_t i = 0; i < implicants.size(); i++)
	{
		if (!implicants[i].used) primes.push_back(implicants[i]);
	}



	// primes is now filled



	return primes;
}