#include "tree.h"
#include <sstream>
#include <iostream>

node* freenode;
std::vector<node*>* primitives;


node_stack::node_stack(class node* node_in, int state) : node(node_in), state(state) {}

/*
node
type
	xor?
		left/right of other types
		if both xor, can be combined

	and/or
		left/right

xor terms vector? or set, with removal
	0 = empty
	64 bit entries, 1 bit per index

xor convert
	or/and algorithm

	anding algorithm on

simplify
	convert all to xor
	combine xor terms


*/

void init_primitives(uint32 count)
{
	primitives = new std::vector<node*>();
	for (int i = 0; i < count; i++)
	{
		primitives->push_back(new_node_prim(i));
	}
}

void generate_new_nodes()
{
	freenode = new node[CHUNK_SIZE];
	for (int i = 0; i < CHUNK_SIZE; i++)
	{
		freenode[i].right = &freenode[i + 1];
	}
	freenode[CHUNK_SIZE - 1].right = NULL;
}

node* get_new_node()
{
	if (freenode == NULL)
	{
		generate_new_nodes();
	}
	node* temp = freenode;
	freenode = freenode->right;
	temp->left = NULL;
	temp->right = NULL;
	temp->node_id = 0;

	return temp;
}

void free_node(node* to_free)
{
	to_free->left = NULL;
	to_free->right = freenode;
	freenode = to_free;
}

void free_tree(node* to_free)
{
	if (to_free->left)
		free_tree(to_free->left);
	if (to_free->right)
		free_tree(to_free->right);

	free_node(to_free);
}

/*
// Recursive copy
node* new_node(node* original)
{
	node* result = get_new_node();
	result->type = original->type;

	if (original->left)
		result->left = new_node(original->left);
	if (original->right)
		result->right = new_node(original->right);

	delete result->terms;
	result->terms = new std::unordered_set<uint64>(*original->terms);

	return result;
}
*/

node* new_node_prim(uint64 val)
{
	node* result = get_new_node();
	result->type = PRIM;
	result->node_id = val;

	return result;
}

node* get_node_prim(uint64 val)
{
	return (*primitives)[val];
}

node* new_node_bit(uint8 val)
{
	node* result = get_new_node();
	result->type = VAL;

	result->val = (val & 1);

	return result;
}

node* new_node(NODE_TYPE type, node* left, node* right)
{
	node* result = get_new_node();
	result->type = type;
	result->left = left;
	result->right = right;

	return result;
}

node::node()
{
	this->type = XOR;
	this->left = NULL;
	this->right = NULL;
	this->node_id = 0;
	this->val = 0;
}


node::~node()
{
	delete this->left;
	delete this->right;
	//delete this;
}

std::string node::get_string()
{
	return std::string();
}

void node::print_tree(int indent)
{
	//std::cout << "NODE: ";
	if (this->type == XOR)
	{
		std::cout << "XOR";
	}
	else if (this->type == AND)
	{
		std::cout << "AND";
	}
	else if (this->type == OR)
	{
		std::cout << "OR";
	}
	else if (this->type == VAL)
	{
		std::cout << "VAL - " << this->val;
	}
	else if (this->type == NOT)
	{
		std::cout << "NOT";
	}
	else if (this->type == PRIM)
	{
		std::cout << "PRIM - " << this->node_id;
	}
	else
	{
		std::cout << "???";
	}

	std::cout << "\n";

	if (this->type == VAL || this->type == PRIM)
	{
		return;
	}

	for (int i = 0; i < indent; i++)
	{
		std::cout << " ";
	}
	std::cout << "left: ";

	if (this->left != NULL)
	{
		this->left->print_tree(indent + 2);
	}
	else
	{
		std::cout << "\n";
	}

	for (int i = 0; i < indent; i++)
	{
		std::cout << " ";
	}

	std::cout << "right: ";

	if (this->right != NULL)
	{
		this->right->print_tree(indent + 2);
	}
	else
	{
		std::cout << "\n";
	}

	this->get_string();
	//std::cout << "up\n";
}



void node::simplify_tree()
{
	this->set_node_id_tree(INT_MAX);
	this->set_node_id_tree(0);

	uint64 node_counter = primitives->size();

	std::vector<node_stack> stack;

	node_stack cur_state = node_stack(this, 0);

	while (true)
	{
		if (cur_state.node->node_id != 0)
		{
			cur_state.state = 3;
		}

		if ((cur_state.node->type == VAL || cur_state.node->type == PRIM) && cur_state.state < 2)
		{
			cur_state.state = 2;
		}

		if (cur_state.state == 0)
		{
			if (cur_state.node->left != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->left;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;
				
			}
			continue;
		}

		if (cur_state.state == 1)
		{
			if (cur_state.node->right != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->right;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;
			}
			continue;
		}

		if (cur_state.state == 2)
		{
			if (cur_state.node->type != PRIM)
			{
				cur_state.node->node_id = node_counter;
				node_counter++;
			}
			cur_state.node->simplify_node();
			cur_state.state++;
			continue;
		}

		if (cur_state.state == 3)
		{
			if (stack.empty())
			{
				break;
			}
			cur_state = stack.back();
			stack.pop_back();
			cur_state.state++;

			if (cur_state.node->left != NULL && cur_state.node->left->type == PASS)
			{
				cur_state.node->left = cur_state.node->left->left;
			}

			if (cur_state.node->right != NULL && cur_state.node->right->type == PASS)
			{
				cur_state.node->right = cur_state.node->right->left;
			}
		}
	}

	return;

	//if (this->node_id != 0)
	//{
	//	return;
	//}

	//this->node_id = node_id;
	//node_id++;

	//if (this->left != NULL)
	//{
	//	this->left->simplify(node_id);
	//	if (this->left->type == PASS)
	//	{
	//		this->left = this->left->left;
	//	}
	//}

	//if (this->right != NULL)
	//{
	//	this->right->simplify(node_id);
	//	if (this->right->type == PASS)
	//	{
	//		this->right = this->right->left;
	//	}
	//}

	//this->simplify_node();
}

void node::simplify_node()
{
	if (this->type == PASS || this->type == VAL || this->type == PRIM)
	{
		return;
	}
	else if (this->type == OR)
	{
		if (this->left == NULL && this->right == NULL)
		{
			// No possibilities
			this->type = VAL;
			this->val = 0;
			return;
		}
		else if (this->left != NULL and this->right == NULL)
		{
			this->type = PASS;
		}
		else if (this->left == NULL and this->right != NULL)
		{
			this->type = PASS;
			this->left = this->right;
			this->right = NULL;
		}
		else
		{
			if (this->left->type == VAL && this->right->type == VAL)
			{
				this->type = VAL;
				this->val = this->left->val | this->right->val;
				this->left = NULL;
				this->right = NULL;
			}
			else if (this->left->type == VAL && this->left->val == 0)
			{
				this->type = PASS;
				this->left = this->right;
				this->right = NULL;
			}
			else if (this->right->type == VAL && this->right->val == 0)
			{
				this->type = PASS;
				this->right = NULL;
			}
			else if ((this->left->type == VAL && this->left->val == 1) || (this->right->type == VAL && this->right->val == 1))
			{
				this->type = VAL;
				this->val = 1;
				this->left = NULL;
				this->right = NULL;
			}
		}

		return;
	}
	else if (this->type == AND)
	{
		if (this->left == NULL || this->right == NULL)
		{
			// No possibilities
			this->type = VAL;
			this->val = 0;
			this->left = NULL;
			this->right = NULL;
			return;
		}

		if (this->left->type == VAL && this->right->type == VAL)
		{
			this->type = VAL;
			this->val = this->left->val & this->right->val;
			this->left = NULL;
			this->right = NULL;
		}
		else if (this->left->type == VAL && this->left->val == 1 && this->right->type != VAL)
		{
			this->type = PASS;
			this->left = this->right;
			this->right = NULL;
		}
		else if (this->right->type == VAL && this->right->val == 1 && this->left->type != VAL)
		{
			this->type = PASS;
			this->right = NULL;
		}
		else if (this->left->type == VAL && this->left->val == 0 || this->right->type == VAL && this->right->val == 0)
		{
			this->type = VAL;
			this->val = 0;
			this->left = NULL;
			this->right = NULL;
		}
		else if (this->left->type == VAL && this->left->val == 1 && this->right->type == VAL && this->right->val == 1)
		{
			this->type = VAL;
			this->val = 1;
			this->left = NULL;
			this->right = NULL;
		}

		return;
	}
	else if (this->type == XOR)
	{
		if (this->left == NULL && this->right == NULL)
		{
			// No possibilities
			this->type = VAL;
			this->val = 0;
			return;
		}
		else if (this->left != NULL and this->right == NULL)
		{
			this->type = PASS;
			this->right = NULL;
		}
		else if (this->left == NULL and this->right != NULL)
		{
			this->type = PASS;
			this->left = this->right;
			this->right = NULL;
		}
		else
		{
			if (this->left->type == VAL && this->right->type == VAL)
			{
				this->type = VAL;
				this->val = this->left->val ^ this->right->val;
				this->left = NULL;
				this->right = NULL;
			}
			else if (this->left->type == VAL && this->left->val == 1)
			{
				this->type = NOT;
				this->left = this->right;
				this->right = NULL;
				this->simplify_node();
			}
			else if (this->right->type == VAL && this->right->val == 1)
			{
				this->type = NOT;
				this->right = NULL;
				this->simplify_node();
			}
			else if (this->left->type == VAL && this->left->val == 0)
			{
				this->type = PASS;
				this->left = this->right;
				this->right = NULL;
			}
			else if (this->right->type == VAL && this->right->val == 0)
			{
				this->type = PASS;
				this->right = NULL;
			}
		}

		return;
	}
	else if (this->type == NOT)
	{
		if (this->left->type == NOT)
		{
			this->type = PASS;
			this->left = this->left->left;
			this->right = NULL;
		}
		else if (this->left->type == VAL)
		{
			this->type = VAL;
			this->val = 1 - this->left->val;
			this->left = NULL;
		}
		else
		{

		}
	}

	return;
}

uint64 node::enumerate(uint64 node_id)
{
	if (this->type != PRIM && this->node_id == 0)
	{
		this->node_id = node_id;
		node_id++;
		if (this->left != NULL)
		{
			node_id = this->left->enumerate(node_id);
		}
		if (this->right != NULL)
		{
			node_id = this->right->enumerate(node_id);
		}
	}

	return node_id;
}

void node::clear_node_ids()
{
	if (this->type != PRIM)
	{
		this->node_id = 0;
		if (this->left != NULL)
		{
			this->left->clear_node_ids();
		}
		if (this->right != NULL)
		{
			this->right->clear_node_ids();
		}
	}
}

void node::set_node_ids(uint64 val)
{
	if (this->type != PRIM)
	{
		if (this->node_id != val)
		{
			this->node_id = val;
			if (this->left != NULL)
			{
				this->left->set_node_ids(val);
			}
			if (this->right != NULL)
			{
				this->right->set_node_ids(val);
			}
		}
	}
}

void node::set_node_id_tree(uint64 val)
{
	std::vector<node_stack> stack;

	node_stack cur_state = node_stack(this, 0);

	while (true)
	{
		if (cur_state.node->node_id == val && cur_state.node->ref_count == val)
		{
			cur_state.state = 3;
		}

		if ((cur_state.node->type == VAL || cur_state.node->type == PRIM) && cur_state.state < 2)
		{
			cur_state.state = 2;
		}

		if (cur_state.state == 0)
		{
			if (cur_state.node->left != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->left;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;

			}
			continue;
		}

		if (cur_state.state == 1)
		{
			if (cur_state.node->right != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->right;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;
			}
			continue;
		}

		if (cur_state.state == 2)
		{
			if (cur_state.node->type != PRIM)
			{
				cur_state.node->node_id = val;
			}
			cur_state.node->ref_count = val;
			//cur_state.node->simplify_node();
			cur_state.state++;
			continue;
		}

		if (cur_state.state == 3)
		{
			if (stack.empty())
			{
				break;
			}
			cur_state = stack.back();
			stack.pop_back();
			cur_state.state++;
		}
	}

	return;

}

uint64 node::get_tseitin_terms_tree()
{
	this->set_node_id_tree(INT_MAX);
	this->set_node_id_tree(0);

	uint64 node_counter = primitives->size();

	std::vector<node_stack> stack;

	node_stack cur_state = node_stack(this, 0);

	while (true)
	{
		if (cur_state.node->node_id != 0)
		{
			cur_state.state = 3;
		}

		if ((cur_state.node->type == VAL || cur_state.node->type == PRIM) && cur_state.state < 2)
		{
			cur_state.state = 2;
		}

		if (cur_state.state == 0)
		{
			if (cur_state.node->left != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->left;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;

			}
			continue;
		}

		if (cur_state.state == 1)
		{
			if (cur_state.node->right != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->right;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;
			}
			continue;
		}

		if (cur_state.state == 2)
		{
			if (cur_state.node->type != PRIM)
			{
				cur_state.node->node_id = node_counter;
				node_counter++;
			}
			cur_state.node->get_tseitin_terms_node();
			//cur_state.node->get_node_csv();
			cur_state.state++;
			continue;
		}

		if (cur_state.state == 3)
		{
			if (stack.empty())
			{
				break;
			}
			cur_state = stack.back();
			stack.pop_back();
			cur_state.state++;
		}
	}

	return node_counter;


	//if (this->type != PRIM && this->node_id == 0)
	//{
	//	this->node_id = node_id;
	//	node_id++;
	//	if (this->left != NULL)
	//	{
	//		node_id = this->left->get_tseitin_terms(node_id);
	//	}
	//	if (this->right != NULL)
	//	{
	//		node_id = this->right->get_tseitin_terms(node_id);
	//	}

	//}

	//return node_id;
}

void node::get_node_csv()
{
	if (this->type == AND)
	{
		uint64 a = this->left->node_id + 1;
		uint64 b = this->right->node_id + 1;
		uint64 c = this->node_id + 1;

		std::cout << "a," << c << "," << a << "," << b << "\n";
	}
	else if (this->type == OR)
	{
		uint64 a = this->left->node_id + 1;
		uint64 b = this->right->node_id + 1;
		uint64 c = this->node_id + 1;

		std::cout << "o," << c << "," << a << "," << b << "\n";
	}
	else if (this->type == NOT)
	{
		uint64 a = this->left->node_id + 1;
		uint64 c = this->node_id + 1;

		std::cout << "n," << c << "," << a << "\n";
	}
	else if (this->type == XOR)
	{
		uint64 a = this->left->node_id + 1;
		uint64 b = this->right->node_id + 1;
		uint64 c = this->node_id + 1;

		std::cout << "x," << c << "," << a << "," << b << "\n";
	}
	else if (this->type == PRIM)
	{
	}
	else if (this->type == VAL)
	{
		std::cout << "??? VAL " << ((int)this->val) << "\n";
	}
	else
	{
		std::cout << "???" << this->type << "\n";
	}
}

void node::get_tseitin_terms_node()
{
	if (this->type == AND)
	{
		uint64 a = this->left->node_id + 1;
		uint64 b = this->right->node_id + 1;
		uint64 c = this->node_id + 1;

		std::cout << "-" << a << " -" << b << " " << c << " 0\n";
		std::cout << a << " -" << c << " 0\n";
		std::cout << b << " -" << c << " 0\n";
	}
	else if (this->type == OR)
	{
		uint64 a = this->left->node_id + 1;
		uint64 b = this->right->node_id + 1;
		uint64 c = this->node_id + 1;

		std::cout << a << " " << b << " -" << c << " 0\n";
		std::cout << "-" << a << " " << c << " 0\n";
		std::cout << "-" << b << " " << c << " 0\n";
	}
	else if (this->type == NOT)
	{
		uint64 a = this->left->node_id + 1;
		uint64 c = this->node_id + 1;

		std::cout << "-" << a << " -" << c << " 0\n";
		std::cout << a << " " << c << " 0\n";
	}
	else if (this->type == XOR)
	{
		uint64 a = this->left->node_id + 1;
		uint64 b = this->right->node_id + 1;
		uint64 c = this->node_id + 1;

		// TODO: Investigate further
		//std::cout << "x" << a << " " << b << " -" << c <<  " 0\n";
		
		
		std::cout << "-" << a << " -" << b << " -" << c << " 0\n";
		std::cout << a << " " << b << " -" << c << " 0\n";
		std::cout << a << " -" << b << " " << c << " 0\n";
		std::cout << "-" << a << " " << b << " " << c << " 0\n";
	}
	else if (this->type == PRIM)
	{
	}
	else if (this->type == VAL)
	{
		std::cout << "??? VAL " << ((int)this->val) << "\n";
	}
	else
	{
		std::cout << "???" << this->type << "\n";
	}
}

std::vector<node*>* xor_bits(std::vector<node*>::iterator left, std::vector<node*>::iterator right, uint8 bit_count)
{
	std::vector<node*>* result = new std::vector<node*>();

	for (int i = 0; i < bit_count; i++)
	{
		node* left_bit = *(left + i);
		node* right_bit = *(right + i);

		result->push_back(new_node(XOR, left_bit, right_bit));
	}

	return result;
}


std::vector<node*>* add_bits(std::vector<node*>::iterator left, std::vector<node*>::iterator right, node* carry_in, uint8 bit_count)
{
	std::vector<node*>* result = new std::vector<node*>();

	node* carry = carry_in;

	for (int i = 0; i < bit_count; i++)
	{
		node* left_bit = *(left + i);
		node* right_bit = *(right + i);

		node* g = new_node(AND, left_bit, right_bit);
		node* p = new_node(OR, left_bit, right_bit);
		node* out = new_node(XOR, new_node(XOR, left_bit, right_bit), carry);

		carry = new_node(OR, g, new_node(AND, p, carry));

		result->push_back(out);
	}

	result->push_back(carry);

	return result;
}

std::vector<node*>* get_static_byte(uint8 val)
{
	std::vector<node*>* val_bits = new std::vector<node*>();
	for (int i = 0; i < 8; i++)
	{
		val_bits->push_back(new_node_bit((val >> i) & 1));
	}

	return val_bits;
}

std::vector<node*>* get_static_16bit(uint16 val)
{
	std::vector<node*>* val_bits = new std::vector<node*>();
	for (int i = 0; i < 16; i++)
	{
		val_bits->push_back(new_node_bit((val >> i) & 1));
	}

	return val_bits;
}

std::vector<node*>* get_num_flags(std::vector<node*>::iterator val_bits)
{
	node* bit0 = *val_bits;
	node* bit1 = *(val_bits + 1);
	node* bit2 = *(val_bits + 2);

	node* inv_bit0 = new_node(NOT, bit0, NULL);
	node* inv_bit1 = new_node(NOT, bit1, NULL);
	node* inv_bit2 = new_node(NOT, bit2, NULL);

	std::vector<node*>* result = new std::vector<node*>();
	result->push_back(new_node(AND, new_node(AND, inv_bit2, inv_bit1), inv_bit0));
	result->push_back(new_node(AND, new_node(AND, inv_bit2, inv_bit1), bit0));
	result->push_back(new_node(AND, new_node(AND, inv_bit2, bit1), inv_bit0));
	result->push_back(new_node(AND, new_node(AND, inv_bit2, bit1), bit0));
	result->push_back(new_node(AND, new_node(AND, bit2, inv_bit1), inv_bit0));
	result->push_back(new_node(AND, new_node(AND, bit2, inv_bit1), bit0));
	result->push_back(new_node(AND, new_node(AND, bit2, bit1), inv_bit0));
	result->push_back(new_node(AND, new_node(AND, bit2, bit1), bit0));

	return result;
}

std::vector<node*>* and_set(std::vector<node*>::iterator val_bits, node* and_node, uint16 bit_count)
{
	std::vector<node*>* result = new std::vector<node*>();

	for (int i = 0; i < bit_count; i++)
	{
		result->push_back(new_node(AND, *(val_bits + i), and_node));
	}

	return result;
}

std::vector<node*>* or_condense(std::vector<std::vector<node*>*>* node_sets)
{
	std::vector<node*>* result = (*node_sets)[0];
	uint16 bit_count = result->size();

	for (auto it = node_sets->begin() + 1; it != node_sets->end(); it++)
	{
		std::vector<node*>* temp = new std::vector<node*>();
		for (int i = 0; i < bit_count; i++)
		{
			temp->push_back(new_node(OR, (*(*it))[i], (*result)[i]));
		}

		result = temp;
	}

	return result;
}

node* and_condense(std::vector<node*>* nodes)
{
	node* result = (*nodes)[0];

	for (auto it = nodes->begin() + 1; it != nodes->end(); it++)
	{
		result = new_node(AND, result, (*it));
	}

	return result;
}

void print_vals(std::vector<node*>* nodes)
{
	for (int i = (*nodes).size() - 1; i >= 0; i--)
	{
		node* x = (*nodes)[i];
		x->simplify_tree();
		if (x->type != VAL)
		{
			std::cout << i << " ???\n";
		}
		else
		{
			std::cout << ((int)x->val) << "";
		}
	}
}

std::vector<node*>* add_byte_static(std::vector<node*>::iterator left, uint8 val, node* carry_in)
{
	std::vector<node*>* val_bits = get_static_byte(val);

	std::vector<node*>* result = add_bits(left, val_bits->begin(), carry_in, 8);

	return result;
}

std::vector<node*>* add_16bit_static(std::vector<node*>::iterator left, uint16 val, node* carry_in)
{
	std::vector<node*>* val_bits = get_static_16bit(val);

	std::vector<node*>* result = add_bits(left, val_bits->begin(), carry_in, 16);

	return result;
}

std::vector<node*>* rng_calc_old(std::vector<node*>::iterator rng1, std::vector<node*>::iterator rng2)
{
	std::vector<node*>* rngB = add_bits(rng1, rng2, new_node_bit(0), 8);

	std::vector<node*>* rngA = add_byte_static(rng1, 0x89, new_node_bit(0));

	node* carry = (*rngA)[8];

	rngB = add_byte_static(rngB->begin(), 0x2A, carry);

	carry = (*rngB)[8];

	rngA = add_byte_static(rngA->begin(), 0x21, carry);

	carry = (*rngA)[8];

	rngB = add_byte_static(rngB->begin(), 0x43, carry);

	std::vector<node*>* val_result = xor_bits(rngA->begin(), rngB->begin(), 8);

	std::vector<node*>* result = new std::vector<node*>();
	result->insert(result->end(), rngA->begin(), rngA->begin() + 8);
	result->insert(result->end(), rngB->begin(), rngB->begin() + 8);
	result->insert(result->end(), val_result->begin(), val_result->begin() + 8);

	return result;
}

std::vector<node*>* rng_calc(std::vector<node*>::iterator rng1, std::vector<node*>::iterator rng2)
{
	std::vector<node*>* rngB = add_bits(rng1, rng2, new_node_bit(0), 8);

	std::vector<node*>* cur_rng = new std::vector<node*>();

	cur_rng->insert(cur_rng->end(), rng1, rng1 + 8);
	cur_rng->insert(cur_rng->end(), rngB->begin(), rngB->begin() + 8);

	node* greater_carry = greater_than_static(cur_rng, 0xD576, 16);

	cur_rng = add_16bit_static(cur_rng->begin(), 0x6daa, greater_carry);

	std::vector<node*>* val_result = xor_bits(cur_rng->begin(), cur_rng->begin() + 8, 8);

	std::vector<node*>* result = new std::vector<node*>();

	result->insert(result->end(), cur_rng->begin(), cur_rng->begin() + 16);

	result->insert(result->end(), val_result->begin(), val_result->begin() + 8);

	return result;
}

node* greater_than_static(std::vector<node*>* val_bitfield, uint64 compare_val, uint8 compare_bits)
{
	node* cur_node = new_node_bit(0);
	bool first_bit_found = false;
	for (int i = 0; i < compare_bits; i++)
	{
		uint8 cur_bit = (compare_val >> i) & 0x01;
		if (!first_bit_found && cur_bit == 1)
		{
			continue;
		}

		first_bit_found = true;

		if (cur_bit == 0)
		{
			cur_node = new_node(OR, (*val_bitfield)[i], cur_node);
		}
		else
		{
			cur_node = new_node(AND, (*val_bitfield)[i], cur_node);
		}
	}

	return cur_node;
}



std::vector<uint64>* node::get_node_type_counts_tree()
{
	this->set_node_id_tree(INT_MAX);
	this->set_node_id_tree(0);

	std::vector<uint64>* node_counts = new std::vector<uint64>(8, 0);

	uint64 node_counter = primitives->size();

	std::vector<node_stack> stack;

	node_stack cur_state = node_stack(this, 0);

	while (true)
	{
		if (cur_state.node->node_id != 0)
		{
			cur_state.state = 3;
		}

		if ((cur_state.node->type == VAL || cur_state.node->type == PRIM) && cur_state.state < 2)
		{
			cur_state.state = 2;
		}

		if (cur_state.state == 0)
		{
			if (cur_state.node->left != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->left;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;

			}
			continue;
		}

		if (cur_state.state == 1)
		{
			if (cur_state.node->right != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->right;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;
			}
			continue;
		}

		if (cur_state.state == 2)
		{
			if (cur_state.node->type != PRIM)
			{
				cur_state.node->node_id = node_counter;
				node_counter++;

				(*node_counts)[cur_state.node->type]++;
			}
			
			cur_state.state++;
			continue;
		}

		if (cur_state.state == 3)
		{
			if (stack.empty())
			{
				break;
			}
			cur_state = stack.back();
			stack.pop_back();
			cur_state.state++;
		}
	}

	std::cout << node_counter << "\n";

	return node_counts;
}

uint64 node::count_refs_tree()
{
	this->set_node_id_tree(INT_MAX);
	this->set_node_id_tree(0);

	uint64 node_counter = primitives->size();
	uint64 max_ref_count = 0;

	std::vector<node_stack> stack;

	node_stack cur_state = node_stack(this, 0);

	while (true)
	{
		if (cur_state.node->node_id != 0)
		{
			cur_state.state = 3;
		}

		if ((cur_state.node->type == VAL || cur_state.node->type == PRIM) && cur_state.state < 2)
		{
			cur_state.state = 2;
		}

		if (cur_state.state == 0)
		{
			if (cur_state.node->left != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->left;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;

			}
			continue;
		}

		if (cur_state.state == 1)
		{
			if (cur_state.node->right != NULL)
			{
				stack.push_back(cur_state);
				cur_state.node = cur_state.node->right;
				cur_state.state = 0;
			}
			else
			{
				cur_state.state++;
			}
			continue;
		}

		if (cur_state.state == 2)
		{
			if (cur_state.node->type != PRIM)
			{
				cur_state.node->node_id = node_counter;
				node_counter++;
			}
			cur_state.state++;
			continue;
		}

		if (cur_state.state == 3)
		{
			cur_state.node->ref_count++;

			if (cur_state.node->type != PRIM && cur_state.node->ref_count > max_ref_count)
			{
				max_ref_count = cur_state.node->ref_count;
			}
			if (stack.empty())
			{
				break;
			}
			cur_state = stack.back();
			stack.pop_back();
			cur_state.state++;
		}
	}

	return max_ref_count;
}

std::vector<node*>* decrypt_memory(std::vector<node*>* bitfield, uint8* encrypted_memory, uint8 memory_length)
{
	std::vector<node*>* result = new std::vector<node*>();
	for (int i = 0; i < memory_length; i++)
	{
		uint16 bitfield_offset = (127 - i) * 8;
		std::vector<node*>* temp = xor_bits(bitfield->begin() + bitfield_offset, get_static_byte(encrypted_memory[i])->begin(), 8);
		result->insert(result->end(), temp->begin(), temp->end());
	}

	return result;
}

void set_prims(uint32 prim_count, uint8 * values)
{
	for (int i = 0; i < prim_count; i++)
	{
		uint32 value_offset = i / 8;
		uint8 bit_offset = i % 8;

		uint8 bit_val = (values[value_offset] >> bit_offset) & 0x01;

		node* t = get_node_prim(i);
		t->type = VAL;
		t->val = bit_val;
		t->node_id = 0;
	}
}

std::vector<uint8>* bitfield_to_bytes(std::vector<node*>* bitfield)
{
	std::vector<uint8>* result = new std::vector<uint8>();

	uint8 cur_byte = 0;
	uint8 bit_count = 0;

	for (auto it = bitfield->begin(); it != bitfield->end(); it++)
	{
		node* x = (*it);
		x->simplify_tree();

		uint8 bit_val = 0;
		if (x->type == VAL)
		{
			bit_val = x->val & 0x01;
		}

		cur_byte = cur_byte | (bit_val << bit_count);
		bit_count++;
		if (bit_count == 8)
		{
			result->push_back(cur_byte);
			cur_byte = 0;
			bit_count = 0;
		}
	}

	if (bit_count != 0)
	{
		result->push_back(cur_byte);
	}

	return result;
}

node* check_equal(std::vector<node*>* left, std::vector<node*>* right, uint16 bit_count)
{
	std::vector<node*>* compare_bits = new std::vector<node*>();
	for (int i = 0; i < bit_count; i++)
	{
		node* left_bit = (*left)[i];
		node* right_bit = (*right)[i];

		compare_bits->push_back(new_node(NOT, new_node(XOR, left_bit, right_bit), NULL));
	}

	return and_condense(compare_bits);
}

node* checksum_bitfield(std::vector<node*>* bitfield)
{
	int byte_count = bitfield->size() / 8;

	std::vector<node*>* rolling_sum = new std::vector<node*>();
	rolling_sum->insert(rolling_sum->end(), bitfield->begin(), bitfield->begin() + 8);
	node* zero = new_node_bit(0);
	for (int i = 0; i < 8; i++)
	{
		rolling_sum->push_back(zero);
	}

	for (int i = 1; i < byte_count - 2; i++)
	{
		std::vector<node*>* temp = new std::vector<node*>();
		temp->insert(temp->end(), bitfield->begin() + i*8, bitfield->begin() + i * 8 + 8);
		for (int i = 0; i < 8; i++)
		{
			temp->push_back(zero);
		}

		rolling_sum = add_bits(rolling_sum->begin(), temp->begin(), zero, 16);
	}

	std::vector<node*>* desired_sum = new std::vector<node*>();
	desired_sum->insert(desired_sum->end(), bitfield->begin() + (byte_count*8) - 16, bitfield->end());

	return check_equal(rolling_sum, desired_sum, 16);
}

node* get_desired_node(std::vector<node*>* bitfield, uint8* values)
{
	node* result = new_node_bit(1);
	for (int i = 0; i < bitfield->size(); i++)
	{
		int bit_index = i % 8;
		uint8 bit_val = (values[i / 8] >> bit_index) & 0x01;
		node* next_node = (*bitfield)[i];

		if (bit_val == 0x00)
		{
			next_node = new_node(NOT, next_node, NULL);
		}
		result = new_node(AND, result, next_node);
	}

	return result;
}