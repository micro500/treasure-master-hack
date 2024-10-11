#include "tree_node.h"
#include "tree_manager.h"
#include <iostream>
#include <sstream>
#include <string>

tree_node_stack::tree_node_stack(class tree_node* node_in, int state, std::vector<tree_node*>::iterator it) : 
	node(node_in), state(state), input_it(it) {}


tree_node::tree_node()
{
	type = _XOR;
	val = 0;
	node_id = 0;
	rng_depth = 0;
	rng_bit_index = 0;
	manager = NULL;
}
tree_node::~tree_node() {}

void tree_node::get_cnf()
{
	clear_tree_node_ids(this);
	manager->init_node_counter();

	process_tree(this,
		[](tree_node* n) -> bool {
			return (n->type == _VAL) || (n->type == _PRIM) || (n->node_id != 0);
		},
		[](tree_node* n) -> void {
			n->node_id = n->manager->get_next_node_counter();
			n->get_cnf_node();
		},
		false
	);
}

void tree_node::get_cnf_node()
{
	if (inputs.size() > 2)
	{
		std::cout << "?!?" << std::endl;
	}
	if (type == _AND)
	{
		uint64 a = inputs[0]->node_id;
		uint64 b = inputs[1]->node_id;
		uint64 c = this->node_id;

		std::ostringstream os;

		os << "-" << a << " -" << b << " " << c << " 0\n";
		os << a << " -" << c << " 0\n";
		os << b << " -" << c << " 0\n";

		std::cout << os.str();
	}
	else if (type == _OR)
	{
		uint64 a = inputs[0]->node_id;
		uint64 b = inputs[1]->node_id;
		uint64 c = this->node_id;

		std::ostringstream os;

		os << a << " " << b << " -" << c << " 0\n";
		os << "-" << a << " " << c << " 0\n";
		os << "-" << b << " " << c << " 0\n";

		std::cout << os.str();
	}
	else if (this->type == _NOT)
	{
		uint64 a = inputs[0]->node_id;
		uint64 c = this->node_id;

		std::ostringstream os;

		os << "-" << a << " -" << c << " 0\n";
		os << a << " " << c << " 0\n";

		std::cout << os.str();
	}
	else if (this->type == _XOR)
	{
		uint64 a = inputs[0]->node_id;
		uint64 b = inputs[1]->node_id;
		uint64 c = this->node_id;

		std::ostringstream os;

		os << "-" << a << " -" << b << " -" << c << " 0\n";
		os << a << " " << b << " -" << c << " 0\n";
		os << a << " -" << b << " " << c << " 0\n";
		os << "-" << a << " " << b << " " << c << " 0\n";

		std::cout << os.str();
	}
	else if (type == _RNG_RES)
	{
		/*std::cout << "RNG_RES [" << int(rng_depth) << "," << int(rng_bit_index) << "] ";
		for (int i = 0; i < 16; i++)
		{
			std::cout << inputs[i]->node_id << " ";
		}
		std::cout << "-> " << node_id << std::endl;
		return;*/

		std::vector<std::vector<int>> rng_vars = manager->get_rng_res_vars(rng_depth, rng_bit_index);
		process_cnf(rng_vars);
	}
	else if (type == _RNG_FWD)
	{
		/*std::cout << "RNG_RES [" << int(rng_depth) << "," << int(rng_bit_index) << "] ";
		for (int i = 0; i < 16; i++)
		{
			std::cout << inputs[i]->node_id << " ";
		}
		std::cout << "-> " << node_id << std::endl;
		return;*/

		std::vector<std::vector<int>> rng_vars = manager->get_rng_fwd_vars(rng_depth, rng_bit_index);
		process_cnf(rng_vars);
	}
	else
	{
		std::cout << "??? " << type << std::endl;
	}
}

void tree_node::process_cnf(std::vector<std::vector<int>> &vars)
{
	std::ostringstream os;
	for (auto line_it = vars.begin(); line_it != vars.end(); line_it++)
	{
		for (auto vars_it = line_it->begin(); vars_it != line_it->end(); vars_it++)
		{
			int var = *vars_it;
			int var_index = abs(var);

			if (var < 0)
			{
				os << "-";
			}

			if (var_index == 17)
			{
				os << node_id;
			}
			else
			{
				os << inputs[var_index - 1]->node_id;
			}
			os << " ";
		}
		os << "0" << std::endl;
	}
	std::cout << os.str();
}

void tree_node::simplify_tree()
{
	clear_tree_node_ids(this);
	manager->init_node_counter();

	process_tree(this,
		[](tree_node* n) -> bool {
			return (n->type == _VAL) || (n->type == _PRIM) || (n->node_id != 0);
		},
		[](tree_node* n) -> void {
			n->node_id = n->manager->get_next_node_counter();
			n->simplify_node();
		},
		true
	);
}

void tree_node::convert_to_val_pass(uint8 val)
{
	inputs.clear();
	inputs.push_back(manager->new_node_bit(val));
	type = _PASS;
}

void tree_node::convert_to_pass()
{
	type = _PASS;
}

void tree_node::convert_to_not()
{
	type = _NOT;
}

void tree_node::promote_and_pass()
{
	tree_node* temp = inputs[0]->inputs[0];
	inputs.clear();
	inputs.push_back(temp);
	convert_to_pass();
}

void tree_node::simplify_node()
{
	if (this->type == _PASS || this->type == _VAL || this->type == _PRIM)
	{
		return;
	}
	else if (this->type == _OR)
	{
		auto it = inputs.begin();
		while (it != inputs.end())
		{
			if ((*it)->type == _VAL && (*it)->val == 0)
			{
				it = inputs.erase(it);
			}
			else if ((*it)->type == _VAL && (*it)->val == 1)
			{
				convert_to_val_pass(1);
				return;
			}
			else
			{
				it++;
			}
		}
		
		if (inputs.size() == 0)
		{
			convert_to_val_pass(0);
		}
		else if (inputs.size() == 1)
		{
			convert_to_pass();
		}

		return;
	}
	else if (this->type == _AND)
	{
		int res_val = 0;
		auto it = inputs.begin();
		while (it != inputs.end())
		{
			if ((*it)->type == _VAL && (*it)->val == 1)
			{
				res_val = 1;
				it = inputs.erase(it);
			}
			else if ((*it)->type == _VAL && (*it)->val == 0)
			{
				convert_to_val_pass(0);
				return;
			}
			else
			{
				it++;
			}
		}

		if (inputs.size() == 0)
		{
			convert_to_val_pass(res_val);
		}
		else if (inputs.size() == 1)
		{
			convert_to_pass();
		}

		return;
	}
	else if (this->type == _XOR)
	{
		uint8 res_val = 0;
		auto it = inputs.begin();
		while (it != inputs.end())
		{
			if ((*it)->type == _VAL)
			{
				res_val = res_val ^ (*it)->val;
				it = inputs.erase(it);
			}
			else
			{
				it++;
			}
		}

		if (inputs.size() == 0)
		{
			convert_to_val_pass(res_val);
		}
		else if (inputs.size() == 1)
		{
			if (res_val == 1)
			{
				convert_to_not();
				this->simplify_node();
			}
			else
			{
				convert_to_pass();
			}
		}
		else
		{
			// Are any nodes a not? Promote it to invert it
		}
	}
	else if (this->type == _NOT)
	{
		if (inputs.size() == 0)
		{
			convert_to_val_pass(0);
		}
		else if (inputs[0]->type == _NOT)
		{
			promote_and_pass();
		}
		else if (inputs[0]->type == _VAL)
		{
			convert_to_val_pass(1 - inputs[0]->val);
		}
	}
}
