#include <stdio.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>

#include "boinc.h"

#include "data_sizes.h"
#include "cmd_args.h"
#include "rng.h"
#include "working_code.h"
#include "key_schedule.h"
#include "verify.h"

int main(int argc, char **argv)
{
	// Usage:
	//  -k, --key "XXXXXXXX": supply 8 hex values to use as the key value, defaults to all 0's?
	//  -d, --data "XXXXXXXX": supply 8 hex values to use as the starting data value, defaults to all 0's
	//  -c, --count 
	//  -a, --attack attack_type:
	//               "individual": each IV is processed and checked individually
	//               "vector":     each working code is added to a std::vector, processed one round, sorted, then std::unique'd
	//               "hash":       each working code is processed a round, then checked against a hash table. collisions are not processed any further
	//  -b, --big-registers: use 128bit registers as an optimization
	//  -s, --key-schedule: pre-process the key schedule as an optimization instead of processing it for every IV
	//  -f, --file filename: read a list of IVs to process from a file. Overrides -k and -d

	int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11};

	initialize_boinc();

	generate_rng_table();

	// Process command line
	process_command_line(argc, argv);


	uint8 value[8];

	if (command_line_options.from_file)
	{
		std::ifstream myfile;
		myfile.open(command_line_options.filename, std::ios::in);
		if (myfile.is_open())
		{
			std::string line;
			while (getline(myfile, line))
			{
				std::stringstream ss;
				ss << std::hex << line.substr(0,8);

				uint32 key;
				ss >> key;

				std::stringstream ss2;
				ss2 << std::hex << line.substr(8,16);

				uint32 data;
				ss2 >> data;

				// something about skipping bad lines?

				value[0] = (key >> 24) & 0xFF;
				value[1] = (key >> 16) & 0xFF;
				value[2] = (key >> 8) & 0xFF;
				value[3] = key & 0xFF;

				value[4] = (data >> 24) & 0xFF;
				value[5] = (data >> 16) & 0xFF;
				value[6] = (data >> 8) & 0xFF;
				value[7] = data & 0xFF;

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


				working_code in_progress(value);

				schedule_counter = 0;

				for (int map_index = 0; map_index < 26; map_index++)
				{
					in_progress.process_map_exit(map_list[map_index],schedule_entries[schedule_counter]);

					if (map_list[map_index] == 0x22)
					{
						in_progress.process_map_exit(map_list[map_index],schedule_entries[schedule_counter+1]);
					}

					schedule_counter++;
					if (map_list[map_index] == 0x22)
					{
						schedule_counter++;
					}
				}


				boinc_log("%02X%02X%02X%02X%02X%02X%02X%02X\n",value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7]);

				if (compare_working_code(in_progress.working_code_data.as_uint8, carnival_world_working_code))
				{
					boinc_log("wcm");
				}

				boinc_log("\t");

				uint8 * decrypted_memory = decrypt_memory(in_progress.working_code_data.as_uint8,carnival_code,carnival_code_length);

				/*
				for (int i = 0; i < 128; i++)
				{
					printf("%02X ",decrypted_memory[i]);
				}
				*/
				
				if (verify_checksum(decrypted_memory,carnival_code_length))
				{
					boinc_log("car");

				}

				boinc_log("\t");
				
				if (compare_working_code(decrypted_memory, carnival_code_decrypted_machine_code))
				{
					boinc_log("mcm");
				}
				boinc_log("\t");

				delete[] decrypted_memory;

				decrypted_memory = decrypt_memory(in_progress.working_code_data.as_uint8,other_world_code,other_world_code_length);
				if (verify_checksum(decrypted_memory,other_world_code_length))
				{
					boinc_log("oth");
				}
				delete[] decrypted_memory;

				boinc_log("\n");

				//fraction_done(((double)count)/((double)ivs_from_file.size()));
			}
		}
		else
		{
			// ERROR
			boinc_log("File open error\n");
		}
	}
	else
	{

		value[0] = (command_line_options.start_key >> 24) & 0xFF;
		value[1] = (command_line_options.start_key >> 16) & 0xFF;
		value[2] = (command_line_options.start_key >> 8) & 0xFF;
		value[3] = command_line_options.start_key & 0xFF;

		value[4] = (command_line_options.start_data >> 24) & 0xFF;
		value[5] = (command_line_options.start_data >> 16) & 0xFF;
		value[6] = (command_line_options.start_data >> 8) & 0xFF;
		value[7] = command_line_options.start_data & 0xFF;

		printf("Starting IV: %02X %02X %02X %02X %02X %02X %02X %02X\n",value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7]);

		uint64 count = 1;

		count = command_line_options.iv_count;

		printf("Will run %i IVs\n",count);		

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

		/*
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
		*/

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

		//std::vector<working_code> code_list(1,value);

		/*
		for (int i = 1; i < 0x1000000; i++)
		{
			value[5] = (i >> 16) & 0xFF;
			value[6] = (i >> 8) & 0xFF;
			value[7] = i & 0xFF;
			code_list.push_back(value);
		}
		*/

	

		/*
		using std::chrono::duration_cast;
		using std::chrono::microseconds;
		typedef std::chrono::high_resolution_clock clock;
		int blah = 0;
		uint64 full_sum = 0;
		*/

		for (int IV = 0; IV < count; IV++)
		{
			working_code in_progress(value);

			schedule_counter = 0;

			for (int map_index = 0; map_index < 26; map_index++)
			{
				/*
				// Step through the vector and do the map exit on each entry
				uint64 sum = 0;
				for (std::vector<working_code>::iterator it = code_list.begin(); it != code_list.end(); ++it)
				{
					auto start = clock::now();
				*/

				/*
				printf("map: %02X\n",map_list[i]);
				printf("rng1: %02X, rng2: %02X, nibble: %04X\t<--NEW\n",schedule_entries[schedule_counter].rng1,schedule_entries[schedule_counter].rng2,schedule_entries[schedule_counter].nibble_selector);
				*/
		
				in_progress.process_map_exit(map_list[map_index],schedule_entries[schedule_counter]);

				/*
				it->process_map_exit(map_list[i],schedule_entries[schedule_counter]);
				*/

				if (map_list[map_index] == 0x22)
				{
					/*
					printf("rng1: %02X, rng2: %02X, nibble: %04X\t<--NEW\n",schedule_entries[schedule_counter+1].rng1,schedule_entries[schedule_counter+1].rng2,schedule_entries[schedule_counter+1].nibble_selector);
					*/
			
					in_progress.process_map_exit(map_list[map_index],schedule_entries[schedule_counter+1]);

					/*
					it->process_map_exit(map_list[i],schedule_entries[schedule_counter+1]);
					*/
				}

				/*
					auto end = clock::now();
					sum += duration_cast<microseconds>(end-start).count();
				}
				*/

				schedule_counter++;
				if (map_list[map_index] == 0x22)
				{
					schedule_counter++;
				}

				/*
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
				*/
			}

			/*
			for (int i = 0; i < 128; i++)
			{
				out.printf("%02X ",in_progress.working_code_data.as_uint8[i]);
			}
			out.printf("\n");
			*/

			uint8 * decrypted_memory = decrypt_memory(in_progress.working_code_data.as_uint8,carnival_code,carnival_code_length);
			if (verify_checksum(decrypted_memory,carnival_code_length))
			{
				boinc_log("%02X %02X %02X %02X %02X %02X %02X %02X\n",value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7]);
				boinc_log("GOOD!\n");
			}

			delete[] decrypted_memory;

			fraction_done(((double)IV)/((double)count));

			// Increment to the next IV
			value[7]++;
			if (value[7] == 0x00)
			{
				for (int i = 6; i >= 0; i--)
				{
					value[i]++;
					if (value[i] != 0x00)
					{
						break;
					}
				}
			}
		}
	}

	finish_boinc();


	/*
	code_list.begin()->display_working_code();

	uint8 * decrypted_memory = decrypt_memory(code_list.begin()->working_code_data.as_uint8,carnival_code,carnival_code_length);
	if (verify_checksum(decrypted_memory,carnival_code_length))
	{
		printf("GOOD\n");
	}

	//check_carnival_code(code_list.begin()->working_code_data.as_uint8);
	printf("\n");
	*/

	/*
	std::cout << full_sum << "\n";

	int x = 0;
	
	std::cout << code_list.size() << "\n";
	*/
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
