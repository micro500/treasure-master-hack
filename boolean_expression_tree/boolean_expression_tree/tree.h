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
	node(NODE_TYPE,int, node *);
	node(NODE_TYPE,node *, char);
	node(NODE_TYPE,node *, int);
	node(NODE_TYPE,char, char);
	node(NODE_TYPE,char, int);
	node(NODE_TYPE,int, char);
	node(NODE_TYPE,int, int);
	node(char);
	node(int);
	*/

	/*
	// Recursive copy constructor
	node (node *);
	*/

	void assign_var(char var, int val);
	std::vector<char> get_vars();

	char var;
	int val;

	node *left;
	node *right;
};

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