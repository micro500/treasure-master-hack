struct rng_data
{
	// The resulting seed after advancing the RNG X number of steps
    short seed_forward_1;
	short seed_forward_128;

	// Results from the RNG simply put into an int container
	int regular_values[32];

	// Results from the RNG summated to be used for code expansion
	int expansion_values_high[32];
	int expansion_values_low[32];

	// Bitmasked values for algorithm 0
	int alg0_values[32];

	// Bitmasked values for algorithm 6
	int alg6_values[32];

	// High byte values for use with algorithms 1 and 4 (addition/subtraction)
	int arth_high_values[32];

	// Low byte values for use with algorithms 1 and 4 (addition/subtraction)
	int arth_low_values[32];

	// Bitmasked values for algorithms 2 and 5
	int alg2_value;
	int alg5_value;
};

typedef struct 
{
	unsigned short rng_seed;
	unsigned short nibble_selector;
} key_schedule_entry_new;


__kernel void tm_process(__global int* code_space, __global struct rng_data* rng_space, __global key_schedule_entry_new* key_schedule, __global int* result_space)
{
	__local unsigned int working_code1[128];
	__local unsigned int working_code2[128];
	//__local unsigned short rng_seed[4];
	//__local unsigned short nibble_selector[4];
	__local unsigned char *working_code_bytes1 = &working_code1[get_local_id(1) << 5];

	unsigned short rng_seed;
	unsigned short nibble_selector;

	unsigned char code_id = get_local_id(1);
	unsigned char int_id = get_local_id(0);
	unsigned int int_addr = get_local_id(1) * 32 + get_local_id(0);
	unsigned int IV = (get_global_id(2) << 16) | ((get_global_id(1) & 0x03FC) << 6) | ((get_global_id(0) & 0x07E0) >> 3) | get_local_id(1);
	unsigned int blah;
	//unsigned int blah2;

	rng_seed = (code_space[0] >> 8 & 0xFF) | ((code_space[0] << 8) & 0xFF00);
	
	// Expand to 128 bytes
	if (int_id % 2 == 0)
	{
		working_code1[int_addr] = code_space[0];
	}
	else
	{
		working_code1[int_addr] = IV;
	}
	//working_code1[int_addr] = select(code_space[0], (uint)(get_global_id(0) / 32), int_id % 2 == 0);

	working_code1[int_addr] = (((working_code1[int_addr] & 0xFF00FF00) + rng_space[rng_seed].expansion_values_high[int_id]) & 0xFF00FF00) | (((working_code1[int_addr] & 0x00FF00FF) + rng_space[rng_seed].expansion_values_low[int_id]) & 0x00FF00FF);


	//barrier(CLK_LOCAL_MEM_FENCE);

	/*
	// Get the key schedule information
	nibble_selector = key_schedule[0].nibble_selector;

	// Seed the RNG
	rng_seed = key_schedule[0].rng_seed;

	//working_code2[int_addr] = (nibble_selector << 16) | rng_seed;
	int j = 0;
	unsigned char nibble = (nibble_selector >> (15 - 0)) & 0x01;
	unsigned char algorithm_id = (((working_code1[j >> 2] >> ((j & 0x3) << 3)) >> (4 * nibble)) >> 1) & 0x07;
	
	working_code2[int_addr] = ((((working_code1[j >> 2] >> ((j & 0x3) << 3)) & 0xFF) >> (4 * nibble)) >> 1) & 0x07;
	*/
	
	
	for (int i = 0; i < 27; i++)
	{
		// make sure all threads are done expanding, or done with the previous algorithm
		barrier(CLK_LOCAL_MEM_FENCE);

		
		// Get the key schedule information
		nibble_selector = key_schedule[i].nibble_selector;

		// Seed the RNG
		rng_seed = key_schedule[i].rng_seed;
		
		

		for (int j = 0; j < 16; j++)
		{
			//rng_seed = (rng_seed >> 8 & 0xFF) | ((rng_seed << 8) & 0xFF00);	
			// Calculate the algorithm number based on the previous data
			unsigned char nibble = (nibble_selector >> (15 - j)) & 0x01;
			//unsigned char algorithm_id = ((((working_code1[j >> 2] >> ((j & 0x3) << 3)) & 0xFF) >> (4 * nibble)) >> 1) & 0x07;
			unsigned char algorithm_id = ((working_code_bytes1[j] >> (4 * nibble)) >> 1) & 0x07;

			
			//unsigned char algorithm_id = 0x00;
			//blah2 = algorithm_id;
						
			switch (algorithm_id)
			{
			case 0x00:
				working_code2[int_addr] = ((working_code1[int_addr] << 1) & 0xfefefefe) | rng_space[rng_seed].alg0_values[int_id];
				break;
			case 0x01:
				working_code2[int_addr] = (((working_code1[int_addr] & 0xFF00FF00) + rng_space[rng_seed].arth_high_values[int_id]) & 0xFF00FF00) | (((working_code1[int_addr] & 0x00FF00FF) + rng_space[rng_seed].arth_low_values[int_id]) & 0x00FF00FF);
				break;
			case 0x02:
				blah = (working_code1[int_addr] & 0x00010000) >> 8;
				if (int_id == 31)
				{
					blah = blah | ((rng_space[rng_seed].alg2_value & 0x000000001) << 24);
				}
				else
				{
					blah = blah | ((working_code1[int_addr+1] & 0x000000001) << 24);
				}
				working_code2[int_addr] = ((working_code1[int_addr] >> 1) & 0x007f007f) | ((working_code1[int_addr] >> 8) & 0x00800080) | ((working_code1[int_addr] << 1) & 0xfe00fe00) | (blah & 0x01000100);
				break;
			case 0x03:
				working_code2[int_addr] = working_code1[int_addr] ^ rng_space[rng_seed].regular_values[int_id];
				break;
			case 0x04:
				working_code2[int_addr] = (((working_code1[int_addr] | 0x00FF00FF) -  rng_space[rng_seed].arth_high_values[int_id]) & 0xFF00FF00) | (((working_code1[int_addr] | 0xFF00FF00) -  rng_space[rng_seed].arth_low_values[int_id]) & 0x00FF00FF);
				break;
			case 0x05:
				blah = (working_code1[int_addr] & 0x00800000) >> 8;
				if (int_id == 31)
				{
					blah = blah | ((rng_space[rng_seed].alg5_value & 0x000000080) << 24);
				}
				else
				{
					blah = blah | ((working_code1[int_addr+1] & 0x000000080) << 24);
				}

				working_code2[int_addr] = ((working_code1[int_addr] << 1) & 0x00fe00fe) | ((working_code1[int_addr] >> 8) & 0x00010001) | ((working_code1[int_addr] >> 1) & 0x7f007f00) | (blah & 0x80008000);

				break;
			case 0x06:
				working_code2[int_addr] = ((working_code1[int_addr] >> 1) & 0x7f7f7f7f) | rng_space[rng_seed].alg6_values[int_id];
				break;
			case 0x07:
				working_code2[int_addr] = ~working_code1[int_addr];
				break;
			}

			// Advance the RNG by the correct amount
			if (algorithm_id == 0x00 || algorithm_id == 0x01 || algorithm_id == 0x03 || algorithm_id == 0x04 || algorithm_id == 0x06)
			{
				rng_seed = rng_space[rng_seed].seed_forward_128;
			}
			else if (algorithm_id == 0x02 || algorithm_id == 0x05)
			{
				rng_seed = rng_space[rng_seed].seed_forward_1;
			}
			
			barrier(CLK_LOCAL_MEM_FENCE);
			working_code1[int_addr] = working_code2[int_addr];
			barrier(CLK_LOCAL_MEM_FENCE);
			
		}
		//working_code2[int_id] = rng_seed;
	}
	
	//code_space[0] = code_space[0] + get_local_id(0) % 32;
	//working_code2[int_id] = blah2;
	//working_code2[int_id] = ((working_code1[4 >> 2] >> ((4 & 0x3) << 3)) & 0xFF);
	//working_code2[int_id] = working_code1[int_id];
	if (IV == 0)
	{
	result_space[int_id] = working_code2[int_addr];
	}
}
