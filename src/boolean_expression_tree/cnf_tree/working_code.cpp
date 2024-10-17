#include <vector>
#include "tree.h"
#include "working_code.h"

working_code::working_code(std::vector<node*>* bitfield_init, std::vector<node*>* rng_vals_init) : bitfield(bitfield_init)
{
	this->set_rng_init(rng_vals_init);
}

working_code::~working_code() {}

void working_code::set_rng_init(std::vector<node*>* rng_vals_init)
{
	rng_vals = new std::vector<std::vector<node*>*>();
	rng_vals->push_back(rng_vals_init);
}

void working_code::rng_process()
{
	std::vector<node*>* cur_rng = (*this->rng_vals)[0];
	for (int i = 0; i < 128; i++)
	{
		cur_rng = rng_calc(cur_rng->begin(), cur_rng->begin() + 8);
		this->rng_vals->push_back(cur_rng);
	}
}

std::vector<node*>* working_code::get_rng(uint8 rng_index)
{
	return (*this->rng_vals)[rng_index];
}

void working_code::add_rng_to_result(std::vector<node*>* result, uint8 rng_index)
{
	std::vector<node*>* rng_val = this->get_rng(rng_index);
	result->insert(result->end(), rng_val->begin(), rng_val->begin() + 16);
}

std::vector<node*>* working_code::alg(uint8 alg_num)
{
	if (alg_num == 0)
	{
		return this->alg0();
	}
	else if (alg_num == 1)
	{
		return this->alg1();
	}
	else if (alg_num == 2)
	{
		return this->alg2();
	}
	else if (alg_num == 3)
	{
		return this->alg3();
	}
	else if (alg_num == 4)
	{
		return this->alg4();
	}
	else if (alg_num == 5)
	{
		return this->alg5();
	}
	else if (alg_num == 6)
	{
		return this->alg6();
	}
	else // if (alg_num == 7)
	{
		return this->alg7();
	}
}

std::vector<node*>* working_code::alg0()
{
	std::vector<node*>* result = new std::vector<node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<node*>* rng_vals = this->get_rng(rng_index);
		std::vector<node*>* temp = new std::vector<node*>();
		temp->push_back(*(rng_vals->begin() + 16 + 7));
		for (int j = 0; j < 7; j++)
		{
			temp->push_back(*(bitfield->begin() + (i * 8) + j));
		}

		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<node*>* working_code::alg1()
{
	std::vector<node*>* result = new std::vector<node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<node*>* rng_vals = this->get_rng(rng_index);
		std::vector<node*>* temp = add_bits(bitfield->begin() + (i * 8), rng_vals->begin() + 16, new_node_bit(0), 8);
		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<node*>* working_code::alg2()
{
	std::vector<node*>* result = new std::vector<node*>();
	int rng_index = 0;

	rng_index++;
	std::vector<node*>* rng_vals = this->get_rng(rng_index);
	node* carry = *(rng_vals->begin() + 16 + 7);

	for (int i = 0x7F; i >= 0; i -= 2)
	{
		node* next_carry = *(bitfield->begin() + ((i - 1) * 8));

		std::vector<node*>* temp = new std::vector<node*>();
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 1)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 2)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 3)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 4)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 5)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 6)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 7)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 7)));


		temp->push_back(carry);
		temp->push_back(*(bitfield->begin() + (i * 8)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 1)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 2)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 3)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 4)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 5)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 6)));

		carry = next_carry;

		result->insert(result->begin(), temp->begin(), temp->begin() + 16);
	}

	this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<node*>* working_code::alg3()
{
	std::vector<node*>* result = new std::vector<node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<node*>* rng_vals = this->get_rng(rng_index);
		std::vector<node*>* temp = xor_bits(bitfield->begin() + (i * 8), rng_vals->begin() + 16, 8);
		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<node*>* working_code::alg4()
{
	std::vector<node*>* result = new std::vector<node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<node*>* rng_vals = this->get_rng(rng_index);
		std::vector<node*>* inv_rng = new std::vector<node*>();
		for (int i = 0; i < 8; i++)
		{
			inv_rng->push_back(new_node(NOT, (*rng_vals)[i + 16], NULL));
		}

		std::vector<node*>* temp = add_bits(bitfield->begin() + (i * 8), inv_rng->begin(), new_node_bit(1), 8);
		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<node*>* working_code::alg5()
{
	std::vector<node*>* result = new std::vector<node*>();
	int rng_index = 0;

	rng_index++;
	std::vector<node*>* rng_vals = this->get_rng(rng_index);

	node* carry = *(rng_vals->begin() + 16 + 7);

	for (int i = 0x7F; i >= 0; i -= 2)
	{
		node* next_carry = *(bitfield->begin() + ((i - 1) * 8 + 7));

		std::vector<node*>* temp = new std::vector<node*>();
		temp->push_back(*(bitfield->begin() + (i * 8)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 1)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 2)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 3)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 4)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 5)));
		temp->push_back(*(bitfield->begin() + ((i - 1) * 8 + 6)));


		temp->push_back(*(bitfield->begin() + (i * 8 + 1)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 2)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 3)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 4)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 5)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 6)));
		temp->push_back(*(bitfield->begin() + (i * 8 + 7)));
		temp->push_back(carry);

		carry = next_carry;

		result->insert(result->begin(), temp->begin(), temp->begin() + 16);
	}

	this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<node*>* working_code::alg6()
{
	std::vector<node*>* result = new std::vector<node*>();
	int rng_index = 0;
	for (int i = 0; i < 128; i++)
	{
		rng_index++;
		std::vector<node*>* rng_vals = this->get_rng(rng_index);
		std::vector<node*>* temp = new std::vector<node*>();
		for (int j = 1; j < 8; j++)
		{
			temp->push_back(*(bitfield->begin() + (i * 8) + j));
		}

		temp->push_back(*(rng_vals->begin() + 16 + 7));

		result->insert(result->end(), temp->begin(), temp->begin() + 8);
	}

	this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<node*>* working_code::alg7()
{
	std::vector<node*>* result = new std::vector<node*>();
	for (int i = 0; i < 1024; i++)
	{
		result->push_back(new_node(NOT, *(bitfield->begin() + i), NULL));
	}

	this->add_rng_to_result(result, 0);
	return result;
}

void working_code::process_working(std::vector<node*>* schedule_entry, int sched_start_at)
{
	std::vector<node*>* rng_init = new std::vector<node*>();
	rng_init->insert(rng_init->end(), schedule_entry->begin(), schedule_entry->begin() + 16);

	this->set_rng_init(rng_init);

	std::vector<node*>* nibble_selector = new std::vector<node*>();
	nibble_selector->insert(nibble_selector->end(), schedule_entry->begin() + 16, schedule_entry->begin() + 32);

	alg06_node = new_node_bit(0);

	for (int i = sched_start_at; i < 16; i++)
	{
		this->rng_process();

		node* nibble = *(nibble_selector->begin() + (15 - i));
		node* inv_nibble = new_node(NOT, nibble, NULL);

		std::vector<node*>* low_bits = new std::vector<node*>();
		low_bits->insert(low_bits->end(), this->bitfield->begin() + (i * 8) + 1, this->bitfield->begin() + (i * 8) + 1 + 3);

		std::vector<node*>* high_bits = new std::vector<node*>();
		high_bits->insert(high_bits->end(), this->bitfield->begin() + (i * 8) + 5, this->bitfield->begin() + (i * 8) + 5 + 3);

		std::vector<std::vector<node*>*>* all_bits = new std::vector<std::vector<node*>*>();

		all_bits->push_back(and_set(low_bits->begin(), inv_nibble, 3));
		all_bits->push_back(and_set(high_bits->begin(), nibble, 3));

		std::vector<node*>* alg_bits = or_condense(all_bits);

		std::vector<std::vector<node*>*>* alg_results = new std::vector<std::vector<node*>*>();
		std::vector<node*>* alg_flags = get_num_flags(alg_bits->begin());

		alg06_node = new_node(OR, (*alg_flags)[0], alg06_node);
		alg06_node = new_node(OR, (*alg_flags)[6], alg06_node);

		for (int i = 0; i < 8; i++)
		{
			std::vector<node*>* alg_result = this->alg(i);

			alg_results->push_back(and_set(alg_result->begin(), (*alg_flags)[i], 1024 + 16));
		}

		std::vector<node*>* result = or_condense(alg_results);

		std::vector<node*>* new_working_code = new std::vector<node*>();
		new_working_code->insert(new_working_code->end(), result->begin(), result->begin() + 1024);

		std::vector<node*>* new_rng_val = new std::vector<node*>();
		new_rng_val->insert(new_rng_val->end(), result->begin() + 1024, result->begin() + 1024 + 16);

		this->bitfield = new_working_code;
		this->set_rng_init(new_rng_val);
	}

}

void working_code::process_working(std::vector<node*>* schedule_entry)
{
	process_working(schedule_entry, 0);
}

