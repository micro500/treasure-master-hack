#include <sstream>
#include <fstream>
#include <iostream>
#include "tree_manager.h"

tree_manager::tree_manager()
{
	primitive_max = 0;
	negative = create_bit_node(0);
	positive = create_bit_node(1);
}
tree_manager::~tree_manager() {}


void tree_manager::generate_new_nodes()
{
	freenode = new tree_node[CHUNK_SIZE];
	for (int i = 0; i < CHUNK_SIZE; i++)
	{
		freenode[i].inputs.push_back(&freenode[i + 1]);
		freenode[i].manager = this;
	}
	freenode[CHUNK_SIZE - 1].inputs.clear();
}

tree_node* tree_manager::get_new_node()
{
	if (freenode == NULL)
	{
		generate_new_nodes();
	}
	tree_node* temp = freenode;
	if (freenode->inputs.size() == 0)
	{
		freenode = NULL;
	}
	else
	{
		freenode = freenode->inputs[0];
	}
	temp->inputs.clear();
	temp->node_id = 0;

	return temp;
}

tree_node* tree_manager::create_prim_node(uint64 val)
{
	tree_node* result = get_new_node();
	result->type = _PRIM;
	result->node_id = val;

	primitives[val] = result;

	if (val > primitive_max)
	{
		primitive_max = val;
	}

	return result;
}

tree_node* tree_manager::create_bit_node(uint8 val)
{
	tree_node* result = get_new_node();
	result->type = _VAL;

	result->val = (val & 1);

	return result;
}

tree_node* tree_manager::get_prim_node(uint64 val)
{
	auto f = primitives.find(val);
	if (f == primitives.end())
	{
		return create_prim_node(val);
	}

	return f->second;
}

tree_node* tree_manager::new_node_bit(uint8 val)
{
	if ((val & 0x01) == 0x01)
	{
		return positive;
	}
	else
	{
		return negative;
	}
}

tree_node* tree_manager::new_node(NODE_TYPE2 type, tree_node* left)
{
	return new_node(type, left, NULL);
}

tree_node* tree_manager::new_node(NODE_TYPE2 type, tree_node* left, tree_node* right)
{
	tree_node* result = get_new_node();
	result->type = type;
	if (left != NULL)
	{
		result->inputs.push_back(left);
	}
	if (right != NULL)
	{
		result->inputs.push_back(right);
	}

	return result;
}

tree_node* tree_manager::new_node(NODE_TYPE2 type, std::vector<tree_node*> leaves)
{
	tree_node* result = get_new_node();
	result->type = type;

	result->inputs.insert(result->inputs.end(), leaves.begin(), leaves.end());

	return result;
}

std::vector<std::vector<int>> tree_manager::load_cnf(std::string filename)
{
	std::vector<std::vector<int>> result;

	std::ifstream file(filename);
	std::string line;
	while (std::getline(file, line))
	{
		std::vector<int> cur_vars;

		size_t last = 0;
		size_t next = 0;
		while ((next = line.find(" ", last)) != std::string::npos)
		{
			cur_vars.push_back(std::stoi(line.substr(last, next - last)));
			last = next + 1;
		}

		// skip the final 0
		//std::cout << line.substr(last) << std::endl;

		result.push_back(cur_vars);
	}

	return result;
}

void tree_manager::load_rng_res_cnf(uint32 depth, uint32 bit_index)
{
	std::ostringstream os;
	os << "..\\rng_qmc\\rng_res_" << depth << "_" << bit_index << ".txt";
	std::string s = os.str();

	auto &depth_map = rng_res_cnf[depth];
	auto& bit_map = rng_res_cnf[depth][bit_index];
	bit_map = load_cnf(s);
}

std::vector<std::vector<int>> tree_manager::get_rng_res_vars(uint32 depth, uint32 bit_index)
{
	if (rng_res_cnf.find(depth) == rng_res_cnf.end() || rng_res_cnf[depth].find(bit_index) == rng_res_cnf[depth].end())
	{
		load_rng_res_cnf(depth, bit_index);
	}

	return rng_res_cnf[depth][bit_index];
}

void tree_manager::load_rng_fwd_cnf(uint32 depth, uint32 bit_index)
{
	std::ostringstream os;
	os << "..\\rng_qmc\\rng_fwd_" << depth << "_" << bit_index << ".txt";
	std::string s = os.str();

	auto& depth_map = rng_fwd_cnf[depth];
	auto& bit_map = rng_fwd_cnf[depth][bit_index];
	bit_map = load_cnf(s);
}

std::vector<std::vector<int>> tree_manager::get_rng_fwd_vars(uint32 depth, uint32 bit_index)
{
	if (rng_fwd_cnf.find(depth) == rng_fwd_cnf.end() || rng_fwd_cnf[depth].find(bit_index) == rng_fwd_cnf[depth].end())
	{
		load_rng_fwd_cnf(depth, bit_index);
	}

	return rng_fwd_cnf[depth][bit_index];
}

void tree_manager::init_node_counter()
{
	node_counter = primitive_max + 1;
}

uint64 tree_manager::get_next_node_counter()
{
	uint64 prev = node_counter;
	node_counter++;
	return prev;
}

std::vector<tree_node*>* tree_manager::get_static_16bit(uint16 val)
{
	std::vector<tree_node*>* val_bits = new std::vector<tree_node*>();
	for (int i = 0; i < 16; i++)
	{
		val_bits->push_back(new_node_bit((val >> i) & 1));
	}

	return val_bits;
}

void process_tree(tree_node* root, std::function<bool(tree_node*)> skip_func, std::function<void(tree_node*)> node_proc_func, bool promote_pass)
{
	std::vector<tree_node_stack> stack;

	tree_node_stack cur_state = tree_node_stack(root, 0, root->inputs.begin());

	while (true)
	{
		// check if we should skip the current node
		if (skip_func(cur_state.node))
		{
			cur_state.state = 2;
		}

		if (cur_state.state == 0)
		{
			// processing inputs...
			if (cur_state.input_it != cur_state.node->inputs.end())
			{
				stack.push_back(cur_state);

				cur_state.node = *(cur_state.input_it);
				cur_state.state = 0;
				cur_state.input_it = cur_state.node->inputs.begin();
				continue;
			}
			else
			{
				cur_state.state = 1;
			}
		}

		if (cur_state.state == 1)
		{
			// processing current node
			node_proc_func(cur_state.node);
			cur_state.state = 2;
		}

		if (cur_state.state == 2)
		{
			// pop

			if (stack.empty())
			{
				break;
			}
			cur_state = stack.back();
			stack.pop_back();

			if (promote_pass)
			{
				if ((*(cur_state.input_it))->type == _PASS)
				{
					*(cur_state.input_it) = (*(cur_state.input_it))->inputs[0];
				}
			}
			cur_state.input_it++;
		}
	}

	return;
}

void set_tree_node_ids(tree_node* root, uint64 val)
{
	process_tree(root,
		[val](tree_node* n) -> bool {
			return (n->type == _PRIM) || (n->node_id == val);
		}, 
		[val](tree_node* n) -> void {
			n->node_id = val;
		},
		false
	);
}

void clear_tree_node_ids(tree_node* root)
{
	set_tree_node_ids(root, INT_MAX);
	set_tree_node_ids(root, 0);
}

std::vector<tree_node*>* tree_manager::get_static_byte(uint8 val)
{
	std::vector<tree_node*>* val_bits = new std::vector<tree_node*>();
	for (int i = 0; i < 8; i++)
	{
		val_bits->push_back(new_node_bit((val >> i) & 1));
	}

	return val_bits;
}

void tree_manager::set_prims(uint32 prim_count, int prim_start, uint8* values)
{
	for (int i = 0; i < prim_count; i++)
	{
		uint32 value_offset = i / 8;
		uint8 bit_offset = i % 8;

		uint8 bit_val = (values[value_offset] >> bit_offset) & 0x01;

		tree_node* t = get_prim_node(prim_start + i);
		t->type = _VAL;
		t->val = bit_val;
		t->node_id = 0;
	}
}

void tree_manager::set_prims_cnf(uint32 prim_count, int prim_start, uint8* values)
{
	for (int i = 0; i < prim_count; i++)
	{
		uint32 value_offset = i / 8;
		uint8 bit_offset = i % 8;

		uint8 bit_val = (values[value_offset] >> bit_offset) & 0x01;

		if (bit_val == 1)
		{
			std::cout << (prim_start + i) << " 0" << std::endl;
		}
		else
		{
			std::cout << (-(prim_start + i)) << " 0" << std::endl;
		}
	}
}

void print_vals2(std::vector<tree_node*>* nodes)
{
	for (int i = (*nodes).size() - 1; i >= 0; i--)
	{
		tree_node* x = (*nodes)[i];
		x->simplify_tree();

		if (x->type == _PASS)
		{
			x = x->inputs[0];
		}

		if (x->type != _VAL)
		{
			std::cout << i << " ???\n";
		}
		else
		{
			std::cout << ((int)x->val) << "";
		}
	}
}

std::vector<uint8>* bitfield_to_bytes2(std::vector<tree_node*>* bitfield)
{
	std::vector<uint8>* result = new std::vector<uint8>();

	uint8 cur_byte = 0;
	uint8 bit_count = 0;

	for (auto it = bitfield->begin(); it != bitfield->end(); it++)
	{
		tree_node* x = (*it);
		x->simplify_tree();

		if (x->type == _PASS)
		{
			x = x->inputs[0];
		}

		uint8 bit_val = 0;
		if (x->type == _VAL)
		{
			bit_val = x->val & 0x01;
		}

		cur_byte = cur_byte | (bit_val << bit_count);
		bit_count++;
		if (bit_count == 8)
		{
			result->push_back(cur_byte);
			cur_byte = 0;
			bit_count = 0;
		}
	}

	if (bit_count != 0)
	{
		result->push_back(cur_byte);
	}

	return result;
}

std::vector<tree_node*>* get_num_flags2(std::vector<tree_node*>::iterator val_bits)
{
	tree_manager* m = (*val_bits)->manager;

	tree_node* bit0 = *val_bits;
	tree_node* bit1 = *(val_bits + 1);
	tree_node* bit2 = *(val_bits + 2);

	tree_node* inv_bit0 = m->new_node(_NOT, bit0, NULL);
	tree_node* inv_bit1 = m->new_node(_NOT, bit1, NULL);
	tree_node* inv_bit2 = m->new_node(_NOT, bit2, NULL);

	std::vector<tree_node*>* result = new std::vector<tree_node*>();
	result->push_back(m->new_node(_AND, m->new_node(_AND, inv_bit2, inv_bit1), inv_bit0));
	result->push_back(m->new_node(_AND, m->new_node(_AND, inv_bit2, inv_bit1), bit0));
	result->push_back(m->new_node(_AND, m->new_node(_AND, inv_bit2, bit1), inv_bit0));
	result->push_back(m->new_node(_AND, m->new_node(_AND, inv_bit2, bit1), bit0));
	result->push_back(m->new_node(_AND, m->new_node(_AND, bit2, inv_bit1), inv_bit0));
	result->push_back(m->new_node(_AND, m->new_node(_AND, bit2, inv_bit1), bit0));
	result->push_back(m->new_node(_AND, m->new_node(_AND, bit2, bit1), inv_bit0));
	result->push_back(m->new_node(_AND, m->new_node(_AND, bit2, bit1), bit0));

	return result;
}

std::vector<tree_node*>* and_set2(std::vector<tree_node*>::iterator val_bits, tree_node* and_node, uint16 bit_count)
{
	tree_manager* m = (*val_bits)->manager;
	std::vector<tree_node*>* result = new std::vector<tree_node*>();

	for (int i = 0; i < bit_count; i++)
	{
		result->push_back(m->new_node(_AND, *(val_bits + i), and_node));
	}

	return result;
}

std::vector<tree_node*>* or_condense2(std::vector<std::vector<tree_node*>*>* node_sets)
{
	std::vector<tree_node*>* result = (*node_sets)[0];
	uint16 bit_count = result->size();

	tree_manager* m = (*result)[0]->manager;

	for (auto it = node_sets->begin() + 1; it != node_sets->end(); it++)
	{
		std::vector<tree_node*>* temp = new std::vector<tree_node*>();
		for (int i = 0; i < bit_count; i++)
		{
			temp->push_back(m->new_node(_OR, (*(*it))[i], (*result)[i]));
		}

		result = temp;
	}

	return result;
}

std::vector<tree_node*>* xor_bits2(std::vector<tree_node*>::iterator left, std::vector<tree_node*>::iterator right, uint8 bit_count)
{
	tree_manager* m = (*left)->manager;
	std::vector<tree_node*>* result = new std::vector<tree_node*>();

	for (int i = 0; i < bit_count; i++)
	{
		tree_node* left_bit = *(left + i);
		tree_node* right_bit = *(right + i);

		result->push_back(m->new_node(_XOR, left_bit, right_bit));
	}

	return result;
}

std::vector<tree_node*>* add_bits2(std::vector<tree_node*>::iterator left, std::vector<tree_node*>::iterator right, tree_node* carry_in, uint8 bit_count)
{
	tree_manager* m = (*left)->manager;
	std::vector<tree_node*>* result = new std::vector<tree_node*>();

	tree_node* carry = carry_in;

	for (int i = 0; i < bit_count; i++)
	{
		tree_node* left_bit = *(left + i);
		tree_node* right_bit = *(right + i);

		tree_node* g = m->new_node(_AND, left_bit, right_bit);
		tree_node* p = m->new_node(_OR, left_bit, right_bit);
		tree_node* out = m->new_node(_XOR, m->new_node(_XOR, left_bit, right_bit), carry);

		carry = m->new_node(_OR, g, m->new_node(_AND, p, carry));

		result->push_back(out);
	}

	result->push_back(carry);

	return result;
}
