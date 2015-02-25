#include "tree.h"
#include "rng.h"

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

node::node(node * original)
{
	this->node_type = original->node_type;
	this->op_type = original->op_type;
	this->value = original->value;
	this->byte_pos = original->byte_pos;
	this->bit_pos = original->bit_pos;

	if (original->node_type == NODE_OP)
	{
		this->left = new node(original->left);
		this->right = new node(original->right);
	}
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

node::node(bool value)
{
	this->node_type = NODE_VAL;
	this->value = value;
}

node::node(NODE_TYPE node_type, int byte_pos, uint8 bit_pos)
{
	this->node_type = node_type;
	this->byte_pos = byte_pos;
	this->bit_pos = bit_pos;
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
			delete this->left;
			delete this->right;
			this->node_type = NODE_VAL;
		}
		else if (this->op_type == OP_OR)
		{
			this->value = this->left->value || this->right->value;
			delete this->left;
			delete this->right;
			this->node_type = NODE_VAL;
		}
		else if (this->op_type == OP_XOR)
		{
			this->value = (((this->left->value ? 1 : 0) ^ (this->right->value ? 1 : 0)) == 1 ? true : false);
			delete this->left;
			delete this->right;
			this->node_type = NODE_VAL;
		}
		else if (this->op_type == OP_NOT)
		{
			this->value = !this->left->value;
			delete this->left;
			this->node_type = NODE_VAL;
		}
	}
}