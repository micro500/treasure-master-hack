#pragma once
#include <vector>
#include "data_sizes.h"
//#include "tree_manager.h"

class tree_manager;

enum NODE_TYPE2
{
	_OR = 0,
	_AND = 1,
	_XOR = 2,
	_VAL = 3,
	_NOT = 5,
	_PRIM = 6,
	_PASS = 7,
	_RNG_RES = 8,
	_RNG_FWD = 9
};

class tree_node
{
public:
	tree_node();
	virtual ~tree_node();

	NODE_TYPE2 type;

	uint64 node_id;

	uint8 val;

	uint8 rng_depth;
	uint8 rng_bit_index;

	std::vector<tree_node*> inputs;

	tree_manager* manager;

	void convert_to_pass();
	void convert_to_not();
	void promote_and_pass();
	void convert_to_val_pass(uint8 val);

	void get_cnf();
	void get_cnf_node();

	void simplify_tree();
	void simplify_node();

	void process_cnf(std::vector<std::vector<int>>& vars);
};

class tree_node_stack
{
public:
	tree_node* node;
	int state;

	std::vector<tree_node*>::iterator input_it;

	tree_node_stack(class tree_node* node_in, int state, std::vector<tree_node*>::iterator it);
};