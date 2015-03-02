#include <vector>
#include <algorithm>

#include "data_sizes.h"
#include "rng.h"

// Inverse RNG loop tables
uint8 rng_X[4114];
uint8 rng_Y[13476];
uint8 rng_Z0[35200];
uint8 rng_Z1[20934];

uint16 rng_table[0x100][0x100];
std::vector<std::vector<uint16>> back_rng(0x10000);

// At least 0x10000 entries, but there will be more than that
std::vector<reverse_rng_seed_info> reverse_rng(0);

bool reverse_rng_seed_sort(reverse_rng_seed_info a, reverse_rng_seed_info b)
{
	if (a.table_length < b.table_length)
	{
		return true;
	}
	else if (a.table_length == b.table_length)
	{
		if (a.rng1 < b.rng1)
		{
			return true;	
		} 
		else if (a.rng1 == b.rng1)
		{
			if (a.rng2 < b.rng2)
			{
				return true;	
			}
			else if (a.rng2 == b.rng2)
			{
				return a.branch_option < b.branch_option;
			}
		}
	}
	return false;
}

unsigned char rng_real(unsigned char *rng1, unsigned char *rng2)
{
	int rngA = *rng1;
	int rngB = *rng2;	
	// LDA $0436
    // CLC
	unsigned char carry = 0;
    // ADC $0437
    // STA $0437
	// Basically, add rng1 and rng2 together w/o carry and store it to rng2
	rngB = (unsigned char)(rngB + rngA);

	// LDA #$89
    // CLC
    // ADC $0436
	// STA $0436
	// Basically, add #$89 to rng1, and remember the carry for the next addition
	rngA = rngA + 0x89;
	if (rngA > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngA = (unsigned char)rngA;

    // LDA #$2A
    // ADC $0437 = #$AE
    // STA $0437 = #$AE
	rngB = rngB + 0x2A + carry;
	if (rngB > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngB = (unsigned char)rngB;

    // LDA $0436 = #$71
    // ADC #$21
    // STA $0436 = #$71
	rngA = rngA + 0x21 + carry;
	if (rngA > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngA = (unsigned char)rngA;

    // LDA $0437 = #$AE
    // ADC #$43
    // STA $0437 = #$AE
	rngB = rngB + 0x43 + carry;
	if (rngB > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngB = (unsigned char)rngB;

    // EOR $0436 = #$71
	*rng1 = (unsigned char)rngA;
	*rng2 = (unsigned char)rngB;

	return *rng1 ^ *rng2;
}

void generate_rng_table()
{
	for (int i = 0; i < 0x100; i++)
	{
		for (int j = 0; j < 0x100; j++)
		{
			unsigned char rng1 = i;
			unsigned char rng2 = j;
			rng_real(&rng1,&rng2);
			rng_table[i][j] = rng1 << 8 | rng2;
		}
	}

	for (int i = 0; i < 0x100; i++)
	{
		for (int j = 0; j < 0x100; j++)
		{
			uint16 result = rng_table[i][j];
			uint8 rng1 = (result >> 8) & 0xFF;
			uint8 rng2 = result & 0xFF;


			uint16 from = ((i & 0xFF) << 8) | (j & 0xFF);
			uint16 to   = ((rng1 & 0xFF) << 8) | (rng2 & 0xFF);
			back_rng[to].push_back(from);
		}
	}

	reverse_rng.reserve(0x10800);

	// To generate X table:
	// Seed rng to 000F
	RNG temp(0x00,0x0F);

	int entries = 0;

	for (int i = 0; i < 0x10000; i++)
	{
		// store current value in table
		// mark location we put it in
		reverse_rng_seed_info current;

		current.rng1 = temp.rng1;
		current.rng2 = temp.rng2;
		current.branch_option = 0;
		rng_X[entries] = temp.rng1 ^ temp.rng2;
		current.table = &rng_X[entries];
		current.useful_bytes = 0;
		current.table_length = 2048;

		reverse_rng.push_back(current);

		entries++;

		// Go back a step in the RNG
		temp.run_back(0);
		if (temp.rng1 == 0x00 && temp.rng2 == 0x0F)
		{
			// We hit the start
			// the value before the run_back was the last one in this loop before 0x000F
			// we need that entry, plus 2047 after it
			for (int j = 0; j < 2047; j++)
			{
				// store current value in table
				// DO NOT mark location
				rng_X[entries] = temp.rng1 ^ temp.rng2;
				temp.run_back(0);
				entries++;
			}

			break;
		}

	}
	printf("loop X: %i\n",entries);


	// To generate Y table:
	// Seed rng to 0004
	temp.seed(0x00,0x04);

	entries = 0;

	for (int i = 0; i < 0x10000; i++)
	{
		// store current value in table
		// mark location we put it in
		reverse_rng_seed_info current;

		current.rng1 = temp.rng1;
		current.rng2 = temp.rng2;
		current.branch_option = 0;
		rng_Y[entries] = temp.rng1 ^ temp.rng2;
		current.table = &rng_Y[entries];
		current.useful_bytes = 0;
		current.table_length = 2048;

		reverse_rng.push_back(current);

		entries++;

		// Go back a step in the RNG
		temp.run_back(0);
		if (temp.rng1 == 0x00 && temp.rng2 == 0x04)
		{
			// We hit the start
			// the value before the run_back was the last one in this loop before 0x000F
			// we need that entry, plus 2047 after it
			for (int j = 0; j < 2047; j++)
			{
				// store current value in table
				// DO NOT mark location
				rng_Y[entries] = temp.rng1 ^ temp.rng2;
				temp.run_back(0);
				entries++;
			}

			break;
		}

	}
	printf("loop Y: %i\n",entries);


	// To generate Z0 table:
	// Seed rng to 0000
	temp.seed(0x00,0x00);

	entries = 0;

	for (int i = 0; i < 0x10000; i++)
	{
		// store current value in table
		// mark location we put it in
		reverse_rng_seed_info current;

		current.rng1 = temp.rng1;
		current.rng2 = temp.rng2;
		current.branch_option = 0;
		rng_Z0[entries] = temp.rng1 ^ temp.rng2;
		current.table = &rng_Z0[entries];
		current.useful_bytes = 0;
		current.table_length = 2048;

		reverse_rng.push_back(current);

		entries++;

		if (temp.rng1 == 0xAA && temp.rng2 == 0x6D)
		{
			printf("test\n");
		}

		// Go back a step in the RNG
		temp.run_back(0);
		if (temp.rng1 == 0x00 && temp.rng2 == 0x00)
		{
			// We hit the start
			// the value before the run_back was the last one in this loop before 0x000F
			// we need that entry, plus 2047 after it
			for (int j = 0; j < 2047; j++)
			{
				// store current value in table
				// DO NOT mark location
				rng_Z0[entries] = temp.rng1 ^ temp.rng2;
				temp.run_back(0);
				entries++;
			}

			break;
		}

	}
	printf("loop Z0: %i\n",entries);

	std::vector<int> table_lengths(0x10000);
	for (int i = 0; i < 0x10000; i++)
	{
		table_lengths[i] = 2048;
	}

	temp.seed(0x21,0x43);

	for (int i = 0; i < 2048; i++)
	{
		table_lengths[(temp.rng1 << 8) | temp.rng2] = i;
		temp.run();
	}

	// To generate Z1 table:
	// Seed to AAE1
	temp.seed(0xAA,0xE1);

	entries = 0;

	for (int i = 0; i < 0x10000; i++)
	{
		// store current value in table
		// mark location we put it in
		//printf("inserting value for: %02X %02X\t useful only for %i executions\n",temp.rng1,temp.rng2,(i < 2047 ? 2048-i : 0));
		reverse_rng_seed_info current;

		current.rng1 = temp.rng1;
		current.rng2 = temp.rng2;
		current.branch_option = 1;
		rng_Z1[entries] = temp.rng1 ^ temp.rng2;
		current.table = &rng_Z1[entries];
		current.useful_bytes = (i < 2048 ? 2048-i : 0);
		// 0 for 2143
		// 1 for the value before that, etc
		current.table_length = table_lengths[(temp.rng1 << 8) | temp.rng2]; // ???

		reverse_rng.push_back(current);

		entries++;

		// Run back using branch 1
		temp.run_back(1);

		// Check if we hit the entry seed
		if (temp.rng1 == 0x21 && temp.rng2 == 0x43)
		{
			// Don't store it. There is no way to get to the entry seed, so there is no 0x21 ^ 0x43 value to put in the table
			break;
		}
		/*
		if (temp.rng1 == 0xAA && temp.rng2 == 0x6D)
		{
			printf("Test\n");
		}
		*/
	}
	printf("Z1: %i\n",entries);

	// Insert the 0x2143 node
	{
	reverse_rng_seed_info current;

	current.rng1 = 0x21;
	current.rng2 = 0x43;
	current.branch_option = 1;
	current.table = NULL;
	current.useful_bytes = -1;
	// 0 for 2143
	current.table_length = 0;

	reverse_rng.push_back(current);
	}

	std::sort(reverse_rng.begin(), reverse_rng.end(), reverse_rng_seed_sort);

	/*
	for (size_t i = 0; i < reverse_rng.size(); i++)
	{
		printf("%02X %02X %i: %i\n",reverse_rng[i].rng1,reverse_rng[i].rng2,reverse_rng[i].branch_option,reverse_rng[i].table_length);
	}
	*/

	/*
	// loop through all possible seed endings
	// take seed
	// get current value
	// run back, get previous value
	// compare to given table
	for (int i = 0; i < reverse_rng.size(); i++)
	{
		printf("checking: %02X %02X %i %i\n",reverse_rng[i].rng1,reverse_rng[i].rng2,reverse_rng[i].branch_option,reverse_rng[i].table_length);

		temp.seed(reverse_rng[i].rng1,reverse_rng[i].rng2);

		printf("Comparing %i values\n",reverse_rng[i].table_length);
		for (int j = 0; j < reverse_rng[i].table_length; j++)
		{
			uint8 val = temp.rng1 ^ temp.rng2;
			if (val != reverse_rng[i].table[j])
			{
				printf("Compare fail at %i, %02X != %02X\n",j, val, reverse_rng[i].table[j]);
			}

			temp.run_back(reverse_rng[i].branch_option);
		}
	}
	*/

	// RNG sucks.

	// X: 000F->(000F + extra)
	//   loop contains 2067 seeds

	// Y: 0004->(0004 + extra)
	//   loop contains 11429 seeds

	// Z0: 0000->(0000 + extra)
	//   loop contains 33153 seed

	// Z1: 2143->54F9 (54F9 = AA6D + extra)
	//   2132->AA6D inclusive contains 18888 seeds
	// Z1 will contain (18886+2046) = 20932 entries
	//  54F9 was chosen because it is 2046(?) past the branch
	//  if you went too far past, running back would end up at the branch, and it would be a waste because it would be identical to Z0

	// 2067 + 11429 + 33153 + 18888 = 65537 (AA6D is counted twice)

}
	
uint8 RNG::run()
{
	uint16 result = rng_table[rng1][rng2];
	rng1 = (result >> 8) & 0xFF;
	rng2 = result & 0xFF;

	return rng1 ^ rng2;
}

void RNG::seed(uint8 a, uint8 b)
{
	rng1 = a;
	rng2 = b;
}

uint8 RNG::run_back(int index)
{
	uint16 from = ((rng1 & 0xFF) << 8) | (rng2 & 0xFF);
	if (index >= back_rng[from].size())
	{
		index = 0;
		//error = true;
		//return 0xFF;
	}

	if (back_rng[from].size() == 0)
	{
		error = true;
		return 0xFF;
	}

	uint16 result = back_rng[from][index];
	rng1 = (result >> 8) & 0xFF;
	rng2 = result & 0xFF;

	error = false;

	return rng1 ^ rng2;
}