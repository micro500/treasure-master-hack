#ifndef TREE_H
#define TREE_H

#include <string>
#include <vector>
#include <algorithm>

#include "data_sizes.h"

#define CHUNK_SIZE 2048

enum NODE_TYPE
{
	NODE_WC = 0,
	NODE_RNG = 1,
	NODE_OP = 2,
	NODE_VAL = 3,
	NODE_UNKNOWN = 4
};

enum OP_TYPE
{
	OP_OR = 0,
	OP_AND = 1,
	OP_NOT = 2,
	OP_XOR = 3
};

class node
{
public:

	std::string to_string();

	void eval(uint16 rng_seed, int last_rng_pos);

	//node* simplify();
	virtual	~node();

	
	node();
	
	node(NODE_TYPE, OP_TYPE, node *, node *);
	/*
	node(NODE_TYPE,char, node *);
	node(NODE_TYPE,int, node *);
	node(NODE_TYPE,node *, char);
	node(NODE_TYPE,node *, int);
	node(NODE_TYPE,char, char);
	node(NODE_TYPE,char, int);
	node(NODE_TYPE,int, char);
	node(NODE_TYPE,int, int);
	node(char);
	*/
	node(bool);
	node(NODE_TYPE, int, uint8);

	node(node *);


	//void assign_var(char var, int val);
	//std::vector<char> get_vars();

	// DATA
	NODE_TYPE node_type;

	// only used if node_type is OP
	OP_TYPE op_type;

	// only used if node_type is WC or RNG
	int byte_pos;
	uint8 bit_pos;
	bool value;

	node *left;
	node *right;
};

/*
node * new_node(NODE_TYPE type, node * left, node * right);
node * new_node(NODE_TYPE type, char var, node * right);
node * new_node(NODE_TYPE type, int val, node * right);
node * new_node(NODE_TYPE type, node * left, char var);
node * new_node(NODE_TYPE type, node * left, int val);
node * new_node(NODE_TYPE type, char var1, char var2);
node * new_node(NODE_TYPE type, char var, int val);
node * new_node(NODE_TYPE type, int val, char var);
node * new_node(NODE_TYPE type, int val1, int val2);
node * new_node(char var);
node * new_node(int val);

node * new_node(node *);

void free_tree(node * to_free);

node * remove_xor(node * exp);
node * canon(node * exp);
*/

#endif //TREE_H