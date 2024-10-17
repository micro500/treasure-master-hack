#include <vector>
#include "tree_node.h"
#include "tree_manager.h"
#include "working_code.h"

working_code::working_code(std::vector<tree_node*>* bitfield_init, tree_manager* manager_in) : bitfield(bitfield_init), manager(manager_in)
{
	alg06_node = NULL;
	rng_vals_init = NULL;

	rng_fwd_1 = NULL;
	rng_fwd_128 = NULL;

	use_rng_steps = true;
}
working_code::~working_code() {}

void working_code::rng_process()
{
	if (use_rng_steps)
	{
		rng_process_steps();
	}
	else
	{
		rng_process_precalc();
	}
}

void working_code::rng_process_precalc()
{
	rng_res_nodes = new std::vector<std::vector<tree_node*>*>();
	for (int depth = 1; depth <= 128; depth++)
	{
		std::vector<tree_node*>* cur_depth = new std::vector<tree_node*>();
		for (int bit_index = 0; bit_index < 8; bit_index++)
		{
			tree_node* t = manager->new_node(_RNG_RES, *rng_vals_init);
			t->rng_depth = depth;
			t->rng_bit_index = bit_index;

			cur_depth->push_back(t);
		}
		rng_res_nodes->push_back(cur_depth);
	}

	rng_fwd_1 = new std::vector<tree_node*>();
	rng_fwd_128 = new std::vector<tree_node*>();

	for (int bit_index = 0; bit_index < 16; bit_index++)
	{
		tree_node* fwd_1 = manager->new_node(_RNG_FWD, *rng_vals_init);
		fwd_1->rng_depth = 1;
		fwd_1->rng_bit_index = bit_index;
		rng_fwd_1->push_back(fwd_1);

		tree_node* fwd_128 = manager->new_node(_RNG_FWD, *rng_vals_init);
		fwd_128->rng_depth = 128;
		fwd_128->rng_bit_index = bit_index;
		rng_fwd_128->push_back(fwd_128);
	}
}

void working_code::rng_process_steps()
{
	rng_res_nodes = new std::vector<std::vector<tree_node*>*>();

	this->rng_vals = new std::vector<std::vector<tree_node*>*>();
	std::vector<tree_node*>* cur_rng = rng_vals_init;
	this->rng_vals->push_back(cur_rng);
	for (int i = 1; i <= 128; i++)
	{
		//cur_rng = rng_calc2(cur_rng->begin());
		cur_rng = rng_calc3(cur_rng->begin(), cur_rng->begin() + 8);
		this->rng_vals->push_back(cur_rng);

		std::vector<tree_node*>* cur_depth = new std::vector<tree_node*>();
		cur_depth->insert(cur_depth->end(), cur_rng->begin() + 16, cur_rng->begin() + 16 + 8);

		rng_res_nodes->push_back(cur_depth);

		if (i == 1)
		{
			rng_fwd_1 = new std::vector<tree_node*>();
			rng_fwd_1->insert(rng_fwd_1->end(), cur_rng->begin(), cur_rng->begin() + 16);
		}

		if (i == 128)
		{
			rng_fwd_128 = new std::vector<tree_node*>();
			rng_fwd_128->insert(rng_fwd_128->end(), cur_rng->begin(), cur_rng->begin() + 16);
		}
	}
}

std::vector<tree_node*>* working_code::get_rng_res(uint8 rng_index)
{
	return (*rng_res_nodes)[rng_index - 1];
}

std::vector<tree_node*>* working_code::get_rng_fwd(uint8 rng_index)
{
	if (rng_index == 1)
	{
		return rng_fwd_1;
	}
	else
	{
		return rng_fwd_128;
	}
}

std::vector<tree_node*>* add_16bit_static2(std::vector<tree_node*>::iterator left, uint16 val, tree_node* carry_in)
{
	tree_manager* m = (*left)->manager;
	std::vector<tree_node*>* val_bits = m->get_static_16bit(val);

	std::vector<tree_node*>* result = add_bits2(left, val_bits->begin(), carry_in, 16);

	return result;
}

tree_node* greater_than_static2(std::vector<tree_node*>* val_bitfield, uint64 compare_val, uint8 compare_bits)
{
	tree_manager* m = (*val_bitfield)[0]->manager;

	tree_node* cur_node = m->new_node_bit(0);
	bool first_bit_found = false;
	for (int i = 0; i < compare_bits; i++)
	{
		uint8 cur_bit = (compare_val >> i) & 0x01;
		if (!first_bit_found && cur_bit == 1)
		{
			continue;
		}

		first_bit_found = true;

		if (cur_bit == 0)
		{
			cur_node = m->new_node(_OR, (*val_bitfield)[i], cur_node);
		}
		else
		{
			cur_node = m->new_node(_AND, (*val_bitfield)[i], cur_node);
		}
	}

	return cur_node;
}


std::vector<tree_node*>* rng_calc3(std::vector<tree_node*>::iterator rng1, std::vector<tree_node*>::iterator rng2)
{
	tree_manager* m = (*rng1)->manager;

	std::vector<tree_node*>* rngB = add_bits2(rng1, rng2, m->new_node_bit(0), 8);

	std::vector<tree_node*>* cur_rng = new std::vector<tree_node*>();

	cur_rng->insert(cur_rng->end(), rng1, rng1 + 8);
	cur_rng->insert(cur_rng->end(), rngB->begin(), rngB->begin() + 8);

	tree_node* greater_carry = greater_than_static2(cur_rng, 0xD576, 16);

	cur_rng = add_16bit_static2(cur_rng->begin(), 0x6daa, greater_carry);

	std::vector<tree_node*>* val_result = xor_bits2(cur_rng->begin(), cur_rng->begin() + 8, 8);

	std::vector<tree_node*>* result = new std::vector<tree_node*>();

	result->insert(result->end(), cur_rng->begin(), cur_rng->begin() + 16);

	result->insert(result->end(), val_result->begin(), val_result->begin() + 8);

	return result;
}


std::vector<tree_node*>* rng_calc2(std::vector<tree_node*>::iterator rng_val)
{
	tree_manager* m = (*rng_val)->manager;

	std::vector<tree_node*>* rngA = new std::vector<tree_node*>();
	rngA->insert(rngA->end(), rng_val, rng_val + 8);

	std::vector<tree_node*>* rngB = new std::vector<tree_node*>();
	rngB->insert(rngB->end(), rng_val + 8, rng_val + 16);

	rngB = add_bits2(rngA->begin(), rngB->begin(), m->new_node_bit(0), 8);

	std::vector<tree_node*>* cur_rng = new std::vector<tree_node*>();

	cur_rng->insert(cur_rng->end(), rngA->begin(), rngA->begin() + 8);
	cur_rng->insert(cur_rng->end(), rngB->begin(), rngB->begin() + 8);

	tree_node* greater_carry = greater_than_static2(cur_rng, 0xD576, 16);

	cur_rng = add_16bit_static2(cur_rng->begin(), 0x6daa, greater_carry);

	std::vector<tree_node*>* val_result = xor_bits2(cur_rng->begin(), cur_rng->begin() + 8, 8);

	std::vector<tree_node*>* result = new std::vector<tree_node*>();

	result->insert(result->end(), cur_rng->begin(), cur_rng->begin() + 16);

	result->insert(result->end(), val_result->begin(), val_result->begin() + 8);

	return result;
}

std::vector<tree_node*>* working_code::alg(uint8 alg_num)
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

std::vector<tree_node*>* working_code::alg0()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<tree_node*>* rng_vals = get_rng_res(rng_index);
		std::vector<tree_node*>* temp = new std::vector<tree_node*>();
		temp->push_back(*(rng_vals->begin() + 7));
		for (int j = 0; j < 7; j++)
		{
			temp->push_back(*(bitfield->begin() + (i * 8) + j));
		}

		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	//this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<tree_node*>* working_code::alg1()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<tree_node*>* rng_vals = get_rng_res(rng_index);
		std::vector<tree_node*>* temp = add_bits2(bitfield->begin() + (i * 8), rng_vals->begin(), manager->new_node_bit(0), 8);
		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	//this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<tree_node*>* working_code::alg2()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	int rng_index = 0;

	rng_index++;
	std::vector<tree_node*>* rng_vals = get_rng_res(rng_index);
	tree_node* carry = *(rng_vals->begin() + 7);

	for (int i = 0x7F; i >= 0; i -= 2)
	{
		tree_node* next_carry = *(bitfield->begin() + ((i - 1) * 8));

		std::vector<tree_node*>* temp = new std::vector<tree_node*>();
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

	//this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<tree_node*>* working_code::alg3()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<tree_node*>* rng_vals = get_rng_res(rng_index);
		std::vector<tree_node*>* temp = xor_bits2(bitfield->begin() + (i * 8), rng_vals->begin(), 8);
		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	//this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<tree_node*>* working_code::alg4()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	int rng_index = 0;
	for (int i = 127; i >= 0; i--)
	{
		rng_index++;
		std::vector<tree_node*>* rng_vals = get_rng_res(rng_index);
		std::vector<tree_node*>* inv_rng = new std::vector<tree_node*>();
		for (int i = 0; i < 8; i++)
		{
			inv_rng->push_back(manager->new_node(_NOT, (*rng_vals)[i], NULL));
		}

		std::vector<tree_node*>* temp = add_bits2(bitfield->begin() + (i * 8), inv_rng->begin(), manager->new_node_bit(1), 8);
		result->insert(result->begin(), temp->begin(), temp->begin() + 8);
	}

	//this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<tree_node*>* working_code::alg5()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	int rng_index = 0;

	rng_index++;
	std::vector<tree_node*>* rng_vals = get_rng_res(rng_index);

	tree_node* carry = *(rng_vals->begin() + 7);

	for (int i = 0x7F; i >= 0; i -= 2)
	{
		tree_node* next_carry = *(bitfield->begin() + ((i - 1) * 8 + 7));

		std::vector<tree_node*>* temp = new std::vector<tree_node*>();
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

	//this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<tree_node*>* working_code::alg6()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	int rng_index = 0;
	for (int i = 0; i < 128; i++)
	{
		rng_index++;
		std::vector<tree_node*>* rng_vals = get_rng_res(rng_index);
		
		std::vector<tree_node*>* temp = new std::vector<tree_node*>();
		for (int j = 1; j < 8; j++)
		{
			temp->push_back(*(bitfield->begin() + (i * 8) + j));
		}

		temp->push_back(*(rng_vals->begin() + 7));

		result->insert(result->end(), temp->begin(), temp->begin() + 8);
	}

	//this->add_rng_to_result(result, rng_index);
	return result;
}

std::vector<tree_node*>* working_code::alg7()
{
	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	for (int i = 0; i < 1024; i++)
	{
		result->push_back(manager->new_node(_NOT, *(bitfield->begin() + i), NULL));
	}

	//this->add_rng_to_result(result, 0);
	return result;
}

void working_code::process_forced_step(std::vector<tree_node*>* schedule_entry, int forced_alg, int nibble_index)
{
	tree_manager* m = (*bitfield)[0]->manager;

	std::vector<tree_node*>* nibble_selector = new std::vector<tree_node*>();
	nibble_selector->insert(nibble_selector->end(), schedule_entry->begin() + 16, schedule_entry->begin() + 32);

	rng_process();

	std::vector<tree_node*>* alg_flags = get_alg_flags(nibble_selector, nibble_index);

	forced_alg_node = m->new_node(_AND, forced_alg_node, (*alg_flags)[forced_alg]);

	std::vector<tree_node*>* alg_result = this->alg(forced_alg);

	std::vector<tree_node*>* new_working_code = new std::vector<tree_node*>();
	new_working_code->insert(new_working_code->end(), alg_result->begin(), alg_result->begin() + 1024);

	this->bitfield = new_working_code;

	if (forced_alg == 2 || forced_alg == 5)
	{
		rng_vals_init = rng_fwd_1;
	}
	else if (forced_alg == 0 || forced_alg == 1 || forced_alg == 3 || forced_alg == 4 || forced_alg == 6)
	{
		rng_vals_init = rng_fwd_128;
	}
}

void working_code::process_working(std::vector<tree_node*>* schedule_entry, int sched_start_at)
{
	process_working(schedule_entry, sched_start_at, 16);
}

void working_code::process_working(std::vector<tree_node*>* schedule_entry, int sched_start_at, int sched_end_at)
{
	tree_manager* m = (*bitfield)[0]->manager;
	rng_vals_init = new std::vector<tree_node*>();
	rng_vals_init->insert(rng_vals_init->end(), schedule_entry->begin(), schedule_entry->begin() + 16);

	//this->set_rng_init(rng_init);

	std::vector<tree_node*>* nibble_selector = new std::vector<tree_node*>();
	nibble_selector->insert(nibble_selector->end(), schedule_entry->begin() + 16, schedule_entry->begin() + 32);

	alg06_node = m->new_node_bit(0);

	for (int i = sched_start_at; i < sched_end_at; i++)
	{
		rng_process();

		std::vector<tree_node*>* alg_flags = get_alg_flags(nibble_selector, i);

		alg06_node = m->new_node(_OR, (*alg_flags)[0], alg06_node);
		alg06_node = m->new_node(_OR, (*alg_flags)[6], alg06_node);

		std::vector<std::vector<tree_node*>*>* alg_results = new std::vector<std::vector<tree_node*>*>();

		for (int i = 0; i < 8; i++)
		{
			std::vector<tree_node*>* alg_result = this->alg(i);

			alg_results->push_back(and_set2(alg_result->begin(), (*alg_flags)[i], 1024));
		}

		std::vector<tree_node*>* result = or_condense2(alg_results);

		std::vector<tree_node*>* new_working_code = new std::vector<tree_node*>();
		new_working_code->insert(new_working_code->end(), result->begin(), result->begin() + 1024);

		//std::vector<tree_node*>* new_rng_val = new std::vector<tree_node*>();
		//new_rng_val->insert(new_rng_val->end(), result->begin() + 1024, result->begin() + 1024 + 16);

		this->bitfield = new_working_code;
		//this->set_rng_init(new_rng_val);

		tree_node* need_fwd_0 = (*alg_flags)[7];
		tree_node* need_fwd_1 = m->new_node(_OR, (*alg_flags)[2], (*alg_flags)[5]);
		tree_node* need_fwd_128 = m->new_node(_OR, (*alg_flags)[0], (*alg_flags)[1]);
		need_fwd_128 = m->new_node(_OR, need_fwd_128, (*alg_flags)[3]);
		need_fwd_128 = m->new_node(_OR, need_fwd_128, (*alg_flags)[4]);
		need_fwd_128 = m->new_node(_OR, need_fwd_128, (*alg_flags)[6]);

		std::vector<std::vector<tree_node*>*>* all_next_rng = new std::vector<std::vector<tree_node*>*>();

		all_next_rng->push_back(and_set2(rng_vals_init->begin(), need_fwd_0, 16));
		all_next_rng->push_back(and_set2(rng_fwd_1->begin(), need_fwd_1, 16));
		all_next_rng->push_back(and_set2(rng_fwd_128->begin(), need_fwd_128, 16));

		rng_vals_init = or_condense2(all_next_rng);
	}
}

std::vector<tree_node*>* working_code::get_alg_flags(std::vector<tree_node*>* nibble_selector, int nibble_index)
{
	tree_manager* m = (*bitfield)[0]->manager;

	tree_node* nibble = *(nibble_selector->begin() + (15 - nibble_index));
	tree_node* inv_nibble = m->new_node(_NOT, nibble, NULL);

	int alg_byte_offset = nibble_index * 8;

	std::vector<tree_node*>* low_bits = new std::vector<tree_node*>();
	low_bits->insert(low_bits->end(), this->bitfield->begin() + alg_byte_offset + 1, this->bitfield->begin() + alg_byte_offset + 1 + 3);

	std::vector<tree_node*>* high_bits = new std::vector<tree_node*>();
	high_bits->insert(high_bits->end(), this->bitfield->begin() + alg_byte_offset + 5, this->bitfield->begin() + alg_byte_offset + 5 + 3);

	std::vector<std::vector<tree_node*>*>* all_bits = new std::vector<std::vector<tree_node*>*>();

	all_bits->push_back(and_set2(low_bits->begin(), inv_nibble, 3));
	all_bits->push_back(and_set2(high_bits->begin(), nibble, 3));

	std::vector<tree_node*>* alg_bits = or_condense2(all_bits);

	std::vector<tree_node*>* alg_flags = get_num_flags2(alg_bits->begin());

	return alg_flags;
}

void working_code::process_working(std::vector<tree_node*>* schedule_entry)
{
	process_working(schedule_entry, 0);
}

std::vector<tree_node*>* decrypt_memory2(std::vector<tree_node*>* bitfield, uint8* encrypted_memory, uint8 memory_length)
{
	tree_manager* m = (*bitfield)[0]->manager;

	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	for (int i = 0; i < memory_length; i++)
	{
		uint16 bitfield_offset = (127 - i) * 8;
		std::vector<tree_node*>* temp = xor_bits2(bitfield->begin() + bitfield_offset, m->get_static_byte(encrypted_memory[i])->begin(), 8);
		result->insert(result->end(), temp->begin(), temp->end());
	}

	return result;
}

