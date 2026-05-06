#include "key_schedule.h"

#include <cassert>
#include <cstring>
#include <vector>

key_schedule::key_schedule(uint32_t key, key_schedule::map_list_type map_list_option) : entry_count(0), seeds{ 0 }, nibble_selectors{ 0 }
{
	if (map_list_option == ALL_MAPS)
	{
		std::vector<uint8_t> map_list { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11 };
		init(key, map_list);
	}
	else if (map_list_option == SKIP_CAR)
	{
		std::vector<uint8_t> map_list{ 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11 };
 		init(key, map_list);
	}
}

key_schedule::key_schedule(uint32_t key, std::vector<uint8_t> map_list)
{
	init(key, map_list);
}

void key_schedule::init(uint32_t key, std::vector<uint8_t> map_list)
{
	entries.clear();
	entry_count = 0;

	schedule_data.as_uint8[0] = (key >> 24) & 0xFF;
	schedule_data.as_uint8[1] = (key >> 16) & 0xFF;
	schedule_data.as_uint8[2] = (key >> 8) & 0xFF;
	schedule_data.as_uint8[3] = key & 0xFF;

	for (std::vector<uint8_t>::iterator it = map_list.begin(); it != map_list.end(); ++it)
	{
		push_entry(generate_schedule_entry(*it));
		if (*it == 0x22)
		{
			push_entry(generate_schedule_entry(*it, 4));
		}
	}
}

void key_schedule::push_entry(key_schedule_entry e)
{
	assert(entry_count < MAX_ENTRIES);
	entries.push_back(e);
	seeds[entry_count] = static_cast<uint16_t>((e.rng1 << 8) | e.rng2);
	nibble_selectors[entry_count] = e.nibble_selector;
	entry_count++;
}


key_schedule::key_schedule_entry key_schedule::generate_schedule_entry(uint8_t map)
{
	uint8_t algorithm_number;

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
		algorithm_number = dispatch_alg(schedule_data.as_uint8, map);
	}

	return generate_schedule_entry(map, algorithm_number);
}

key_schedule::key_schedule_entry key_schedule::generate_schedule_entry(uint8_t map, uint8_t algorithm)
{
	run_alg(algorithm, map, schedule_data.as_uint8);

	key_schedule_entry result;
	result.rng1 = schedule_data.as_uint8[0];
	result.rng2 = schedule_data.as_uint8[1];
	result.nibble_selector = static_cast<uint16_t>((schedule_data.as_uint8[3] << 8) + schedule_data.as_uint8[2]);
	result.algorithm = algorithm;

	return result;
}

uint8_t key_schedule::dispatch_alg(const uint8_t schedule_data[4], uint8_t map)
{
	return static_cast<uint8_t>((schedule_data[(map >> 4) & 0x03] >> 2) & 0x07);
}

void key_schedule::run_alg(uint8_t algorithm, uint8_t map, uint8_t schedule_data[4])
{
	switch (algorithm)
	{
	case 0: algorithm_0(schedule_data, map); break;
	case 1: algorithm_1(schedule_data, map); break;
	case 2: algorithm_2(schedule_data, map); break;
	case 3: algorithm_3(schedule_data, map); break;
	case 4: algorithm_4(schedule_data, map); break;
	case 5: algorithm_5(schedule_data, map); break;
	case 6: algorithm_6(schedule_data, map); break;
	case 7: algorithm_7(schedule_data, map); break;
	default: break;
	}
}

void key_schedule::test_alg(const uint8_t input[4], uint8_t algorithm, uint8_t map, uint8_t output[4])
{
	std::memcpy(output, input, 4);
	run_alg(algorithm, map, output);
}

void key_schedule::test_dispatch(const uint8_t input[4], uint8_t map, uint8_t output[4])
{
	std::memcpy(output, input, 4);
	run_alg(dispatch_alg(output, map), map, output);
}

void key_schedule::algorithm_0(uint8_t s[4], uint8_t map)
{
	uint8_t temp = static_cast<uint8_t>(map ^ s[1]);
	uint8_t carry = static_cast<uint8_t>((((int)temp + s[0]) >> 8) & 0x01);
	temp = static_cast<uint8_t>(temp + s[0]);
	uint8_t next_carry = static_cast<uint8_t>((((int)temp - s[2] - (1 - carry)) < 0 ? 0 : 1));
	temp = static_cast<uint8_t>(temp - s[2] - (1 - carry));
	carry = next_carry;

	uint8_t rolling_sum = temp;

	for (char i = 3; i >= 0; i--)
	{
		next_carry = static_cast<uint8_t>((((int)rolling_sum + s[i] + carry) >> 8) & 0x01);
		rolling_sum = static_cast<uint8_t>(rolling_sum + s[i] + carry);
		s[i] = rolling_sum;

		carry = next_carry;
	}
}

void key_schedule::algorithm_1(uint8_t s[4], uint8_t map)
{
	unsigned char carry = 1;
	unsigned char rolling_sum = map;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = static_cast<unsigned char>((((int)rolling_sum + s[i] + carry) >> 8) & 0x01);
		rolling_sum = static_cast<unsigned char>(rolling_sum + s[i] + carry);
		s[i] = rolling_sum;

		carry = next_carry;
	}
}

void key_schedule::algorithm_2(uint8_t s[4], uint8_t map)
{
	// Add the map number to the first byte
	s[0] += map;

	unsigned char temp[4];
	temp[0] = s[0];
	temp[1] = s[1];
	temp[2] = s[2];
	temp[3] = s[3];

	// Reverse the code:
	s[0] = temp[3];
	s[1] = temp[2];
	s[2] = temp[1];
	s[3] = temp[0];
}

void key_schedule::algorithm_3(uint8_t s[4], uint8_t map)
{
	// Run alg 2 first
	algorithm_2(s, map);
	// Then alg 1
	algorithm_1(s, map);
}

void key_schedule::algorithm_4(uint8_t s[4], uint8_t map)
{
	// Run alg 2 first
	algorithm_2(s, map);
	// Then alg 0
	algorithm_0(s, map);
}

void key_schedule::algorithm_5(uint8_t s[4], uint8_t map)
{
	// Do an ASL on the map number
	unsigned char temp = static_cast<unsigned char>(map << 1);

	// EOR #$FF
	temp = static_cast<uint8_t>(temp ^ 0xFF);
	temp = static_cast<uint8_t>(temp + s[0]);
	temp = static_cast<uint8_t>(temp - map);
	s[0] = temp;

	uint8_t temp2[4];
	temp2[1] = s[1];
	temp2[2] = s[2];
	temp2[3] = s[3];

	s[1] = temp2[3];
	s[2] = temp2[1];
	s[3] = temp2[2];
}

void key_schedule::algorithm_6(uint8_t s[4], uint8_t map)
{
	// Run alg 5 first
	algorithm_5(s, map);
	// Then alg 1
	algorithm_1(s, map);
}

void key_schedule::algorithm_7(uint8_t s[4], uint8_t map)
{
	// Run alg 5 first
	algorithm_5(s, map);
	// Then alg 0
	algorithm_0(s, map);
}
