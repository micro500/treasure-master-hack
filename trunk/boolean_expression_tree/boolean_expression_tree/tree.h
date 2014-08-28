#pragma once

#include <string>
#include <vector>
#include <algorithm>

enum NODE_TYPE
{
	OR = 0,
	AND = 1,
	XOR = 2,
	VAL = 3,
	VAR = 4
};

class node
{
public:

	std::string get_string();

	node* simplify();
	virtual	~node();

	NODE_TYPE type;
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

	// Recursive copy constructor
	node (node *);

	void assign_var(char var, bool val);
	std::vector<char> get_vars();

	char var;
	bool val;

	node *left;
	node *right;
};