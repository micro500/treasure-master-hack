#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>
#include "tree_node.h"

#define CHUNK_SIZE 2048

class tree_manager
{
public:
	tree_manager();
	virtual ~tree_manager();

	tree_node* freenode;

	tree_node* get_prim_node(uint64 val);
	tree_node* new_node_bit(uint8 val);
	tree_node* new_node(NODE_TYPE2 type, tree_node* left);
	tree_node* new_node(NODE_TYPE2 type, tree_node* left, tree_node* right);
	tree_node* new_node(NODE_TYPE2 type, std::vector<tree_node*> leaves);

	std::vector<tree_node*>* get_static_byte(uint8 val);

	std::vector<std::vector<int>> get_rng_res_vars(uint32 depth, uint32 bit_index);
	std::vector<std::vector<int>> get_rng_fwd_vars(uint32 depth, uint32 bit_index);

	std::vector<tree_node*>* get_static_16bit(uint16 val);

	void set_prims(uint32 prim_count, int prim_start, uint8* values);
	void set_prims_cnf(uint32 prim_count, int prim_start, uint8* values);

	void init_node_counter();
	uint64 get_next_node_counter();

	uint64 node_counter;

private:
	std::unordered_map<uint64, tree_node*> primitives;
	uint64 primitive_max;
	tree_node* negative;
	tree_node* positive;

	void generate_new_nodes();
	tree_node* get_new_node();
	tree_node* create_prim_node(uint64 val);
	tree_node* create_bit_node(uint8 val);

	std::vector<std::vector<int>> load_cnf(std::string filename);

	void load_rng_res_cnf(uint32 depth, uint32 bit_index);
	std::unordered_map<int, std::unordered_map<int, std::vector<std::vector<int>>>> rng_res_cnf;

	void load_rng_fwd_cnf(uint32 depth, uint32 bit_index);
	std::unordered_map<int, std::unordered_map<int, std::vector<std::vector<int>>>> rng_fwd_cnf;
};

void process_tree(tree_node* root, std::function<bool(tree_node*)> skip_func, std::function<void(tree_node*)> node_proc_func, bool promote_pass);
void set_tree_node_ids(tree_node* root, uint64 val);
void clear_tree_node_ids(tree_node* root);
void print_vals2(std::vector<tree_node*>* nodes);
std::vector<uint8>* bitfield_to_bytes2(std::vector<tree_node*>* bitfield);
std::vector<tree_node*>* get_num_flags2(std::vector<tree_node*>::iterator val_bits);
std::vector<tree_node*>* and_set2(std::vector<tree_node*>::iterator val_bits, tree_node* and_node, uint16 bit_count);
std::vector<tree_node*>* or_condense2(std::vector<std::vector<tree_node*>*>* node_sets);
std::vector<tree_node*>* xor_bits2(std::vector<tree_node*>::iterator left, std::vector<tree_node*>::iterator right, uint8 bit_count);
std::vector<tree_node*>* add_bits2(std::vector<tree_node*>::iterator left, std::vector<tree_node*>::iterator right, tree_node* carry_in, uint8 bit_count);
