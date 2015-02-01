#include <stdio.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>

#include "data_sizes.h"
#include "rng.h"
#include "working_code.h"
#include "key_schedule.h"
#include "verify.h"

int main(void)
{
	generate_rng_table();

	/*
	unsigned char working_code[128];

	// For the known working code, the payload is the following:
	working_code[0] = 0x2c;
	working_code[1] = 0xa5;
	working_code[2] = 0xb4;
	working_code[3] = 0x2d;
	working_code[4] = 0xf7;
	working_code[5] = 0x3a;
	working_code[6] = 0x26;
	working_code[7] = 0x12;
	*/

	int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11};

	uint8 value[8];

	// For the known working code, the payload is the following:
	value[0] = 0x2c;
	value[1] = 0xa5;
	value[2] = 0xb4;
	value[3] = 0x2d;
	value[4] = 0xf7;
	value[5] = 0x3a;
	value[6] = 0x26;
	value[7] = 0x12;

	key_schedule_data schedule_data;
	schedule_data.as_uint8[0] = value[0];
	schedule_data.as_uint8[1] = value[1];
	schedule_data.as_uint8[2] = value[2];
	schedule_data.as_uint8[3] = value[3];

	key_schedule_entry schedule_entries[27];

	int schedule_counter = 0;
	for (int i = 0; i < 26; i++)
	{
		schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data);

		if (map_list[i] == 0x22)
		{
			schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data,4);
		}
	}

	std::vector<working_code> code_list(1,value);

	/*
	for (int i = 1; i < 0x1000000; i++)
	{
		value[5] = (i >> 16) & 0xFF;
		value[6] = (i >> 8) & 0xFF;
		value[7] = i & 0xFF;
		code_list.push_back(value);
	}
	*/

	schedule_counter = 0;

	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;
	int blah = 0;
	uint64 full_sum = 0;
	for (int i = 0; i < 26; i++)
	{
		// Step through the vector and do the map exit on each entry
		uint64 sum = 0;
		for (std::vector<working_code>::iterator it = code_list.begin(); it != code_list.end(); ++it)
		{
			auto start = clock::now();
			printf("map: %02X\n",map_list[i]);
			printf("rng1: %02X, rng2: %02X, nibble: %04X\t<--NEW\n",schedule_entries[schedule_counter].rng1,schedule_entries[schedule_counter].rng2,schedule_entries[schedule_counter].nibble_selector);
		
			it->process_map_exit(map_list[i],schedule_entries[schedule_counter]);
		
			if (map_list[i] == 0x22)
			{
				printf("rng1: %02X, rng2: %02X, nibble: %04X\t<--NEW\n",schedule_entries[schedule_counter+1].rng1,schedule_entries[schedule_counter+1].rng2,schedule_entries[schedule_counter+1].nibble_selector);
			
				it->process_map_exit(map_list[i],schedule_entries[schedule_counter+1]);
			}
			auto end = clock::now();
			sum += duration_cast<microseconds>(end-start).count();
		}

		schedule_counter++;
		if (map_list[i] == 0x22)
		{
			schedule_counter++;
		}

		std::cout << code_list.size() << " run\n";
		std::cout << sum << "us\n";
		full_sum += sum;
		
		blah++;

		size_t x = code_list.size();
		auto start = clock::now();

		std::sort(code_list.begin(), code_list.end());
		code_list.erase(std::unique(code_list.begin(),code_list.end()),code_list.end());

		auto end = clock::now();
		x -= code_list.size();
		sum = duration_cast<microseconds>(end-start).count();
		full_sum += sum;
		std::cout << x << " deleted\n";
		std::cout << sum << " us\n";

		printf("\n");
	}

	code_list.begin()->display_working_code();

	uint8 * decrypted_memory = decrypt_memory(code_list.begin()->working_code_data.as_uint8,carnival_code,carnival_code_length);
	if (verify_checksum(decrypted_memory,carnival_code_length))
	{
		printf("GOOD\n");
	}

	//check_carnival_code(code_list.begin()->working_code_data.as_uint8);
	printf("\n");


	std::cout << full_sum << "\n";

	int x = 0;
	
	std::cout << code_list.size() << "\n";

	return 0;

	/*
	working_code[0] = 0x00;
	working_code[1] = 0x00;
	working_code[2] = 0x00;
	working_code[3] = 0x00;
	working_code[4] = 0x00;
	working_code[5] = 0x00;
	working_code[6] = 0x00;
	working_code[7] = 0x00;
	*/
	/*
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;

	auto sum = 0;
	auto full_sum = 0;

	for (int i = 0; i < 0xFFFF; i++)
	{
		working_code[6] = (i & 0xFF00) >> 8;
		working_code[7] = i & 0xFF;
		auto start = clock::now();
		process_code(working_code);
		check_carnival_code(working_code);
		auto end = clock::now();
		
		sum += duration_cast<microseconds>(end-start).count();
		if (i % 0x100 == 0)
		{
			std::cout << i << " " << sum << "ms\n";
			full_sum += sum;
			sum = 0;
		}
	}

	std::cout << full_sum / 256 << "\n";
	*/
	//display_working_code(working_code);
}
