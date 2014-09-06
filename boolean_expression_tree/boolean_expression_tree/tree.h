#pragma once

#include <string>
#include <vector>
#include <algorithm>

#define CHUNK_SIZE 2048

enum NODE_TYPE
{
	OR = 0,
	AND = 1,
	XOR = 2,
	VAL = 3,
	VAR = 4,
	NOT = 5
};

class node
{
public:

	std::string get_string();

	node* simplify();
	virtual	~node();

	NODE_TYPE type;
	node();
	/*
	node(NODE_TYPE,node *, node *);
	node(NODE_TYPE,char, node *);
	node(NODE_TYPE,bool, node *);
	node(NODE_TYPE,node *, char);
	node(NODE_TYPE,node *, bool);
	node(NODE_TYPE,char, char);
	node(NODE_TYPE,char, bool);
	node(NODE_TYPE,bool, char);
	node(NODE_TYPE,bool, bool);
	node(char);
	node(bool);
	*/

	/*
	// Recursive copy constructor
	node (node *);
	*/

	void assign_var(char var, bool val);
	std::vector<char> get_vars();

	char var;
	bool val;

	node *left;
	node *right;
};

node * new_node(NODE_TYPE type, node * left, node * right);
node * new_node(NODE_TYPE type, char var, node * right);
node * new_node(NODE_TYPE type, bool val, node * right);
node * new_node(NODE_TYPE type, node * left, char var);
node * new_node(NODE_TYPE type, node * left, bool val);
node * new_node(NODE_TYPE type, char var1, char var2);
node * new_node(NODE_TYPE type, char var, bool val);
node * new_node(NODE_TYPE type, bool val, char var);
node * new_node(NODE_TYPE type, bool val1, bool val2);
node * new_node(char var);
node * new_node(bool val);

node * new_node(node *);

void free_tree(node * to_free);

node * remove_xor(node * exp);
node * canon(node * exp);