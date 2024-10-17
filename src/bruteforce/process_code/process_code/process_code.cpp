#include <stdio.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <unordered_set>

#include "boinc.h"

#include "data_sizes.h"
#include "cmd_args.h"
#include "rng.h"
#include "working_code.h"
#include "key_schedule.h"
#include "verify.h"
#include "MurmurHash3_wrapper.h"

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

	uint64 IVs_to_run = command_line_options.iv_count;

	uint64 IVs_finished = 0;

	std::ifstream myfile;
	if (command_line_options.from_file)
	{
		myfile.open(command_line_options.filename, std::ios::in);
		if (!myfile.is_open())
		{
			boinc_log("File open error\n");

			return 1;
		}
	}

	if (command_line_options.attack == INDIVIDUAL)
	{
		while (true)
		{
			// Get a key
			if (command_line_options.from_file)
			{
				std::string line;
				if (!getline(myfile, line))
				{
					break;
				}

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
			}
			else
			{
				if (IVs_to_run == IVs_finished)
				{
					break;
				}

				if (IVs_finished == 0)
				{
					value[0] = (command_line_options.start_key >> 24) & 0xFF;
					value[1] = (command_line_options.start_key >> 16) & 0xFF;
					value[2] = (command_line_options.start_key >> 8) & 0xFF;
					value[3] = command_line_options.start_key & 0xFF;

					value[4] = (command_line_options.start_data >> 24) & 0xFF;
					value[5] = (command_line_options.start_data >> 16) & 0xFF;
					value[6] = (command_line_options.start_data >> 8) & 0xFF;
					value[7] = command_line_options.start_data & 0xFF;
				}
				else
				{
					// Increment to the next IV
					for (int i = 7; i >= 0; i--)
					{
						value[i]++;
						if (value[i] != 0x00)
						{
							break;
						}
					}
				}
			}

			// Pre-process the key schedule
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

			in_progress.display_working_code();

			output_stats(&in_progress);

			IVs_finished++;

			if (command_line_options.from_file)
			{
				// TODO
			}
			else
			{
				fraction_done(((double)IVs_finished)/((double)IVs_to_run));
			}
		}
	}
	else if (command_line_options.attack == VECTOR)
	{
		boinc_log("vector attack\n");
		// will force a pre-processed key schedule for now

		std::vector<working_code> IVs_in_progress;

		key_schedule_entry schedule_entries[27];

		while (true)
		{
			// Get a key
			if (command_line_options.from_file)
			{
				std::string line;
				if (!getline(myfile, line))
				{
					break;
				}

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
			}
			else
			{
				if (IVs_to_run == IVs_finished)
				{
					break;
				}

				if (IVs_finished == 0)
				{
					value[0] = (command_line_options.start_key >> 24) & 0xFF;
					value[1] = (command_line_options.start_key >> 16) & 0xFF;
					value[2] = (command_line_options.start_key >> 8) & 0xFF;
					value[3] = command_line_options.start_key & 0xFF;

					value[4] = (command_line_options.start_data >> 24) & 0xFF;
					value[5] = (command_line_options.start_data >> 16) & 0xFF;
					value[6] = (command_line_options.start_data >> 8) & 0xFF;
					value[7] = command_line_options.start_data & 0xFF;
				}
				else
				{
					// Increment to the next IV
					for (int i = 7; i >= 0; i--)
					{
						value[i]++;
						if (value[i] != 0x00)
						{
							break;
						}
					}
				}
			}

			// Get the key from the first IV
			if (IVs_finished == 0)
			{
				key_schedule_data schedule_data;
				// Pre-process the key schedule
				schedule_data.as_uint8[0] = value[0];
				schedule_data.as_uint8[1] = value[1];
				schedule_data.as_uint8[2] = value[2];
				schedule_data.as_uint8[3] = value[3];

				int schedule_counter = 0;
				for (int i = 0; i < 26; i++)
				{
					schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data);

					if (map_list[i] == 0x22)
					{
						schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data,4);
					}
				}
			}

			IVs_in_progress.push_back(value);

			IVs_finished++;
		}

		int schedule_counter = 0;

		for (int map_index = 0; map_index < 26; map_index++)
		{
			uint64 count = 0;
			for (std::vector<working_code>::iterator it = IVs_in_progress.begin(); it != IVs_in_progress.end(); ++it)
			{
				it->process_map_exit(map_list[map_index],schedule_entries[schedule_counter]);

				if (map_list[map_index] == 0x22)
				{
					it->process_map_exit(map_list[map_index],schedule_entries[schedule_counter+1]);
				}

				count++;

				if (command_line_options.from_file)
				{
					// TODO
				}
				else
				{
					// TODO
					fraction_done((((double)count)+((double)(map_index*command_line_options.iv_count)))/(((double)26)*((double)command_line_options.iv_count)));
				} 
			}

			// Advance the schedule
			schedule_counter++;
			if (map_list[map_index] == 0x22)
			{
				schedule_counter++;
			}

			boinc_log("%lu run\n",IVs_in_progress.size());

			size_t x = IVs_in_progress.size();
			std::sort(IVs_in_progress.begin(), IVs_in_progress.end());
			IVs_in_progress.erase(std::unique(IVs_in_progress.begin(),IVs_in_progress.end()),IVs_in_progress.end());
			// shrink to fit
			std::vector<working_code>(IVs_in_progress).swap(IVs_in_progress);
			x -= IVs_in_progress.size();
			boinc_log("%lu deleted\n",x);

			boinc_log("\n");
		}

		boinc_log("%lu checking\n\n",IVs_in_progress.size());

		for (std::vector<working_code>::iterator it = IVs_in_progress.begin(); it != IVs_in_progress.end(); ++it)
		{
			output_stats(&(*it));
		}
	}
	else if (command_line_options.attack == VECTOR_CONSTANT_SIZE)
	{
		boinc_log("vector attack, constant size of %d\n", command_line_options.vector_size);
		// will force a pre-processed key schedule for now

		std::vector<working_code> IVs_in_progress;

		key_schedule_entry schedule_entries[27];

		int schedule_counter = 0;

		bool no_more = false;

		// Go one map at a time
		for (int map_index = 0; map_index < 26; map_index++)
		{
			printf("vector size before add: %lu\n",IVs_in_progress.size());

			// Add more to the vector if there are not enough
			while (IVs_in_progress.size() < command_line_options.vector_size && !no_more)
			{
				std::vector<working_code> new_IVs;
				printf("want: %lu\n", command_line_options.vector_size - IVs_in_progress.size());
				while ((IVs_in_progress.size() + new_IVs.size()) < command_line_options.vector_size)
				{
					// Get a key
					if (command_line_options.from_file)
					{
						std::string line;
						if (!getline(myfile, line))
						{
							no_more = true;
							break;
						}

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
					}
					else
					{
						if (IVs_to_run == IVs_finished)
						{
							no_more = true;
							break;
						}

						if (IVs_finished == 0)
						{
							value[0] = (command_line_options.start_key >> 24) & 0xFF;
							value[1] = (command_line_options.start_key >> 16) & 0xFF;
							value[2] = (command_line_options.start_key >> 8) & 0xFF;
							value[3] = command_line_options.start_key & 0xFF;

							value[4] = (command_line_options.start_data >> 24) & 0xFF;
							value[5] = (command_line_options.start_data >> 16) & 0xFF;
							value[6] = (command_line_options.start_data >> 8) & 0xFF;
							value[7] = command_line_options.start_data & 0xFF;
						}
						else
						{
							// Increment to the next IV
							for (int i = 7; i >= 0; i--)
							{
								value[i]++;
								if (value[i] != 0x00)
								{
									break;
								}
							}
						}
					}

					// Get the key from the first IV
					if (IVs_finished == 0)
					{
						key_schedule_data schedule_data;
						// Pre-process the key schedule
						schedule_data.as_uint8[0] = value[0];
						schedule_data.as_uint8[1] = value[1];
						schedule_data.as_uint8[2] = value[2];
						schedule_data.as_uint8[3] = value[3];

						int schedule_counter = 0;
						for (int i = 0; i < 26; i++)
						{
							schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data);

							if (map_list[i] == 0x22)
							{
								schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data,4);
							}
						}
					}

					new_IVs.push_back(value);

					IVs_finished++;
				}
				printf("new IVs: %lu\n",new_IVs.size());

				// catch the new IVs up
				if (new_IVs.size() > 0)
				{
					int sub_schedule_counter = 0;
					for (int sub_map_index = 0; sub_map_index < map_index; sub_map_index++)
					{

						for (std::vector<working_code>::iterator it = new_IVs.begin(); it != new_IVs.end(); ++it)
						{
							it->process_map_exit(map_list[sub_map_index],schedule_entries[sub_schedule_counter]);

							if (map_list[sub_map_index] == 0x22)
							{
								it->process_map_exit(map_list[sub_map_index],schedule_entries[sub_schedule_counter+1]);
							}
						} 

						// Advance the schedule
						sub_schedule_counter++;
						if (map_list[sub_map_index] == 0x22)
						{
							sub_schedule_counter++;
						}

						boinc_log("%lu subrun\n",new_IVs.size());

						size_t x = new_IVs.size();
						std::sort(new_IVs.begin(), new_IVs.end());
						new_IVs.erase(std::unique(new_IVs.begin(),new_IVs.end()),new_IVs.end());
						// shrink to fit
						std::vector<working_code>(new_IVs).swap(new_IVs);
						x -= new_IVs.size();
						boinc_log("%lu subdeleted\n",x);
					}
				}
				printf("new IVs after: %lu\n",new_IVs.size());
				IVs_in_progress.insert(IVs_in_progress.end(), new_IVs.begin(), new_IVs.end());
				new_IVs.empty();

				printf("vector size after add: %lu\n",IVs_in_progress.size());

				std::vector<working_code>(IVs_in_progress).swap(IVs_in_progress);
			}
			
			//std::vector<working_code>(new_IVs).swap(new_IVs);

			printf("finished so far: %lu\n", IVs_finished);
			printf("process main vector, index: %i\n", map_index);
			uint64 count = 0;
			for (std::vector<working_code>::iterator it = IVs_in_progress.begin(); it != IVs_in_progress.end(); ++it)
			{
				it->process_map_exit(map_list[map_index],schedule_entries[schedule_counter]);

				if (map_list[map_index] == 0x22)
				{
					it->process_map_exit(map_list[map_index],schedule_entries[schedule_counter+1]);
				}

				count++;

				if (command_line_options.from_file)
				{
					// TODO
				}
				else
				{
					// TODO
					fraction_done((((double)count)+((double)(map_index*command_line_options.iv_count)))/(((double)26)*((double)command_line_options.iv_count)));
				} 
			}

			// Advance the schedule
			schedule_counter++;
			if (map_list[map_index] == 0x22)
			{
				schedule_counter++;
			}

			boinc_log("%lu run\n",IVs_in_progress.size());

			size_t x = IVs_in_progress.size();
			std::sort(IVs_in_progress.begin(), IVs_in_progress.end());
			IVs_in_progress.erase(std::unique(IVs_in_progress.begin(),IVs_in_progress.end()),IVs_in_progress.end());
			// shrink to fit
			std::vector<working_code>(IVs_in_progress).swap(IVs_in_progress);
			x -= IVs_in_progress.size();
			boinc_log("%lu deleted\n",x);

			boinc_log("\n");
		}

		boinc_log("%lu checking\n\n",IVs_in_progress.size());

		for (std::vector<working_code>::iterator it = IVs_in_progress.begin(); it != IVs_in_progress.end(); ++it)
		{
			output_stats(&(*it));
		}

		printf("finished: %lu\n", IVs_finished);
	}
	else if (command_line_options.attack == HASH)
	{
		boinc_log("hash attack\n");
		// will force a pre-processed key schedule for now

		int skip_count = 0;

		key_schedule_entry schedule_entries[27];

		std::unordered_set<MurmurHashKey> global_hash_table;
		uint32 global_hash_count = 0;

		int schedule_counter = 0;
		while (IVs_finished < command_line_options.iv_count)
		{
			printf("\nstarting some attack\n");
			// Start a blank vector
			std::vector<working_code> IVs_in_progress;

			// go map by map	
			for (int map_index = 0; map_index < 26; map_index++)
			{
				if (map_index == 0)
				{
					std::unordered_set<MurmurHashKey> local_hash_table;
					local_hash_table.reserve(0x10000);

					printf("First round\n");
					// On first round, do a vector add
					while (IVs_in_progress.size() < command_line_options.vector_size)
					{
						// Get a key
						if (command_line_options.from_file)
						{
							std::string line;
							if (!getline(myfile, line))
							{
								break;
							}

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
						}
						else
						{
							if (IVs_to_run == IVs_finished)
							{
								break;
							}

							if (IVs_finished == 0)
							{
								value[0] = (command_line_options.start_key >> 24) & 0xFF;
								value[1] = (command_line_options.start_key >> 16) & 0xFF;
								value[2] = (command_line_options.start_key >> 8) & 0xFF;
								value[3] = command_line_options.start_key & 0xFF;

								value[4] = (command_line_options.start_data >> 24) & 0xFF;
								value[5] = (command_line_options.start_data >> 16) & 0xFF;
								value[6] = (command_line_options.start_data >> 8) & 0xFF;
								value[7] = command_line_options.start_data & 0xFF;
							}
							else
							{
								// Increment to the next IV
								for (int i = 7; i >= 0; i--)
								{
									value[i]++;
									if (value[i] != 0x00)
									{
										break;
									}
								}
							}
						}

						// Get the key from the first IV
						if (IVs_finished == 0)
						{
							key_schedule_data schedule_data;
							// Pre-process the key schedule
							schedule_data.as_uint8[0] = value[0];
							schedule_data.as_uint8[1] = value[1];
							schedule_data.as_uint8[2] = value[2];
							schedule_data.as_uint8[3] = value[3];

							int schedule_counter_gen = 0;
							for (int i = 0; i < 26; i++)
							{
								schedule_entries[schedule_counter_gen++] = generate_schedule_entry(map_list[i],&schedule_data);

								if (map_list[i] == 0x22)
								{
									schedule_entries[schedule_counter_gen++] = generate_schedule_entry(map_list[i],&schedule_data,4);
								}
							}
						}

						working_code in_progress(value);

						// run a round, check hash, add if new
						in_progress.process_map_exit(map_list[map_index],schedule_entries[schedule_counter]);

						if (map_list[map_index] == 0x22)
						{
							in_progress.process_map_exit(map_list[map_index],schedule_entries[schedule_counter+1]);
						}

						//boinc_log("%02X%02X%02X%02X%02X%02X%02X%02X\t",in_progress.starting_value[0],in_progress.starting_value[1],in_progress.starting_value[2],in_progress.starting_value[3],in_progress.starting_value[4],in_progress.starting_value[5],in_progress.starting_value[6],in_progress.starting_value[7]);

						// check hash
						MurmurHashKey hashKey = generateHashKey(in_progress);

						// check the hash?
						if (local_hash_table.count(hashKey) > 0)
						{
							// in hash
							//printf("Already in hash\n");
							//printf("%i\n",skip_count);
							skip_count++;
							//printf("%i\n",skip_count);
						}
						else
						{
							local_hash_table.insert(hashKey);
							IVs_in_progress.push_back(in_progress);
							//printf("New\n");
						}

						IVs_finished++;
					}
					printf("processed: %i\n",IVs_finished);
					printf("skipped: %i\n\n",skip_count);

					// Advance the schedule
					schedule_counter++;
					if (map_list[map_index] == 0x22)
					{
						schedule_counter++;
					}
				}
				else
				{
					uint64 count = 0;
					for (std::vector<working_code>::iterator it = IVs_in_progress.begin(); it != IVs_in_progress.end(); ++it)
					{
						it->process_map_exit(map_list[map_index],schedule_entries[schedule_counter]);

						if (map_list[map_index] == 0x22)
						{
							it->process_map_exit(map_list[map_index],schedule_entries[schedule_counter+1]);
						}

						count++;

						if (command_line_options.from_file)
						{
							// TODO
						}
						else
						{
							// TODO
							fraction_done((((double)count)+((double)(map_index*command_line_options.iv_count)))/(((double)26)*((double)command_line_options.iv_count)));
						} 
					}

					// Advance the schedule
					schedule_counter++;
					if (map_list[map_index] == 0x22)
					{
						schedule_counter++;
					}

					boinc_log("%lu run\n",IVs_in_progress.size());

					// for now do a vector redcuction
					size_t x = IVs_in_progress.size();
					std::sort(IVs_in_progress.begin(), IVs_in_progress.end());
					IVs_in_progress.erase(std::unique(IVs_in_progress.begin(),IVs_in_progress.end()),IVs_in_progress.end());
					// shrink to fit
					std::vector<working_code>(IVs_in_progress).swap(IVs_in_progress);
					x -= IVs_in_progress.size();
					boinc_log("%lu deleted\n",x);

					if (map_index == 4)
					{
						size_t x2 = IVs_in_progress.size();
						boinc_log("global reduction\n");
						// loop through IVs
						
						std::vector<working_code>::iterator it2 = IVs_in_progress.begin();
						while (it2 != IVs_in_progress.end())
						{
							//it2->display_working_code();

							// check hash
							MurmurHashKey hashKey = generateHashKey((*it2));

							// if found, remove from current working set
							if (global_hash_table.count(hashKey) > 0)
							{
								// in hash
								//printf("Already in hash\n");
								//printf("%i\n",skip_count);
								skip_count++;
								it2 = IVs_in_progress.erase(it2);

								//printf("%i\n",skip_count);
							}
							else
							{
								global_hash_table.insert(hashKey);
								global_hash_count++;
							}

							// next
							++it2;
						}
						
						printf("end?\n");
						x2 -= IVs_in_progress.size();
						boinc_log("%lu hdeleted\n",x2);
						
					}

					boinc_log("\n");
				}
			}

			boinc_log("%lu checking\n\n",IVs_in_progress.size());

			for (std::vector<working_code>::iterator it = IVs_in_progress.begin(); it != IVs_in_progress.end(); ++it)
			{
				output_stats(&(*it));
			}
		}
		printf("global hash size: %i\n",global_hash_count);
	}

	finish_boinc();

	return 0;
}
