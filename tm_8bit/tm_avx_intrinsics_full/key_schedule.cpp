#include "data_sizes.h"
#include "key_schedule.h"


key_schedule_entry generate_schedule_entry(uint8 map, key_schedule_data *current_schedule_data)
{
	unsigned char algorithm_number;
	
	// Special case for 0x1B
	if (map == 0x1B)
	{
		algorithm_number = 6;
		generate_schedule_entry(map, current_schedule_data, algorithm_number);
	}

	// Special case
	if (map == 0x06)
	{
		algorithm_number = 0;
	}
	else
	{
		algorithm_number = (current_schedule_data->as_uint8[(map >> 4) & 0x03] >> 2) & 0x07;
	}

	return generate_schedule_entry(map, current_schedule_data, algorithm_number);
}

key_schedule_entry generate_schedule_entry(uint8 map, key_schedule_data *current_schedule_data, uint8 algorithm)
{
	key_schedule_algorithm(map, current_schedule_data, algorithm);

	key_schedule_entry result;
	result.rng1 = current_schedule_data->as_uint8[0];
	result.rng2 = current_schedule_data->as_uint8[1];
	result.nibble_selector = (current_schedule_data->as_uint8[3] << 8) + current_schedule_data->as_uint8[2];

	return result;
}

void key_schedule_algorithm(uint8 map, key_schedule_data *current_schedule_data, uint8 algorithm)
{
	//printf("key_schedule_algorithm: %i\n", algorithm_number);
	if (algorithm == 0x00)
	{
		key_schedule_algorithm_0(map, current_schedule_data);
	}
	else if (algorithm == 0x01)
	{
		key_schedule_algorithm_1(map, current_schedule_data);
	}
	else if (algorithm == 0x02)
	{
		key_schedule_algorithm_2(map, current_schedule_data);
	}
	else if (algorithm == 0x03)
	{
		key_schedule_algorithm_3(map, current_schedule_data);
	}
	else if (algorithm == 0x04)
	{
		key_schedule_algorithm_4(map, current_schedule_data);
	}
	else if (algorithm == 0x05)
	{
		key_schedule_algorithm_5(map, current_schedule_data);
	}
	else if (algorithm == 0x06)
	{
		key_schedule_algorithm_6(map, current_schedule_data);
	}
	else if	(algorithm == 0x07)
	{
		key_schedule_algorithm_7(map, current_schedule_data);
	}
}

void key_schedule_algorithm_0(uint8 map, key_schedule_data *current_schedule_data)
{
	unsigned char temp = map ^ current_schedule_data->as_uint8[1];
	unsigned char carry = (((int)temp + current_schedule_data->as_uint8[0]) >> 8) & 0x01;
	temp = temp + current_schedule_data->as_uint8[0];
	unsigned char next_carry = (((int)temp - current_schedule_data->as_uint8[2] - (1 - carry)) < 0 ? 0 : 1);
	temp = temp - current_schedule_data->as_uint8[2] - (1 - carry);
	carry = next_carry;

	unsigned char rolling_sum = temp;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + current_schedule_data->as_uint8[i] + carry) >> 8) & 0x01;
		rolling_sum += current_schedule_data->as_uint8[i] + carry;
		current_schedule_data->as_uint8[i] = rolling_sum;

		carry = next_carry;
	}
}

void key_schedule_algorithm_1(uint8 map, key_schedule_data *current_schedule_data)
{
	unsigned char carry = 1;
	unsigned char rolling_sum = map;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + current_schedule_data->as_uint8[i] + carry) >> 8) & 0x01;
		rolling_sum += current_schedule_data->as_uint8[i] + carry;
		current_schedule_data->as_uint8[i] = rolling_sum;

		carry = next_carry;
	}
}

void key_schedule_algorithm_2(uint8 map, key_schedule_data *current_schedule_data)
{
	// Add the map number to the first byte
	current_schedule_data->as_uint8[0] += map;

	unsigned char temp[4];
	temp[0] = current_schedule_data->as_uint8[0];
	temp[1] = current_schedule_data->as_uint8[1];
	temp[2] = current_schedule_data->as_uint8[2];
	temp[3] = current_schedule_data->as_uint8[3];

	// Reverse the code:
	current_schedule_data->as_uint8[0] = temp[3];
	current_schedule_data->as_uint8[1] = temp[2];
	current_schedule_data->as_uint8[2] = temp[1];
	current_schedule_data->as_uint8[3] = temp[0];
}

void key_schedule_algorithm_3(uint8 map, key_schedule_data *current_schedule_data)
{
	// Run alg 2 first
	key_schedule_algorithm_2(map, current_schedule_data);
	//display_code_backup(code_backup);
	// Then alg 1
	key_schedule_algorithm_1(map, current_schedule_data);
}

void key_schedule_algorithm_4(uint8 map, key_schedule_data *current_schedule_data)
{
	// Run alg 2 first
	key_schedule_algorithm_2(map, current_schedule_data);
	//display_code_backup(code_backup);
	// Then alg 1
	key_schedule_algorithm_0(map, current_schedule_data);
}

void key_schedule_algorithm_5(uint8 map, key_schedule_data *current_schedule_data)
{
	// Do an ASL on the map number
	unsigned char temp = map << 1;

	// EOR #$FF
	temp = temp ^ 0xFF;
	temp = temp + current_schedule_data->as_uint8[0];
	temp = temp - map;
	current_schedule_data->as_uint8[0] = temp;
	
	unsigned char temp2[4];
	temp2[1] = current_schedule_data->as_uint8[1];
	temp2[2] = current_schedule_data->as_uint8[2];
	temp2[3] = current_schedule_data->as_uint8[3];

	current_schedule_data->as_uint8[1] = temp2[3];
	current_schedule_data->as_uint8[2] = temp2[1];
	current_schedule_data->as_uint8[3] = temp2[2];
}

void key_schedule_algorithm_6(uint8 map, key_schedule_data *current_schedule_data)
{
	// Run alg 2 first
	key_schedule_algorithm_5(map, current_schedule_data);

	// Then alg 1
	key_schedule_algorithm_1(map, current_schedule_data);
}

void key_schedule_algorithm_7(uint8 map, key_schedule_data *current_schedule_data)
{
	// Run alg 2 first
	key_schedule_algorithm_5(map, current_schedule_data);

	// Then alg 1
	key_schedule_algorithm_0(map, current_schedule_data);
}

