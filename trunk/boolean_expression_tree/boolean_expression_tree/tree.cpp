#include "tree.h"

node * freenode;

void generate_new_nodes()
{
	freenode = new node[CHUNK_SIZE];
	for (int i = 0; i < CHUNK_SIZE; i++)
	{
		freenode[i].right = &freenode[i+1];
	}
	freenode[CHUNK_SIZE - 1].right = NULL;
}

node * get_new_node()
{
	if (freenode == NULL)
	{
		generate_new_nodes();
	}
	node * temp = freenode;
	freenode = freenode->right;
	temp->left = NULL;
	temp->right = NULL;

	return temp;
}

void free_node(node * to_free)
{
	to_free->left = NULL;
	to_free->right = freenode;
	freenode = to_free;
}

void free_tree(node * to_free)
{
	if (to_free->left)
		free_tree(to_free->left);
	if (to_free->right)
		free_tree(to_free->right);

	free_node(to_free);
}

node * remove_xor(node * exp)
{
	switch (exp->type)
	{
	case AND:
	case OR:
		exp->right = remove_xor(exp->right);
	case NOT:
		exp->left = remove_xor(exp->left);
		return exp;

	case XOR:
		{
		exp->left = remove_xor(exp->left);
		exp->right = remove_xor(exp->right);

		node * new_left = new_node(AND,exp->left,new_node(NOT,new_node(exp->right),(node*)NULL));
		node * new_right = new_node(AND,new_node(NOT,new_node(exp->left),(node*)NULL),exp->right);

		exp->type = OR;
		exp->left = new_left;
		exp->right = new_right;
		return exp;
		}

	default:
		return exp;
	}
}

node * canon(node * exp)
{
	node * temp;

	switch (exp->type)
	{
	case AND:
	case OR:
		exp->left = canon(exp->left);
		exp->right = canon(exp->right);
		return exp;

	case NOT:
		switch(exp->left->type)
		{
		case NOT:
			// Double negative
			temp = exp->left->left;
			free_node(exp->left);
			free_node(exp);
			temp = canon(temp);
			return temp;

		case VAL:
			exp->left->val = !exp->left->val;
			temp = exp->left;
			free_node(exp);
			return temp;

		case AND:
			exp->type = OR;
			exp->right = new_node(NOT,exp->left->right,(node*)NULL);
			exp->left->right = NULL;
			exp->left->type = NOT;

			exp->left = canon(exp->left);
			exp->right = canon(exp->right);
			return exp;

		case OR:
			exp->type = AND;
			exp->right = new_node(NOT,exp->left->right,(node*)NULL);
			exp->left->right = NULL;
			exp->left->type = NOT;

			exp->left = canon(exp->left);
			exp->right = canon(exp->right);
			return exp;	
		default:
			return exp;
		}

	default:
		return exp;
		
	}
}

node * new_node(NODE_TYPE type, node * left, node * right)
{
	node * result = get_new_node();
	result->type = type;
	result->left = left;
	result->right = right;

	return result;
}

node * new_node(NODE_TYPE type, char var, node * right)
{
	node * result = get_new_node();
	result->type = type;
	result->left = new_node(var);
	result->right = right;

	return result;
}

node * new_node(NODE_TYPE type, bool val, node * right)
{
	node * result = get_new_node();
	result->type = type;
	result->left = new_node(val);
	result->right = right;

	return result;
}

node * new_node(NODE_TYPE type, node * left, char var)
{
	node * result = get_new_node();
	result->type = type;
	result->left = left;
	result->right = new_node(var);

	return result;
}

node * new_node(NODE_TYPE type, node * left, bool val)
{
	node * result = get_new_node();
	result->type = type;
	result->left = left;
	result->right = new_node(val);

	return result;
}

node * new_node(NODE_TYPE type, char var1, char var2)
{
	node * result = get_new_node();
	result->type = type;
	result->left = new_node(var1);
	result->right = new_node(var2);

	return result;
}

node * new_node(NODE_TYPE type, char var, bool val)
{
	node * result = get_new_node();
	result->type = type;
	result->left = new_node(var);
	result->right = new_node(val);

	return result;
}

node * new_node(NODE_TYPE type, bool val, char var)
{
	node * result = get_new_node();
	result->type = type;
	result->left = new_node(val);
	result->right = new_node(var);

	return result;
}

node * new_node(NODE_TYPE type, bool val1, bool val2)
{
	node * result = get_new_node();
	result->type = type;
	result->left = new_node(val1);
	result->right = new_node(val2);

	return result;
}

node * new_node(char var)
{
	node * result = get_new_node();
	result->type = VAR;
	result->var = var;

	result->left = NULL;
	result->right = NULL;

	return result;
}

node * new_node(bool val)
{
	node * result = get_new_node();
	result->type = VAL;
	result->val = val;

	result->left = NULL;
	result->right = NULL;

	return result;
}

// Recursive copy
node * new_node(node * original)
{
	node * result = get_new_node();
	result->type = original->type;
	result->val = original->val;
	result->var = original->var;

	if (original->left)
		result->left = new_node(original->left);
	if (original->right)
		result->right = new_node(original->right);

	return result;
}



node::node()
{
	this->type = VAL;
	this->var = NULL;
	this->val = true;
	this->left = NULL;
	this->right = NULL;
}

/*
node::node(NODE_TYPE type, node * left, node * right)
{
	this->type = type;
	this->left = left;
	this->right = right;
}

node::node(NODE_TYPE type, char var, node * right)
{
	this->type = type;
	this->left = new node(var);
	this->right = right;
}

node::node(NODE_TYPE type, bool val, node * right)
{
	this->type = type;
	this->left = new node(val);
	this->right = right;
}

node::node(NODE_TYPE type, node * left, char var)
{
	this->type = type;
	this->left = left;
	this->right = new node(var);
}

node::node(NODE_TYPE type, node * left, bool val)
{
	this->type = type;
	this->left = left;
	this->right = new node(val);
}

node::node(NODE_TYPE type, char var1, char var2)
{
	this->type = type;
	this->left = new node(var1);
	this->right = new node(var2);
}

node::node(NODE_TYPE type, char var, bool val)
{
	this->type = type;
	this->left = new node(var);
	this->right = new node(val);
}

node::node(NODE_TYPE type, bool val, char var)
{
	this->type = type;
	this->left = new node(val);
	this->right = new node(var);
}

node::node(NODE_TYPE type, bool val1, bool val2)
{
	this->type = type;
	this->left = new node(val1);
	this->right = new node(val2);
}

node::node(char var)
{
	this->type = VAR;
	this->var = var;

	this->left = 0;
	this->right = 0;
}

node::node(bool val)
{
	this->type = VAL;
	this->val = val;

	this->left = 0;
	this->right = 0;
}
*/

node::~node()
{
	if (this->type != VAL && this->type != VAR)
	{
		delete this->left;
		delete this->right;
	}

	//delete this;
}

/*
node::node(node * original)
{
	this->type = original->type;
	if (original->type == VAL)
	{
		this->val = original->val;
	}
	else if (original->type == VAR)
	{
		this->var = original->var;
	}
	else
	{
		this->left = new node(original->left);
		this->right = new node(original->right);
	}
}
*/

std::string node::get_string()
{
	if (this->type == VAR)
	{
		return std::string(1,this->var);
	}
	else if (this-> type == VAL)
	{
		return std::string(1,this->val ? '1' : '0');
	}
	else
	{
		std::string left_val = this->left->get_string();
		std::string right_val; 
		if (this->type != NOT)
			right_val = this->right->get_string();

		std::string joiner;
		switch (this->type)
		{
		case OR:
			joiner = "|";
			return "(" + left_val + " " + joiner + " " + right_val + ")";

		case AND:
			joiner = "&";
			return "(" + left_val + " " + joiner + " " + right_val + ")";

		case XOR:
			joiner = "^";
			return "(" + left_val + " " + joiner + " " + right_val + ")";

		case NOT:
			return "!" + left_val;
		}

		
	}
}

void node::assign_var(char var, bool val)
{
	if (this->type == VAL)
	{
		return;
	}
	else if (this->type == VAR)
	{
		if (this->var == var)
		{
			this->type = VAL;
			this->val = val;
		} 
	}
	else 
	{
		this->left->assign_var(var,val);
		if (this->type != NOT)
			this->right->assign_var(var,val);
	}
}

std::vector<char> node::get_vars()
{
	if(this->type == VAL)
	{
		return std::vector<char>();
	}
	else if (this->type == VAR)
	{
		return std::vector<char>(1,this->var);
	}
	else
	{
		std::vector<char> vars = this->left->get_vars();
		std::vector<char> right_vars;
		if (this->type != NOT)
		{
			right_vars = this->right->get_vars();
			vars.insert(vars.end(),right_vars.begin(),right_vars.end());
		}
		
		return vars;
	}
}


node* node::simplify()
{
	if (this->type == VAR || this->type == VAL)
	{
		return this;
	}
	else
	{
		this->left = this->left->simplify();
		if (this->type != NOT)
			this->right = this->right->simplify();

		if (this->type == AND)
		{
			if ((this->left->type == VAL && this->left->val == false) || (this->right->type == VAL && this->right->val == false))
			{
				free_tree(this->left);
				this->left = NULL;
				free_tree(this->right);
				this->right = NULL;

				this->type = VAL;
				this->val = false;
				return this;
			}
			else if (this->left->type == VAL && this->left->val == true)
			{
				// this needs to become right
				//delete this->left;
				node * temp = new_node(this->right);
				free_tree(this); // does this work??
				
				return temp;
			}
			else if (this->right->type == VAL && this->right->val == true)
			{
				// this needs to become left
				//delete this->right;
				node * temp = new_node(this->left);
				free_tree(this); // does this work??

				return temp;
			}
		}
		else if (this->type == OR)
		{
			if ((this->left->type == VAL && this->left->val == true) || (this->right->type == VAL && this->right->val == true))
			{
				free_tree(this->left);
				this->left = NULL;
				free_tree(this->right);
				this->right = NULL;

				this->type = VAL;
				this->val = true;
			}
			else if (this->left->type == VAL && this->left->val == false)
			{
				// this needs to become right	
				//delete this->left;
				node * temp = new_node(this->right);

				free_tree(this); // does this work??
				return temp;
			}
			else if (this->right->type == VAL && this->right->val == false)
			{
				// this needs to become left
				//delete this->right;
				node * temp = new_node(this->left);

				free_tree(this); // does this work??
				return temp;
			}
		}
		else if (this->type == XOR)
		{
			if (this->left->type == VAL && this->left->val == false)
			{
				// this needs to become right	
				//delete this->left;
				node * temp = new_node(this->right);

				free_tree(this); // does this work??
				return temp;
			}
			else if (this->right->type == VAL && this->right->val == false)
			{
				// this needs to become left
				//delete this->right;
				node * temp = new_node(this->left);

				free_tree(this); // does this work??
				return temp;
			}
			else if (this->left->type == VAL && this->left->val == true && this->right->type == VAL && this->right->val == true)
			{
				//delete this->left;
				//delete this->right;
				free_tree(this); // does this work??
				return new_node(false);
			}
			else if (this->left->type == VAL && this->left->val == false && this->right->type == VAL && this->right->val == false)
			{
				//delete this->left;
				//delete this->right;
				free_tree(this); // does this work??
				return new_node(false);
			}
		}
		else if (this->type == NOT)
		{
			if (this->left->type == VAL)
			{
				// Invert the value at left and return that node instead
				node * temp = this->left;
				temp->val = !temp->val;

				free_node(this); // does this work??
				return temp;
			}
		}

		return this;
	}
}