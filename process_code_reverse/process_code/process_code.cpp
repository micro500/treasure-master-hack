#include <stdio.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <unordered_set>

#include "boinc.h"

#include "data_sizes.h"
#include "cmd_args.h"
#include "rng.h"
#include "tree.h"
#include "working_code.h"
#include "key_schedule.h"
#include "verify.h"
#include "MurmurHash3_wrapper.h"

#define BIT_POS(x,y) (x*8 + (7-y))

struct rng_info
{
	int byte_pos;
	uint8 bit_pos;

	node * bit;
};

void attack(node * bitfield[], int rng_pos, std::vector<rng_info> rng_bits, std::vector<int> path, int depth_limit);

void print_bitfield(node * bitfield[])
{
	for (int i = 0; i < 128 * 8; i++)
	{
		printf("%s\n",bitfield[i]->to_string().c_str());
	}
}

void add_byte(int bitfield_byte_pos, node * bit_field[], node * r_bit0, node * r_bit1, node * r_bit2, node * r_bit3, node * r_bit4, node * r_bit5, node * r_bit6, node * r_bit7)
{
	node * l_bit0 = bit_field[BIT_POS(bitfield_byte_pos,0)];
	node * l_bit1 = bit_field[BIT_POS(bitfield_byte_pos,1)];
	node * l_bit2 = bit_field[BIT_POS(bitfield_byte_pos,2)];
	node * l_bit3 = bit_field[BIT_POS(bitfield_byte_pos,3)];
	node * l_bit4 = bit_field[BIT_POS(bitfield_byte_pos,4)];
	node * l_bit5 = bit_field[BIT_POS(bitfield_byte_pos,5)];
	node * l_bit6 = bit_field[BIT_POS(bitfield_byte_pos,6)];
	node * l_bit7 = bit_field[BIT_POS(bitfield_byte_pos,7)];

	node * carry0;
	node * carry1;
	node * carry2;
	node * carry3;
	node * carry4;
	node * carry5;
	node * carry6;
	node * p0;
	node * p1;
	node * p2;
	node * p3;
	node * p4;
	node * p5;
	node * p6;
	node * g0;
	node * g1;
	node * g2;
	node * g3;
	node * g4;
	node * g5;
	node * g6;

	// General formulas for byte addition:
	// a,b = bits to sum
	// c = carry
	// out = a ^ b ^ c

	// Generate carry:
	// g = a & b

	// Propagate carry:
	// p = a | b

	// Carry:
	// carry = g | (p & c)


	g0 = new node(NODE_OP, OP_AND, new node(l_bit0), new node(r_bit0));
	p0 = new node(NODE_OP, OP_OR, new node(l_bit0), new node(r_bit0));
	// Since there is no carry, the (p & c) term would drop out and you are left with g
	carry0 = g0;
	// No carry for the 0th bit
	bit_field[BIT_POS(bitfield_byte_pos,0)] = new node(NODE_OP, OP_XOR, new node(l_bit0), new node(r_bit0));
	
	g1 = new node(NODE_OP,OP_AND,new node(l_bit1), new node(r_bit1));
	p1 = new node(NODE_OP, OP_OR,new node(l_bit1), new node(r_bit1));
	carry1 = new node(NODE_OP, OP_OR,g1,new node(NODE_OP, OP_AND,p1,new node(carry0)));
	bit_field[BIT_POS(bitfield_byte_pos,1)] = new node(NODE_OP, OP_XOR,new node(NODE_OP, OP_XOR,new node(l_bit1),new node(r_bit1)),new node(carry0));

	g2 = new node(NODE_OP, OP_AND,new node(l_bit2), new node(r_bit2));
	p2 = new node(NODE_OP, OP_OR,new node(l_bit2), new node(r_bit2));
	carry2 = new node(NODE_OP, OP_OR,g2,new node(NODE_OP, OP_AND,p2,new node(carry1)));
	bit_field[BIT_POS(bitfield_byte_pos,2)] = new node(NODE_OP, OP_XOR,new node(NODE_OP, OP_XOR,new node(l_bit2),new node(r_bit2)),new node(carry1));

	g3 = new node(NODE_OP, OP_AND,new node(l_bit3), new node(r_bit3));
	p3 = new node(NODE_OP, OP_OR,new node(l_bit3), new node(r_bit3));
	carry3 = new node(NODE_OP, OP_OR,g3,new node(NODE_OP, OP_AND,p3,new node(carry2)));
	bit_field[BIT_POS(bitfield_byte_pos,3)] = new node(NODE_OP, OP_XOR,new node(NODE_OP, OP_XOR,new node(l_bit3),new node(r_bit3)),new node(carry2));

	g4 = new node(NODE_OP, OP_AND, new node(l_bit4), new node(r_bit4));
	p4 = new node(NODE_OP, OP_OR, new node(l_bit4), new node(r_bit4));
	carry4 = new node(NODE_OP, OP_OR,g4,new node(NODE_OP, OP_AND,p4,new node(carry3)));
	bit_field[BIT_POS(bitfield_byte_pos,4)] = new node(NODE_OP, OP_XOR,new node(NODE_OP, OP_XOR,new node(l_bit4),new node(r_bit4)),new node(carry3));

	g5 = new node(NODE_OP, OP_AND,new node(l_bit5), new node(r_bit5));
	p5 = new node(NODE_OP, OP_OR,new node(l_bit5), new node(r_bit5));
	carry5 = new node(NODE_OP, OP_OR,g5,new node(NODE_OP, OP_AND,p5,new node(carry4)));
	bit_field[BIT_POS(bitfield_byte_pos,5)] = new node(NODE_OP, OP_XOR,new node(NODE_OP, OP_XOR,new node(l_bit5),new node(r_bit5)),new node(carry4));

	g6 = new node(NODE_OP, OP_AND,new node(l_bit6), new node(r_bit6));
	p6 = new node(NODE_OP, OP_OR,new node(l_bit6), new node(r_bit6));
	carry6 = new node(NODE_OP, OP_OR,g6,new node(NODE_OP, OP_AND,p6,new node(carry5)));
	bit_field[BIT_POS(bitfield_byte_pos,6)] = new node(NODE_OP, OP_XOR,new node(NODE_OP, OP_XOR,new node(l_bit6),new node(r_bit6)),new node(carry5));

	// Carry for bit 7 is dropped
	bit_field[BIT_POS(bitfield_byte_pos,7)] = new node(NODE_OP, OP_XOR,new node(NODE_OP, OP_XOR,new node(l_bit7),new node(r_bit7)),new node(carry6));
}

void sub_rng_byte(int bitfield_byte_pos, int rng_pos, node * bit_field[])
{
	node * r_bit0 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,0), new node(true));
	node * r_bit1 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,1), new node(true));
	node * r_bit2 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,2), new node(true));
	node * r_bit3 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,3), new node(true));
	node * r_bit4 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,4), new node(true));
	node * r_bit5 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,5), new node(true));
	node * r_bit6 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,6), new node(true));
	node * r_bit7 = new node(NODE_OP, OP_XOR, new node(NODE_RNG,rng_pos,7), new node(true));

	add_byte(bitfield_byte_pos, bit_field, r_bit0, r_bit1, r_bit2, r_bit3, r_bit4, r_bit5, r_bit6, r_bit7);

	r_bit0 = new node(true);
	r_bit1 = new node(false);
	r_bit2 = new node(false);
	r_bit3 = new node(false);
	r_bit4 = new node(false);
	r_bit5 = new node(false);
	r_bit6 = new node(false);
	r_bit7 = new node(false);

	add_byte(bitfield_byte_pos, bit_field, r_bit0, r_bit1, r_bit2, r_bit3, r_bit4, r_bit5, r_bit6, r_bit7);
}

void add_rng_byte(int bitfield_byte_pos, int rng_pos, node * bit_field[])
{
	node * r_bit0 = new node(NODE_RNG,rng_pos,0);
	node * r_bit1 = new node(NODE_RNG,rng_pos,1);
	node * r_bit2 = new node(NODE_RNG,rng_pos,2);
	node * r_bit3 = new node(NODE_RNG,rng_pos,3);
	node * r_bit4 = new node(NODE_RNG,rng_pos,4);
	node * r_bit5 = new node(NODE_RNG,rng_pos,5);
	node * r_bit6 = new node(NODE_RNG,rng_pos,6);
	node * r_bit7 = new node(NODE_RNG,rng_pos,7);

	add_byte(bitfield_byte_pos, bit_field, r_bit0, r_bit1, r_bit2, r_bit3, r_bit4, r_bit5, r_bit6, r_bit7);
}

void inverse_alg0(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	for (int byte_pos = 0; byte_pos < 128; byte_pos++)
	{
		// The lowest bit comes from the RNG
		rng_info rng_bit;
		rng_bit.byte_pos = rng_pos;
		rng_bit.bit_pos = 7;
		rng_bit.bit = bitfield[BIT_POS(byte_pos,0)];
		rng_bits.push_back(rng_bit);
		rng_pos--;

		// Shift byte right
		bitfield[BIT_POS(byte_pos,0)] = bitfield[BIT_POS(byte_pos,1)];
		bitfield[BIT_POS(byte_pos,1)] = bitfield[BIT_POS(byte_pos,2)];
		bitfield[BIT_POS(byte_pos,2)] = bitfield[BIT_POS(byte_pos,3)];
		bitfield[BIT_POS(byte_pos,3)] = bitfield[BIT_POS(byte_pos,4)];
		bitfield[BIT_POS(byte_pos,4)] = bitfield[BIT_POS(byte_pos,5)];
		bitfield[BIT_POS(byte_pos,5)] = bitfield[BIT_POS(byte_pos,6)];
		bitfield[BIT_POS(byte_pos,6)] = bitfield[BIT_POS(byte_pos,7)];
		// recovered an unknown bit
		bitfield[BIT_POS(byte_pos,7)] = new node();
	}
}

void inverse_alg1(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	for (int byte_pos = 0; byte_pos < 128; byte_pos++)
	{
		// Subtract an RNG value 
		sub_rng_byte(byte_pos,rng_pos-byte_pos,bitfield);
	}

	rng_pos -= 128;
}

void inverse_alg2(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	// TODO: 
	// Mark this is as a lost bit, not a static value
	// Shouldn't matter for now, we don't look at the bytes this should affect anyway
	node * carry = new node();

	for (int byte_pos = 0; byte_pos < 128; byte_pos+=2)
	{
		// Save bit 0 of byte 1 as the new carry
		node * new_carry = bitfield[BIT_POS((byte_pos + 1),0)];

		// Shift byte 1 right
		bitfield[BIT_POS((byte_pos + 1),0)] = bitfield[BIT_POS((byte_pos + 1),1)];
		bitfield[BIT_POS((byte_pos + 1),1)] = bitfield[BIT_POS((byte_pos + 1),2)];
		bitfield[BIT_POS((byte_pos + 1),2)] = bitfield[BIT_POS((byte_pos + 1),3)];
		bitfield[BIT_POS((byte_pos + 1),3)] = bitfield[BIT_POS((byte_pos + 1),4)];
		bitfield[BIT_POS((byte_pos + 1),4)] = bitfield[BIT_POS((byte_pos + 1),5)];
		bitfield[BIT_POS((byte_pos + 1),5)] = bitfield[BIT_POS((byte_pos + 1),6)];
		bitfield[BIT_POS((byte_pos + 1),6)] = bitfield[BIT_POS((byte_pos + 1),7)];
		// Set byte 1, bit 7 as byte 0, bit 7
		bitfield[BIT_POS((byte_pos + 1),7)] = bitfield[BIT_POS(byte_pos,7)];

		// Shift byte 0 left
		bitfield[BIT_POS(byte_pos,7)] = bitfield[BIT_POS(byte_pos,6)];
		bitfield[BIT_POS(byte_pos,6)] = bitfield[BIT_POS(byte_pos,5)];
		bitfield[BIT_POS(byte_pos,5)] = bitfield[BIT_POS(byte_pos,4)];
		bitfield[BIT_POS(byte_pos,4)] = bitfield[BIT_POS(byte_pos,3)];
		bitfield[BIT_POS(byte_pos,3)] = bitfield[BIT_POS(byte_pos,2)];
		bitfield[BIT_POS(byte_pos,2)] = bitfield[BIT_POS(byte_pos,1)];
		bitfield[BIT_POS(byte_pos,1)] = bitfield[BIT_POS(byte_pos,0)];
		// Set byte 0, bit 0 as old carry
		bitfield[BIT_POS(byte_pos,0)] = carry;

		carry = new_carry;
	}

	// Remaining carry is from the RNG
	rng_info rng_bit;
	rng_bit.byte_pos = rng_pos;
	rng_bit.bit_pos = 7;
	rng_bit.bit = carry;
	rng_bits.push_back(rng_bit);

	rng_pos--;
}

void inverse_alg3(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	for (int byte_pos = 0; byte_pos < 128; byte_pos++)
	{
		for (int bit_pos = 0; bit_pos < 8; bit_pos++)
		{
			// XOR with an RNG bit
			bitfield[BIT_POS(byte_pos,bit_pos)] = new node(NODE_OP, OP_XOR, bitfield[BIT_POS(byte_pos,bit_pos)], new node(NODE_RNG, rng_pos-byte_pos, bit_pos));
		}
	}

	rng_pos -= 128;
}

void inverse_alg4(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	for (int byte_pos = 0; byte_pos < 128; byte_pos++)
	{
		// Add an RNG value 
		add_rng_byte(byte_pos,rng_pos-byte_pos,bitfield);
		//bitfield[BIT_POS(byte_pos,bit_pos)] = new node(NODE_OP,OP_XOR,bitfield[BIT_POS(byte_pos,bit_pos)],new node(NODE_RNG,rng_pos-byte_pos,bit_pos));
	}

	rng_pos -= 128;
}

void inverse_alg5(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	// TODO: 
	// Mark this is as a lost bit, not a static value
	// Shouldn't matter for now, we don't look at the bytes this should affect anyway
	node * carry = new node();

	for (int byte_pos = 0; byte_pos < 128; byte_pos+=2)
	{
		// Save bit 7 of byte 1 as the new carry
		node * new_carry = bitfield[BIT_POS((byte_pos + 1),7)];

		// Shift byte 1 left
		bitfield[BIT_POS((byte_pos + 1),7)] = bitfield[BIT_POS((byte_pos + 1),6)];
		bitfield[BIT_POS((byte_pos + 1),6)] = bitfield[BIT_POS((byte_pos + 1),5)];
		bitfield[BIT_POS((byte_pos + 1),5)] = bitfield[BIT_POS((byte_pos + 1),4)];
		bitfield[BIT_POS((byte_pos + 1),4)] = bitfield[BIT_POS((byte_pos + 1),3)];
		bitfield[BIT_POS((byte_pos + 1),3)] = bitfield[BIT_POS((byte_pos + 1),2)];
		bitfield[BIT_POS((byte_pos + 1),2)] = bitfield[BIT_POS((byte_pos + 1),1)];
		bitfield[BIT_POS((byte_pos + 1),1)] = bitfield[BIT_POS((byte_pos + 1),0)];
		// Set byte 1, bit 7 as byte 0, bit 7
		bitfield[BIT_POS((byte_pos + 1),0)] = bitfield[BIT_POS(byte_pos,0)];	

		// Shift byte 0 right
		bitfield[BIT_POS(byte_pos,0)] = bitfield[BIT_POS(byte_pos,1)];
		bitfield[BIT_POS(byte_pos,1)] = bitfield[BIT_POS(byte_pos,2)];
		bitfield[BIT_POS(byte_pos,2)] = bitfield[BIT_POS(byte_pos,3)];
		bitfield[BIT_POS(byte_pos,3)] = bitfield[BIT_POS(byte_pos,4)];
		bitfield[BIT_POS(byte_pos,4)] = bitfield[BIT_POS(byte_pos,5)];
		bitfield[BIT_POS(byte_pos,5)] = bitfield[BIT_POS(byte_pos,6)];
		bitfield[BIT_POS(byte_pos,6)] = bitfield[BIT_POS(byte_pos,7)];
		// Set byte 0, bit 0 as old carry
		bitfield[BIT_POS(byte_pos,7)] = carry;

		carry = new_carry;
	}

	// Remaining carry is from the RNG
	rng_info rng_bit;
	rng_bit.byte_pos = rng_pos;
	rng_bit.bit_pos = 7;
	rng_bit.bit = carry;
	rng_bits.push_back(rng_bit);

	rng_pos--;
}

void inverse_alg6(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	for (int byte_pos = 127; byte_pos >= 0; byte_pos--)
	{
		// The highest bit comes from the RNG
		rng_info rng_bit;
		rng_bit.byte_pos = rng_pos;
		rng_bit.bit_pos = 7;
		rng_bit.bit = bitfield[BIT_POS(byte_pos,7)];
		rng_bits.push_back(rng_bit);
		rng_pos--;

		// Shift byte left
		bitfield[BIT_POS(byte_pos,7)] = bitfield[BIT_POS(byte_pos,6)];
		bitfield[BIT_POS(byte_pos,6)] = bitfield[BIT_POS(byte_pos,5)];
		bitfield[BIT_POS(byte_pos,5)] = bitfield[BIT_POS(byte_pos,4)];
		bitfield[BIT_POS(byte_pos,4)] = bitfield[BIT_POS(byte_pos,3)];
		bitfield[BIT_POS(byte_pos,3)] = bitfield[BIT_POS(byte_pos,2)];
		bitfield[BIT_POS(byte_pos,2)] = bitfield[BIT_POS(byte_pos,1)];
		bitfield[BIT_POS(byte_pos,1)] = bitfield[BIT_POS(byte_pos,0)];
		// recovered an unknown bit
		bitfield[BIT_POS(byte_pos,0)] = new node();
	}
}

void inverse_alg7(node* bitfield[], int &rng_pos, std::vector<rng_info> &rng_bits)
{
	for (int i = 0; i < 128 * 8; i++)
	{
		bitfield[i] = new node(NODE_OP,OP_XOR,bitfield[i],new node(true));
	}
}

int main(int argc, char **argv)
{
	int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11};

	generate_rng_table();

	node* all_bits[8 * 128];

	// from the known code processing
	//uint8 known_working_code[128] = { 0xAB, 0xD6, 0x71, 0xC2, 0xBA, 0x58, 0xDB, 0xD6, 0x3C, 0xCA, 0x02, 0xEB, 0x04, 0x77, 0xC7, 0x34, 0x77, 0xB5, 0x7D, 0x91, 0x5F, 0x0D, 0xB8, 0x3A, 0xF5, 0xB8, 0xCF, 0x9C, 0x23, 0xC3, 0x78, 0x38, 0xB7, 0x62, 0x85, 0x46, 0x62, 0x9C, 0x99, 0x35, 0x68, 0xB9, 0xC7, 0x9F, 0x0B, 0x4F, 0xE0, 0xB4, 0x1A, 0x39, 0xB1, 0xE7, 0x82, 0x43, 0x53, 0x6A, 0xEB, 0xA2, 0x48, 0x5E, 0x6B, 0xE5, 0xFC, 0x87, 0x02, 0xF8, 0xF9, 0xEB, 0x14, 0x78, 0x67, 0xEF, 0xC8, 0xBA, 0xD1, 0xAE, 0x2C, 0x30, 0x38, 0x99, 0xCD, 0x91, 0x8E, 0x1E, 0x5B, 0x40, 0x87, 0x92, 0xC1, 0x11, 0x00, 0x62, 0x25, 0x22, 0xDE, 0xB3, 0x8C, 0xA5, 0x29, 0x4F, 0x77, 0xC0, 0x60, 0x83, 0xA6, 0xA5, 0x88, 0x02, 0x54, 0x21, 0x42, 0x3F, 0x14, 0xC6, 0x88, 0xBB, 0xCB, 0xD0, 0xB6, 0x3D, 0xC2, 0xB2, 0x57, 0x4E, 0xC3, 0x72, 0x66, 0xDD };

	// end of known code processing (2-3-7-2-3-7-4-3-6)
	//uint8 known_working_code[128] = { 0x56, 0xD2, 0x3D, 0xCF, 0x42, 0xBF, 0x93, 0xD0, 0x29, 0xD1, 0xF0, 0xA3, 0x7F, 0x36, 0x99, 0xD6, 0xC2, 0x58, 0x94, 0x0F, 0xDE, 0x2C, 0x52, 0x1C, 0x72, 0x4A, 0x4C, 0x82, 0x4E, 0x96, 0x82, 0x03, 0xCF, 0xFE, 0xFA, 0x9C, 0x14, 0x8C, 0xF2, 0x54, 0x8F, 0x4F, 0x80, 0xF8, 0x81, 0x8C, 0xEF, 0x4A, 0x8F, 0x1F, 0x51, 0x86, 0xAA, 0xB0, 0xA9, 0xDA, 0x0E, 0x24, 0xD5, 0xE9, 0xC7, 0x3C, 0xF5, 0xDF, 0xF8, 0xF7, 0xA0, 0x7C, 0x1B, 0xF3, 0x3E, 0x56, 0xFE, 0x43, 0x70, 0xF6, 0x89, 0xB2, 0x18, 0x88, 0x4D, 0x30, 0xD9, 0x1F, 0xD6, 0xCA, 0xED, 0xAE, 0xBF, 0xCD, 0x25, 0x05, 0x78, 0x4E, 0x44, 0xE2, 0x97, 0xF1, 0xD9, 0xAA, 0x31, 0x11, 0x47, 0x5C, 0x77, 0x3E, 0xE3, 0xDD, 0x22, 0x84, 0x93, 0x14, 0x97, 0x10, 0xF8, 0xD7, 0x1C, 0x93, 0x44, 0x9C, 0xEF, 0xD4, 0xEB, 0x77, 0x40, 0x3C, 0x22, 0xFD };

	// 6 test
	//uint8 known_working_code[128] = { 0xFD, 0x31, 0x0C, 0xFD, 0x78, 0x62, 0xD7, 0x67, 0x37, 0xAF, 0xA1, 0x6A, 0xF6, 0xD7, 0xC6, 0x17, 0xFF, 0xE9, 0x05, 0xB6, 0xF6, 0x60, 0xDE, 0x87, 0x49, 0xC9, 0xE1, 0x67, 0xAA, 0x97, 0x50, 0x8D, 0xBF, 0x7E, 0x80, 0xDF, 0x64, 0xC1, 0xFB, 0x52, 0xCD, 0xE9, 0x46, 0xE5, 0xDF, 0x50, 0x93, 0x96, 0x00, 0xA1, 0x3D, 0xDB, 0xF8, 0x56, 0xE6, 0x05, 0x2E, 0xFD, 0x11, 0x16, 0xFB, 0x4A, 0xDD, 0x1E, 0x68, 0xB2, 0x6B, 0xA8, 0xD4, 0x66, 0xFF, 0x73, 0xA6, 0x05, 0x2E, 0xB3, 0x10, 0xF1, 0x6E, 0xEA, 0x84, 0x47, 0xF3, 0x11, 0xE4, 0xE7, 0x3B, 0xAE, 0x9B, 0xB7, 0x56, 0x2B, 0x9D, 0xAE, 0x93, 0x56, 0xE5, 0xBF, 0xA3, 0x6B, 0x6C, 0x17, 0x8C, 0x80, 0x05, 0x4E, 0x27, 0xA7, 0x95, 0xB8, 0x6A, 0xDC, 0xD2, 0xF9, 0x4C, 0x4E, 0x46, 0xF9, 0xF2, 0xF7, 0x26, 0x02, 0x55, 0xAD, 0x6E, 0x93, 0xF1, 0xC8 };

	// 3-6 test
	//uint8 known_working_code[128] = { 0x26, 0x78, 0x58, 0x18, 0xC2, 0x48, 0x63, 0x80, 0xC2, 0xE7, 0x8F, 0x2B, 0xF4, 0xC5, 0xB9, 0xDD, 0xAF, 0x52, 0x89, 0x42, 0xF7, 0x1B, 0xE0, 0xD3, 0x79, 0xA4, 0x4C, 0xF8, 0x2D, 0x67, 0x18, 0xB2, 0xE6, 0x6F, 0xD2, 0x7D, 0xC8, 0x3E, 0xE7, 0x02, 0xDE, 0x90, 0xBB, 0x4F, 0x11, 0x34, 0x50, 0xCB, 0x1D, 0xEF, 0xEB, 0x5B, 0x53, 0x7F, 0xE3, 0x2F, 0x88, 0x79, 0xBA, 0x4B, 0xFE, 0x74, 0x13, 0x6E, 0x2B, 0xCB, 0x86, 0x43, 0x0A, 0x22, 0x8E, 0xCA, 0x1D, 0x9F, 0x3B, 0xA7, 0x26, 0x0C, 0x04, 0xA4, 0x23, 0x81, 0x6B, 0x19, 0xF0, 0xCE, 0xF2, 0xCC, 0xE1, 0x08, 0x05, 0x9B, 0xFE, 0xA3, 0x60, 0x75, 0x29, 0x83, 0x5D, 0xFB, 0x1D, 0xBF, 0xD6, 0x29, 0x53, 0x6C, 0xBC, 0xA2, 0x61, 0x7F, 0x54, 0x04, 0xDD, 0x2D, 0xEB, 0x65, 0xCF, 0x7D, 0xB8, 0x51, 0x45, 0x69, 0xE8, 0xE9, 0xF0, 0xDD, 0xA6, 0x4B };

	// 4-3-6 test
	//uint8 known_working_code[128] = { 0xCD, 0x4C, 0xCE, 0xFA, 0x9F, 0x95, 0x46, 0x54, 0x34, 0xC1, 0x68, 0x90, 0xCB, 0x71, 0x2F, 0xA7, 0x6C, 0xD7, 0xAC, 0xEE, 0x99, 0x1D, 0x2A, 0xA2, 0xDF, 0x0F, 0x37, 0x41, 0x2F, 0x7E, 0xA9, 0x8C, 0x25, 0x57, 0xC8, 0xE4, 0xA2, 0xCC, 0x7C, 0x10, 0x3A, 0xC5, 0x33, 0x6B, 0x0C, 0xE9, 0x26, 0x87, 0x19, 0x8A, 0x62, 0x5C, 0xB0, 0xAF, 0x30, 0x42, 0x65, 0x16, 0x9E, 0xCF, 0x30, 0xC3, 0x4C, 0xE3, 0x43, 0xA6, 0x5C, 0x3D, 0x83, 0x54, 0xF3, 0xE9, 0xA8, 0x6D, 0xF5, 0x8B, 0xDB, 0x37, 0x7F, 0xCE, 0xF4, 0x79, 0x4B, 0x74, 0x40, 0x30, 0xCC, 0x96, 0xC2, 0x5D, 0x54, 0xF7, 0xDB, 0x8D, 0x34, 0x96, 0xA9, 0xD3, 0x10, 0x0B, 0x57, 0xF6, 0x6D, 0xBB, 0xB1, 0xAC, 0x2D, 0xA2, 0x32, 0x52, 0x74, 0x83, 0x84, 0xE3, 0x12, 0x81, 0x90, 0x4F, 0x88, 0x62, 0xC9, 0x29, 0x97, 0xB0, 0x56, 0xBF, 0xB1, 0x6C };

	// 5-7-7-1-0 test
	//uint8 known_working_code[128] = { 0x1E, 0xA6, 0xD6, 0x44, 0x9B, 0x34, 0x6D, 0xC0, 0xD5, 0x3A, 0x17, 0x85, 0xE9, 0x38, 0x81, 0x21, 0xED, 0x64, 0xDB, 0x9E, 0x4D, 0xC6, 0xE2, 0x35, 0x07, 0x8E, 0x97, 0x37, 0xF9, 0x1C, 0x34, 0x04, 0x9E, 0xA8, 0xE9, 0x1C, 0x34, 0x5A, 0xCB, 0x78, 0x68, 0xB8, 0x98, 0x69, 0x0C, 0xA1, 0xA7, 0xE6, 0x7D, 0x34, 0xDA, 0xF1, 0x9C, 0x31, 0x1D, 0xC7, 0x48, 0xDC, 0x73, 0xD6, 0x6C, 0x30, 0x98, 0xB7, 0x59, 0x71, 0xEB, 0x27, 0x1B, 0xE0, 0x5A, 0x87, 0xE8, 0xAC, 0x58, 0x91, 0x10, 0xBC, 0xA2, 0x6B, 0x11, 0x41, 0x9A, 0x17, 0x55, 0x26, 0x61, 0xE3, 0xDD, 0xFC, 0x84, 0xAE, 0x16, 0x23, 0xB1, 0x54, 0x90, 0x87, 0x47, 0x3D, 0xA1, 0xBD, 0x57, 0x8E, 0x30, 0x48, 0x19, 0x33, 0x09, 0xF0, 0xF0, 0x44, 0xA9, 0xD5, 0x06, 0xFB, 0xC9, 0x22, 0x41, 0x99, 0x5A, 0x22, 0x76, 0x8F, 0xEA, 0x6E, 0x97, 0x50 };

	// final result we are expecting
	uint8 known_working_code[128] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE9, 0x6E, 0xA1, 0xE6, 0xF0, 0x9E, 0x01, 0xA9, 0x88, 0x85, 0x01, 0xC9, 0xF0, 0x62, 0xE6, 0x38, 0x26, 0xB9, 0xA1, 0x58, 0xBA, 0xB7, 0x76, 0x58, 0xEF, 0x76, 0x62, 0xC0, 0x95, 0xCD, 0x05, 0x6A, 0x74, 0x16, 0x92, 0x38, 0xC4, 0x27, 0xA2, 0xA7, 0x8A, 0x8A, 0x5D, 0xE3, 0x9B, 0xFF, 0xAA, 0x6C, 0x37, 0x98, 0x29, 0x9E, 0x46, 0xB9, 0xA7, 0xB5, 0x17, 0x8B, 0x08, 0xE4, 0xA2, 0xCA, 0x00, 0x40, 0xF6, 0x94, 0x01, 0x8A, 0x97, 0x46, 0xB7, 0xA1, 0xA5, 0x98, 0x50, 0x7E, 0xB5, 0x63, 0x38, 0xD1, 0xB7, 0xF4, 0xF0 };

	// Initialize the bitfield
	for (int byte_pos = 0; byte_pos < 128; byte_pos++)
	{
		for (int bit_pos = 0; bit_pos < 8; bit_pos++)
		{
			all_bits[BIT_POS(byte_pos,bit_pos)] = new node();

			
			// bytes 0x6A5-0x6AC, 0x6B4-0x6C3, and 0x6CA-0x6CC
			if ((byte_pos >= 118 && byte_pos <= 127) || (byte_pos >= 97 && byte_pos <= 112) || (byte_pos >= 88 && byte_pos <= 90))
			{
				all_bits[BIT_POS(byte_pos,bit_pos)]->node_type = NODE_VAL;
				all_bits[BIT_POS(byte_pos,bit_pos)]->value = ((known_working_code[byte_pos] >> bit_pos) & 0x01) == 0x01;
			}
			
			/*
			all_bits[BIT_POS(byte_pos,bit_pos)]->node_type = NODE_VAL;
			all_bits[BIT_POS(byte_pos,bit_pos)]->value = ((known_working_code[127-byte_pos] >> bit_pos) & 0x01) == 0x01;
			*/

			/*
			if ((byte_pos >= 97 && byte_pos <= 112) || (byte_pos >= 117 && byte_pos <= 127) )
			{
				all_bits[BIT_POS(byte_pos,bit_pos)]->node_type = NODE_WC;
				all_bits[BIT_POS(byte_pos,bit_pos)]->byte_pos = byte_pos;
				all_bits[BIT_POS(byte_pos,bit_pos)]->bit_pos = bit_pos;
			}
			*/
		}
	}

	std::vector<rng_info> rng_bits;
	int rng_pos = 0;

	std::vector<int> path;


	attack(all_bits, rng_pos, rng_bits, path, 2);
	return 0;


	// try 0 or 6
	// run alg 0 or 6
	//inverse_alg5(all_bits,rng_pos,rng_bits);
	inverse_alg1(all_bits,rng_pos,rng_bits);
	inverse_alg7(all_bits,rng_pos,rng_bits);
	inverse_alg0(all_bits,rng_pos,rng_bits);
	//print_bitfield(all_bits);

	// we now have a lot of bits in the vector, check them all

	// rng_pos holds the position of the next pos to use, so to get the last position used add 1
	int last_rng_pos = rng_pos + 1;

	int largest_match_count = 0;
	int largest_mach_seed = 0x10000;

	// loop through all RNG seeds
	for (int rng_seed = 0; rng_seed < 0x10000; rng_seed++)
	{
		printf("RNG seed: %04X: ",rng_seed);

		int bits_compared = 0;
		int bits_matched = 0;

		// loop through rng_bits vector
		for (uint32 index = 0; index < rng_bits.size(); index++)
		{
			if (rng_bits[index].bit->node_type != NODE_UNKNOWN)
			{
				// get the actual value for the rng bit
				bool actual_value = get_bit_value(rng_seed, -(last_rng_pos - rng_bits[index].byte_pos), rng_bits[index].bit_pos);

				// evaluate the bit formula
				node * bit_copy = new node(rng_bits[index].bit);

				//printf("%s must equal %i%s\n",bit_copy->to_string().c_str(),actual_value ? 1 : 0, bit_copy->value == actual_value ? " <---" : "");

				bit_copy->eval(rng_seed,last_rng_pos);

				if (bit_copy->node_type != NODE_VAL)
				{

				}

				//printf("%s must equal %i%s\n\n",bit_copy->to_string().c_str(),actual_value ? 1 : 0, bit_copy->value == actual_value ? " <---" : "");

				// compare
				// count how many match
				bits_compared++;
				if (bit_copy->value == actual_value)
				{
					bits_matched++;
				}

				delete bit_copy;
			}
		}

		if (bits_matched > largest_match_count)
		{
			largest_match_count = bits_matched;
			largest_mach_seed = rng_seed;
		}
		printf("\t%i\t / %i\n",bits_matched,bits_compared);
	}

	printf("\nBest match: %04X %i\n",largest_mach_seed,largest_match_count);

	return 0;

	// try 6

	// try the rest
	// try 7
	//   try 0

	

	inverse_alg2(all_bits,rng_pos,rng_bits);
	inverse_alg3(all_bits,rng_pos,rng_bits);
	inverse_alg7(all_bits,rng_pos,rng_bits);
	inverse_alg2(all_bits,rng_pos,rng_bits);
	inverse_alg3(all_bits,rng_pos,rng_bits);
	inverse_alg7(all_bits,rng_pos,rng_bits);
	inverse_alg4(all_bits,rng_pos,rng_bits);
	inverse_alg3(all_bits,rng_pos,rng_bits);
	inverse_alg6(all_bits,rng_pos,rng_bits);

	print_bitfield(all_bits);

	for (uint32 i = 0; i < rng_bits.size(); i++)
	{
		printf("(%i,%i): %s\n",rng_bits[i].byte_pos,rng_bits[i].bit_pos,rng_bits[i].bit->to_string().c_str());
	}

	return 0;
}

void attack(node * bitfield[], int rng_pos, std::vector<rng_info> rng_bits, std::vector<int> path, int depth_limit)
{
	std::vector<int> algs;
	if (path.size() + 1 == depth_limit)
	{
		algs.push_back(0);
		algs.push_back(6);
	}
	else
	{
		//algs.push_back(1);
		algs.push_back(2);
		algs.push_back(3);
		algs.push_back(4);
		algs.push_back(5);
		algs.push_back(7);
	}

	for (size_t alg_index = 0; alg_index < algs.size(); alg_index++)
	{
		// Don't do 7 twice in a row (waste of time, they cancel)
		if (algs[alg_index] == 7 && path.size() > 0 && path.back() == 7)
		{
			continue;
		}

		// Clone the bitfield
		node * bitfield_copy[128 * 8];
		for (int bitfield_index = 0; bitfield_index < 128*8; bitfield_index++)
		{
			bitfield_copy[bitfield_index] = new node(bitfield[bitfield_index]);
		}

		// Clone the path
		std::vector<int> path_copy = path;

		// Add this new algorithm to the path
		path_copy.push_back(algs[alg_index]);

		int rng_pos_copy = rng_pos;
		std::vector<rng_info> rng_bits_copy = rng_bits;

		switch (algs[alg_index])
		{
		case 0:
			inverse_alg0(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;

		case 1:
			inverse_alg1(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;

		case 2:
			inverse_alg2(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;

		case 3:
			inverse_alg3(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;

		case 4:
			inverse_alg4(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;

		case 5:
			inverse_alg5(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;

		case 6:
			inverse_alg6(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;

		case 7:
			inverse_alg7(bitfield_copy,rng_pos_copy,rng_bits_copy);
			break;
		}

		if (algs[alg_index] == 0 || algs[alg_index] == 6)
		{
			// check bits
			for (size_t path_index = 0; path_index < path_copy.size(); path_index++)
			{
				printf("%i ",path_copy[path_index]);
			}

			int last_rng_pos = rng_pos_copy + 1;

			int largest_match_count = 0;
			int largest_mach_seed = 0x10000;

			int bits_to_match = 0;

			// loop through all RNG seeds
			for (int rng_seed = 0; rng_seed < 0x10000; rng_seed++)
			{
				//printf("RNG seed: %04X: ",rng_seed);

				int bits_compared = 0;
				int bits_matched = 0;

				// loop through rng_bits vector
				for (uint32 index = 0; index < rng_bits_copy.size(); index++)
				{
					if (rng_bits_copy[index].bit->node_type != NODE_UNKNOWN)
					{
						// get the actual value for the rng bit
						bool actual_value = get_bit_value(rng_seed, -(last_rng_pos - rng_bits_copy[index].byte_pos), rng_bits_copy[index].bit_pos);

						// evaluate the bit formula
						node * bit_copy = new node(rng_bits_copy[index].bit);

						//printf("%s must equal %i%s\n",bit_copy->to_string().c_str(),actual_value ? 1 : 0, bit_copy->value == actual_value ? " <---" : "");

						bit_copy->eval(rng_seed,last_rng_pos);

						if (bit_copy->node_type != NODE_VAL)
						{

						}

						//printf("%s must equal %i%s\n\n",bit_copy->to_string().c_str(),actual_value ? 1 : 0, bit_copy->value == actual_value ? " <---" : "");

						// compare
						// count how many match
						bits_compared++;
						if (bit_copy->value == actual_value)
						{
							bits_matched++;
						}

						delete bit_copy;
					}
				}

				if (bits_matched > largest_match_count)
				{
					largest_match_count = bits_matched;
					largest_mach_seed = rng_seed;
					bits_to_match = bits_compared;
				}
				//printf("\t%i\t / %i\n",bits_matched,bits_compared);
			}

			printf("\t %04X %i / %i\n",largest_mach_seed,largest_match_count, bits_to_match);
		}
		else
		{
			// recurse
			attack(bitfield_copy,rng_pos_copy,rng_bits_copy,path_copy,depth_limit);
		}

		// destroy copies
		for (int bitfield_index = 0; bitfield_index < 128*8; bitfield_index++)
		{
			delete bitfield_copy[bitfield_index];
		} 
	}
}

// recursive attack:
// pick a depth
// ex: 1

// send bitfield, rng pos, rng bits, path of depth (3-4-5)

// pick an algorithm to invert:
//   all other depths, choose from 1,2,3,4,5,7
//     if previous alg was 7, skip it

// copy the bitfield, rng pos, rng bits, path
// add the alg to the path
// perform the inverse alg
//  recurse(depth+1)
// when we return:
//  destroy the bitfield copy
//  delete the path copy and rng bits
//  loop
//    if we're out of algorithms to try, go up a level

//   if we are at the max depth we can only pick 0 or 6
//   run the alg
//   check all of our bits
//   report our depth
//   report our results (best match)
//   go up a level

