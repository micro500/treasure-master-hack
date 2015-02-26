#include "tree.h"
#include "rng.h"

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

node::node()
{
	this->node_type = NODE_UNKNOWN;
	this->op_type = OP_OR;

	this->byte_pos = 0;
	this->bit_pos = 0;
	this->value = false;

	this->left = NULL;
	this->right = NULL;
}

node * new_node()
{
	node * result = get_new_node();
	result->node_type = NODE_UNKNOWN;
	result->op_type = OP_OR;

	result->byte_pos = 0;
	result->bit_pos = 0;
	result->value = false;

	result->left = NULL;
	result->right = NULL;

	return result;
}


node::~node()
{
	if (this->node_type == NODE_OP)
	{
		delete this->left;
		if (this->op_type != OP_NOT)
		{
			delete this->right;
		}
	}
}

/*
node::node(node * original)
{
	this->node_type = original->node_type;
	this->op_type = original->op_type;
	this->value = original->value;
	this->byte_pos = original->byte_pos;
	this->bit_pos = original->bit_pos;

	if (original->node_type == NODE_OP)
	{
		this->left = new_node(original->left);
		this->right = new_node(original->right);
	}
}*/

node * new_node(node * original)
{
	node * result = get_new_node();
	result->node_type = original->node_type;
	result->op_type = original->op_type;
	result->value = original->value;
	result->byte_pos = original->byte_pos;
	result->bit_pos = original->bit_pos;

	if (original->node_type == NODE_OP)
	{
		result->left = new_node(original->left);
		result->right = new_node(original->right);
	}
	else
	{
		result->left = NULL;
		result->right = NULL;
	}

	return result;
}


std::string node::to_string()
{
	std::string return_val("");
	if (this->node_type == NODE_UNKNOWN)
	{
		return_val += "UNKNOWN";
	}
	else if (this->node_type == NODE_WC || this->node_type == NODE_RNG)
	{
		if (this->node_type == NODE_WC)
		{
			return_val += "{WC,";
		}
		else
		{
			return_val += "{RNG,";
		}
		return_val += std::to_string(byte_pos);
		return_val += ",";
		return_val += std::to_string(bit_pos);
		return_val += "}";
	}
	else if (this->node_type == NODE_VAL)
	{
		return_val += (this->value ? "1" : "0");
	}
	else
	{
		std::string left_val = this->left->to_string();
		std::string right_val; 
		if (this->op_type != OP_NOT)
			right_val = this->right->to_string();

		std::string joiner;
		switch (this->op_type)
		{
		case OP_OR:
			joiner = "|";
			return_val += "(" + left_val + " " + joiner + " " + right_val + ")";
			break;

		case OP_AND:
			joiner = "&";
			return_val += "(" + left_val + " " + joiner + " " + right_val + ")";
			break;

		case OP_XOR:
			joiner = "^";
			return_val += "(" + left_val + " " + joiner + " " + right_val + ")";
			break;

		case OP_NOT:
			return_val += "!" + left_val;
			break;
		}
	}

	return return_val;
}

/*
node::node(NODE_TYPE node_type, OP_TYPE op_type, node * left, node * right)
{
	if (op_type != OP_NOT && right->node_type == NODE_UNKNOWN)
	{
		this->node_type = NODE_UNKNOWN;
		delete left;
		delete right;
	}
	else if (left->node_type == NODE_UNKNOWN)
	{
		this->node_type = NODE_UNKNOWN;
		delete left;
	}
	else
	{
		this->node_type = node_type;
		this->op_type = op_type;
		this->left = left;
		this->right = right;
	}
}
*/

node * new_node(NODE_TYPE node_type, OP_TYPE op_type, node * left, node * right)
{
	node * result = get_new_node();
	if (op_type != OP_NOT && right->node_type == NODE_UNKNOWN)
	{
		result->node_type = NODE_UNKNOWN;
		free_tree(left);
		free_tree(right);
	}
	else if (left->node_type == NODE_UNKNOWN)
	{
		result->node_type = NODE_UNKNOWN;
		free_tree(left);
	}
	else
	{
		result->node_type = node_type;
		result->op_type = op_type;
		result->left = left;
		result->right = right;
	}

	return result;
}

/*
node::node(bool value)
{
	this->node_type = NODE_VAL;
	this->value = value;
}
*/

node * new_node(bool value)
{
	node * result = get_new_node();
	result->node_type = NODE_VAL;
	result->value = value;
	result->left = NULL;
	result->right = NULL;

	return result;
}

/*
node::node(NODE_TYPE node_type, int byte_pos, uint8 bit_pos)
{
	this->node_type = node_type;
	this->byte_pos = byte_pos;
	this->bit_pos = bit_pos;
}
*/

node * new_node(NODE_TYPE node_type, int byte_pos, uint8 bit_pos)
{
	node * result = get_new_node();
	result->node_type = node_type;
	result->byte_pos = byte_pos;
	result->bit_pos = bit_pos;

	result->left = NULL;
	result->right = NULL;

	return result;
}

void node::eval(uint16 rng_seed, int last_rng_pos)
{
	if (this->node_type == NODE_VAL || this->node_type == NODE_UNKNOWN)
	{
		// Do nothing
	}
	else if (this->node_type == NODE_RNG)
	{
		this->value = get_bit_value(rng_seed, -(last_rng_pos - this->byte_pos), this->bit_pos);
		this->node_type = NODE_VAL;
	}
	else if (this->node_type == NODE_WC)
	{
		// TODO?
	}
	else
	{
		this->left->eval(rng_seed, last_rng_pos);
		if (this->op_type != OP_NOT)
		{
			this->right->eval(rng_seed, last_rng_pos);
			if (this->right->node_type != NODE_VAL)
			{
				return;
			}
		}

		if (this->left->node_type != NODE_VAL)
		{
			return;
		}

		if (this->op_type == OP_AND)
		{
			this->value = this->left->value && this->right->value;
			free_tree(this->left);
			free_tree(this->right);
			this->left = NULL;
			this->right = NULL;
			this->node_type = NODE_VAL;
		}
		else if (this->op_type == OP_OR)
		{
			this->value = this->left->value || this->right->value;
			free_tree(this->left);
			free_tree(this->right);
			this->left = NULL;
			this->right = NULL;
			this->node_type = NODE_VAL;
		}
		else if (this->op_type == OP_XOR)
		{
			this->value = (((this->left->value ? 1 : 0) ^ (this->right->value ? 1 : 0)) == 1 ? true : false);
			free_tree(this->left);
			free_tree(this->right);
			this->left = NULL;
			this->right = NULL;
			this->node_type = NODE_VAL;
		}
		else if (this->op_type == OP_NOT)
		{
			this->value = !this->left->value;
			free_tree(this->left);
			this->left = NULL;
			this->right = NULL;
			this->node_type = NODE_VAL;
		}
	}
}