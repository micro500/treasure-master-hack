#pragma once

#include "data_sizes.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>

#define CHUNK_SIZE 2048

enum NODE_TYPE
{
	OR = 0,
	AND = 1,
	XOR = 2,
	VAL = 3,
	NOT = 5, // ???
	PRIM = 6,
	PASS = 7
};

class node
{
public:

	std::string get_string();
	void print_tree(int indent);

	//node* simplify(uint64 node_id);
	void simplify_node();
	void simplify_tree();
	virtual	~node();

	void set_node_id_tree(uint64 val);

	uint64 enumerate(uint64 node_id);
	uint64 get_tseitin_terms_tree();
	void get_tseitin_terms_node();
	void clear_node_ids();
	void set_node_ids(uint64 val);
	void get_node_csv();

	std::vector<uint64>* get_node_type_counts_tree();
	uint64 count_refs_tree();

	NODE_TYPE type;
	node();

	uint64 node_id;

	uint64 ref_count;
	
	uint8 val;

	node* left;
	node* right;
};

node* new_node_prim(uint64 val);

node* new_node_bit(uint8 val);

node* new_node(NODE_TYPE type, node* left, node* right);

node* get_node_prim(uint64 val);


void init_primitives(uint32 count);

/*
node* new_node(NODE_TYPE type, node* left, node* right);
node* new_node(NODE_TYPE type, char var, node* right);
node* new_node(NODE_TYPE type, int val, node* right);
node* new_node(NODE_TYPE type, node* left, char var);
node* new_node(NODE_TYPE type, node* left, int val);
node* new_node(NODE_TYPE type, char var1, char var2);
node* new_node(NODE_TYPE type, char var, int val);
node* new_node(NODE_TYPE type, int val, char var);
node* new_node(NODE_TYPE type, int val1, int val2);
node* new_node(char var);
node* new_node(int val);

node* new_node(node*);

void free_tree(node* to_free);

node* remove_xor(node* exp);
node* canon(node* exp);
*/


class node_stack
{
public:
	node* node;
	int state;

	node_stack(class node * node_in, int state);
};


std::vector<node*>* xor_bits(std::vector<node*>::iterator left, std::vector<node*>::iterator right, uint8 bit_count);
std::vector<node*>* add_bits(std::vector<node*>::iterator left, std::vector<node*>::iterator right, node* carry_in, uint8 bit_count);
std::vector<node*>* get_static_byte(uint8 val);
std::vector<node*>* get_static_16bit(uint16 val);
std::vector<node*>* get_num_flags(std::vector<node*>::iterator val_bits);
std::vector<node*>* and_set(std::vector<node*>::iterator val_bits, node* and_node, uint16 bit_count);
std::vector<node*>* or_condense(std::vector<std::vector<node*>*>* node_sets);
node* and_condense(std::vector<node*>* nodes);
void print_vals(std::vector<node*>* nodes);
std::vector<node*>* add_byte_static(std::vector<node*>::iterator left, uint8 val, node* carry_in);
std::vector<node*>* add_16bit_static(std::vector<node*>::iterator left, uint16 val, node* carry_in);
std::vector<node*>* rng_calc_old(std::vector<node*>::iterator rng1, std::vector<node*>::iterator rng2);
std::vector<node*>* rng_calc(std::vector<node*>::iterator rng1, std::vector<node*>::iterator rng2);
node* greater_than_static(std::vector<node*>* val_bitfield, uint64 compare_val, uint8 compare_bits);
std::vector<node*>* decrypt_memory(std::vector<node*>* bitfield, uint8* encrypted_memory, uint8 memory_length);
void set_prims(uint32 prim_count, uint8* values);
std::vector<uint8>* bitfield_to_bytes(std::vector<node*>* bitfield);
node* check_equal(std::vector<node*>* left, std::vector<node*>* right, uint16 bit_count);
node* checksum_bitfield(std::vector<node*>* bitfield);
node* get_desired_node(std::vector<node*>* bitfield, uint8* values);