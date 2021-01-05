#include "data_sizes.h"
#include "key_schedule.h"

#include <vector>

key_schedule::key_schedule(uint32 key, key_schedule::map_list_type map_list_option)
{
	if (map_list_option == ALL_MAPS)
	{
		std::vector<uint8> map_list { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11 };
		init(key, map_list);
	}
	else if (map_list_option == SKIP_CAR)
	{
		std::vector<uint8> map_list{ 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11 };
 		init(key, map_list);
	}
}

key_schedule::key_schedule(uint32 key, std::vector<uint8> map_list)
{
	init(key, map_list);
}

void key_schedule::init(uint32 key, std::vector<uint8> map_list)
{
	schedule_data.as_uint8[0] = (key >> 24) & 0xFF;
	schedule_data.as_uint8[1] = (key >> 16) & 0xFF;
	schedule_data.as_uint8[2] = (key >> 8) & 0xFF;
	schedule_data.as_uint8[3] = key & 0xFF;

	for (std::vector<uint8>::iterator it = map_list.begin(); it != map_list.end(); ++it)
	{
		entries.push_back(generate_schedule_entry(*it));
		if (*it == 0x22)
		{
			entries.push_back(generate_schedule_entry(*it, 4));
		}
	}
}


key_schedule::key_schedule_entry key_schedule::generate_schedule_entry(uint8 map)
{
	unsigned char algorithm_number;
	
	// Special case for 0x1B
	if (map == 0x1B)
	{
		algorithm_number = 6;
		generate_schedule_entry(map, algorithm_number);
	}

	// Special case
	if (map == 0x06)
	{
		algorithm_number = 0;
	}
	else
	{
		algorithm_number = (schedule_data.as_uint8[(map >> 4) & 0x03] >> 2) & 0x07;
	}

	return generate_schedule_entry(map, algorithm_number);
}

key_schedule::key_schedule_entry key_schedule::generate_schedule_entry(uint8 map, uint8 algorithm)
{
	if (algorithm == 0x00)
	{
		algorithm_0(map);
	}
	else if (algorithm == 0x01)
	{
		algorithm_1(map);
	}
	else if (algorithm == 0x02)
	{
		algorithm_2(map);
	}
	else if (algorithm == 0x03)
	{
		algorithm_3(map);
	}
	else if (algorithm == 0x04)
	{
		algorithm_4(map);
	}
	else if (algorithm == 0x05)
	{
		algorithm_5(map);
	}
	else if (algorithm == 0x06)
	{
		algorithm_6(map);
	}
	else if (algorithm == 0x07)
	{
		algorithm_7(map);
	}

	key_schedule_entry result;
	result.rng1 = schedule_data.as_uint8[0];
	result.rng2 = schedule_data.as_uint8[1];
	result.nibble_selector = (schedule_data.as_uint8[3] << 8) + schedule_data.as_uint8[2];

	return result;
}

void key_schedule::algorithm_0(uint8 map)
{
	unsigned char temp = map ^ schedule_data.as_uint8[1];
	unsigned char carry = (((int)temp + schedule_data.as_uint8[0]) >> 8) & 0x01;
	temp = temp + schedule_data.as_uint8[0];
	unsigned char next_carry = (((int)temp - schedule_data.as_uint8[2] - (1 - carry)) < 0 ? 0 : 1);
	temp = temp - schedule_data.as_uint8[2] - (1 - carry);
	carry = next_carry;

	unsigned char rolling_sum = temp;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + schedule_data.as_uint8[i] + carry) >> 8) & 0x01;
		rolling_sum += schedule_data.as_uint8[i] + carry;
		schedule_data.as_uint8[i] = rolling_sum;

		carry = next_carry;
	}
}

void key_schedule::algorithm_1(uint8 map)
{
	unsigned char carry = 1;
	unsigned char rolling_sum = map;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + schedule_data.as_uint8[i] + carry) >> 8) & 0x01;
		rolling_sum += schedule_data.as_uint8[i] + carry;
		schedule_data.as_uint8[i] = rolling_sum;

		carry = next_carry;
	}
}

void key_schedule::algorithm_2(uint8 map)
{
	// Add the map number to the first byte
	schedule_data.as_uint8[0] += map;

	unsigned char temp[4];
	temp[0] = schedule_data.as_uint8[0];
	temp[1] = schedule_data.as_uint8[1];
	temp[2] = schedule_data.as_uint8[2];
	temp[3] = schedule_data.as_uint8[3];

	// Reverse the code:
	schedule_data.as_uint8[0] = temp[3];
	schedule_data.as_uint8[1] = temp[2];
	schedule_data.as_uint8[2] = temp[1];
	schedule_data.as_uint8[3] = temp[0];
}

void key_schedule::algorithm_3(uint8 map)
{
	// Run alg 2 first
	algorithm_2(map);
	//display_code_backup(code_backup);
	// Then alg 1
	algorithm_1(map);
}

void key_schedule::algorithm_4(uint8 map)
{
	// Run alg 2 first
	algorithm_2(map);
	//display_code_backup(code_backup);
	// Then alg 1
	algorithm_0(map);
}

void key_schedule::algorithm_5(uint8 map)
{
	// Do an ASL on the map number
	unsigned char temp = map << 1;

	// EOR #$FF
	temp = temp ^ 0xFF;
	temp = temp + schedule_data.as_uint8[0];
	temp = temp - map;
	schedule_data.as_uint8[0] = temp;
	
	unsigned char temp2[4];
	temp2[1] = schedule_data.as_uint8[1];
	temp2[2] = schedule_data.as_uint8[2];
	temp2[3] = schedule_data.as_uint8[3];

	schedule_data.as_uint8[1] = temp2[3];
	schedule_data.as_uint8[2] = temp2[1];
	schedule_data.as_uint8[3] = temp2[2];
}

void key_schedule::algorithm_6(uint8 map)
{
	// Run alg 2 first
	algorithm_5(map);

	// Then alg 1
	algorithm_1(map);
}

void key_schedule::algorithm_7(uint8 map)
{
	// Run alg 2 first
	algorithm_5(map);

	// Then alg 1
	algorithm_0(map);
}

