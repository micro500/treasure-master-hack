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
};

