#include "tree.h"

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

node::~node()
{
	if (this->type != VAL && this->type != VAR)
	{
		delete this->left;
		delete this->right;
	}

	//delete this;
}

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
		std::string right_val = this->right->get_string();

		std::string joiner;
		if (this->type == OR)
		{
			joiner = "|";
		}
		else if (this->type == AND)
		{
			joiner = "&";
		}
		else if (this->type == XOR)
		{
			joiner = "^";
		}

		return "(" + left_val + " " + joiner + " " + right_val + ")";
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
		std::vector<char> right_vars = this->right->get_vars();
		vars.insert(vars.end(),right_vars.begin(),right_vars.end());
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
		this->right = this->right->simplify();

		if (this->type == AND)
		{
			if ((this->left->type == VAL && this->left->val == false) || (this->right->type == VAL && this->right->val == false))
			{
				delete this->left;
				this->left = NULL;
				delete this->right;
				this->right = NULL;

				this->type = VAL;
				this->val = false;
				return this;
			}
			else if (this->left->type == VAL && this->left->val == true)
			{
				// this needs to become right
				//delete this->left;
				node * temp = new node(this->right);
				delete this; // does this work??
				
				return temp;
			}
			else if (this->right->type == VAL && this->right->val == true)
			{
				// this needs to become left
				//delete this->right;
				node * temp = new node(this->left);
				delete this; // does this work??

				return temp;
			}
		}
		else if (this->type == OR)
		{
			if ((this->left->type == VAL && this->left->val == true) || (this->right->type == VAL && this->right->val == true))
			{
				delete this->left;
				this->left = NULL;
				delete this->right;
				this->right = NULL;

				this->type = VAL;
				this->val = true;
			}
			else if (this->left->type == VAL && this->left->val == false)
			{
				// this needs to become right	
				//delete this->left;
				node * temp = new node(this->right);

				delete this; // does this work??
				return temp;
			}
			else if (this->right->type == VAL && this->right->val == false)
			{
				// this needs to become left
				//delete this->right;
				node * temp = new node(this->left);

				delete this; // does this work??
				return temp;
			}
		}
		else if (this->type == XOR)
		{
			if (this->left->type == VAL && this->left->val == false)
			{
				// this needs to become right	
				//delete this->left;
				node * temp = new node(this->right);

				delete this; // does this work??
				return temp;
			}
			else if (this->right->type == VAL && this->right->val == false)
			{
				// this needs to become left
				//delete this->right;
				node * temp = new node(this->left);

				delete this; // does this work??
				return temp;
			}
			else if (this->left->type == VAL && this->left->val == true && this->right->type == VAL && this->right->val == true)
			{
				//delete this->left;
				//delete this->right;
				delete this; // does this work??
				return new node(false);
			}
			else if (this->left->type == VAL && this->left->val == false && this->right->type == VAL && this->right->val == false)
			{
				//delete this->left;
				//delete this->right;
				delete this; // does this work??
				return new node(false);
			}
		}

		return this;
	}
}