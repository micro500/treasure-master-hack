#include "data_sizes.h"
#include "tree.h"
#include "key_schedule.h"
#include <vector>

std::vector<node*>* run_key_schedule(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* result = new std::vector<node*>();
	result->insert(result->end(), key_schedule_data, key_schedule_data + 32);

	unsigned char algorithm_number;

	// Special case for 0x1B
	if (map == 0x1B)
	{
		algorithm_number = 6;
		result = key_schedule_algorithm(map, result->begin(), algorithm_number);
	}

	// Special case
	if (map == 0x06)
	{
		algorithm_number = 0;
		result = key_schedule_algorithm(map, result->begin(), algorithm_number);
	}
	else
	{
		uint8 sched_index = (map >> 4) & 0x03;

		std::vector<node*>* alg_bits = new std::vector<node*>();
		if (sched_index == 0)
		{
			alg_bits->insert(alg_bits->end(), result->begin() + 2, result->begin() + 5);
		}
		else if (sched_index == 1)
		{
			alg_bits->insert(alg_bits->end(), result->begin() + 10, result->begin() + 13);
		}
		else if (sched_index == 2)
		{
			alg_bits->insert(alg_bits->end(), result->begin() + 18, result->begin() + 21);
		}
		else if (sched_index == 3)
		{
			alg_bits->insert(alg_bits->end(), result->begin() + 26, result->begin() + 29);
		}

		std::vector<std::vector<node*>*>* alg_results = new std::vector<std::vector<node*>*>();
		std::vector<node*>* alg_flags = get_num_flags(alg_bits->begin());

		for (int i = 0; i < 8; i++)
		{
			std::vector<node*>* alg_result = key_schedule_algorithm(map, result->begin(), i);

			alg_results->push_back(and_set(alg_result->begin(), (*alg_flags)[i], 32));
		}

		result = or_condense(alg_results);
	}

	return result;
}

std::vector<node*>* key_schedule_algorithm(uint8 map, std::vector<node*>::iterator key_schedule_data, uint8 algorithm)
{
	std::vector<node*>* result;

	if (algorithm == 0x00)
	{
		result = key_schedule_algorithm_0(map, key_schedule_data);
	}
	else if (algorithm == 0x01)
	{
		result = key_schedule_algorithm_1(map, key_schedule_data);
	}
	else if (algorithm == 0x02)
	{
		result = key_schedule_algorithm_2(map, key_schedule_data);
	}
	else if (algorithm == 0x03)
	{
		result = key_schedule_algorithm_3(map, key_schedule_data);
	}
	else if (algorithm == 0x04)
	{
		result = key_schedule_algorithm_4(map, key_schedule_data);
	}
	else if (algorithm == 0x05)
	{
		result = key_schedule_algorithm_5(map, key_schedule_data);
	}
	else if (algorithm == 0x06)
	{
		result = key_schedule_algorithm_6(map, key_schedule_data);
	}
	else// if (algorithm == 0x07)
	{
		result = key_schedule_algorithm_7(map, key_schedule_data);
	}

	return result;
}


std::vector<node*>* key_schedule_algorithm_0(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* result = new std::vector<node*>();

	std::vector<node*>* temp = xor_bits(key_schedule_data + 8, get_static_byte(map)->begin(), 8);

	temp = add_bits(temp->begin(), key_schedule_data, new_node_bit(0), 8);
	node* carry = (*temp)[8];

	std::vector<node*>* inv_sched2 = new std::vector<node*>();
	for (int i = 0; i < 8; i++)
	{
		inv_sched2->push_back(new_node(NOT, *(key_schedule_data + 16 + i), NULL));
	}

	temp = add_bits(temp->begin(), inv_sched2->begin(), carry, 8);
	carry = (*temp)[8];

	temp = add_bits(temp->begin(), key_schedule_data + 24, carry, 8);
	carry = (*temp)[8];
	std::vector<node*>* sched3 = new std::vector<node*>();
	sched3->insert(sched3->end(), temp->begin(), temp->begin() + 8);

	temp = add_bits(temp->begin(), key_schedule_data + 16, carry, 8);
	carry = (*temp)[8];
	std::vector<node*>* sched2 = new std::vector<node*>();
	sched2->insert(sched2->end(), temp->begin(), temp->begin() + 8);

	temp = add_bits(temp->begin(), key_schedule_data + 8, carry, 8);
	carry = (*temp)[8];
	std::vector<node*>* sched1 = new std::vector<node*>();
	sched1->insert(sched1->end(), temp->begin(), temp->begin() + 8);

	temp = add_bits(temp->begin(), key_schedule_data, carry, 8);
	carry = (*temp)[8];
	std::vector<node*>* sched0 = new std::vector<node*>();
	sched0->insert(sched0->end(), temp->begin(), temp->begin() + 8);

	result->insert(result->end(), sched0->begin(), sched0->begin() + 8);
	result->insert(result->end(), sched1->begin(), sched1->begin() + 8);
	result->insert(result->end(), sched2->begin(), sched2->begin() + 8);
	result->insert(result->end(), sched3->begin(), sched3->begin() + 8);

	return result;
}


std::vector<node*>* key_schedule_algorithm_1(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	node* carry = new_node_bit(1);

	std::vector<node*>* rolling_sum = get_static_byte(map);

	rolling_sum = add_bits(rolling_sum->begin(), key_schedule_data + 24, carry, 8);
	carry = (*rolling_sum)[8];
	std::vector<node*>* sched3 = new std::vector<node*>();
	sched3->insert(sched3->end(), rolling_sum->begin(), rolling_sum->begin() + 8);

	rolling_sum = add_bits(rolling_sum->begin(), key_schedule_data + 16, carry, 8);
	carry = (*rolling_sum)[8];
	std::vector<node*>* sched2 = new std::vector<node*>();
	sched2->insert(sched2->end(), rolling_sum->begin(), rolling_sum->begin() + 8);

	rolling_sum = add_bits(rolling_sum->begin(), key_schedule_data + 8, carry, 8);
	carry = (*rolling_sum)[8];
	std::vector<node*>* sched1 = new std::vector<node*>();
	sched1->insert(sched1->end(), rolling_sum->begin(), rolling_sum->begin() + 8);

	rolling_sum = add_bits(rolling_sum->begin(), key_schedule_data, carry, 8);
	carry = (*rolling_sum)[8];
	std::vector<node*>* sched0 = new std::vector<node*>();
	sched0->insert(sched0->end(), rolling_sum->begin(), rolling_sum->begin() + 8);

	std::vector<node*>* result = new std::vector<node*>();
	result->insert(result->end(), sched0->begin(), sched0->begin() + 8);
	result->insert(result->end(), sched1->begin(), sched1->begin() + 8);
	result->insert(result->end(), sched2->begin(), sched2->begin() + 8);
	result->insert(result->end(), sched3->begin(), sched3->begin() + 8);

	return result;
}

std::vector<node*>* key_schedule_algorithm_2(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* temp = add_bits(key_schedule_data, get_static_byte(map)->begin(), new_node_bit(0), 8);

	std::vector<node*>* result = new std::vector<node*>();
	result->insert(result->end(), key_schedule_data + 24, key_schedule_data + 32);
	result->insert(result->end(), key_schedule_data + 16, key_schedule_data + 24);
	result->insert(result->end(), key_schedule_data + 8, key_schedule_data + 16);
	result->insert(result->end(), temp->begin(), temp->begin() + 8);

	return result;
}

std::vector<node*>* key_schedule_algorithm_3(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* result;

	result = key_schedule_algorithm_2(map, key_schedule_data);

	result = key_schedule_algorithm_1(map, result->begin());

	return result;
}

std::vector<node*>* key_schedule_algorithm_4(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* result;

	result = key_schedule_algorithm_2(map, key_schedule_data);

	result = key_schedule_algorithm_0(map, result->begin());

	return result;
}


std::vector<node*>* key_schedule_algorithm_5(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* map_nodes = get_static_byte(map);

	std::vector<node*>* temp = new std::vector<node*>();
	temp->push_back(new_node_bit(0));
	temp->insert(temp->end(), map_nodes->begin(), map_nodes->begin() + 7);

	temp = xor_bits(temp->begin(), get_static_byte(0xFF)->begin(), 8);
	temp = add_bits(temp->begin(), key_schedule_data, new_node_bit(0), 8);

	uint8 neg_map = ((map ^ 0xFF) + 1) & 0xFF;

	temp = add_bits(temp->begin(), get_static_byte(neg_map)->begin(), new_node_bit(0), 8);

	std::vector<node*>* result = new std::vector<node*>();
	result->insert(result->end(), temp->begin(), temp->begin() + 8);

	result->insert(result->end(), key_schedule_data + 24, key_schedule_data + 32);
	result->insert(result->end(), key_schedule_data + 8, key_schedule_data + 16);
	result->insert(result->end(), key_schedule_data + 16, key_schedule_data + 24);

	return result;
}

std::vector<node*>* key_schedule_algorithm_6(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* result;

	result = key_schedule_algorithm_5(map, key_schedule_data);

	result = key_schedule_algorithm_1(map, result->begin());

	return result;
}

std::vector<node*>* key_schedule_algorithm_7(uint8 map, std::vector<node*>::iterator key_schedule_data)
{
	std::vector<node*>* result;

	result = key_schedule_algorithm_5(map, key_schedule_data);

	result = key_schedule_algorithm_0(map, result->begin());

	return result;
}