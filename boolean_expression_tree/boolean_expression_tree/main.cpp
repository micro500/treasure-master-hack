#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "tree.h"
#include "data_sizes.h"
#include "qmp.h"
#include "espresso/espresso.h"
#include "espresso/sparse.h"

#include <chrono>
#include <iostream>

const char bit_names[65] = "23456789@#$%abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
/*unsigned char rng_table_old[120] = 
{0xC7, 0xD1, 0xB9, 0x1A, 0x99, 0x77, 0x52, 0x5B,
0x33, 0x94, 0x14, 0xFB, 0xCC, 0x33, 0xC1, 0x3C,
0xFD, 0x92, 0x72, 0xAC, 0x6F, 0x5D, 0x41, 0x36,
0xBA, 0x78, 0x17, 0xD8, 0x18, 0xF7, 0xF8, 0x37,
0xD5, 0x18, 0xC9, 0xD1, 0x02, 0xE1, 0xBC, 0x36,
0xF6, 0x8E, 0x29, 0x82, 0x9A, 0x5F, 0xF2, 0x90,
0x43, 0xEC, 0x9C, 0x41, 0xF4, 0x92, 0x45, 0xFE,
0x8E, 0x33, 0x8E, 0xBF, 0x21, 0x91, 0xAB, 0x12,
0xA7, 0xEB, 0x08, 0x4B, 0xC5, 0xEB, 0x6A, 0x24,
0xCC, 0x08, 0x05, 0xA5, 0x27, 0x6E, 0x78, 0xC0,
0x51, 0x4D, 0x93, 0x8F, 0x1C, 0xA7, 0xF1, 0xE7,
0x7E, 0x38, 0x4F, 0x34, 0x60, 0x89, 0xF8, 0xA2,
0xC9, 0xBE, 0xE2, 0x10, 0x77, 0x2B, 0x58, 0x2B,
0x55, 0xA4, 0x99, 0x8A, 0xF6, 0x45, 0x39, 0x5E,
0x11, 0x53, 0xA2, 0xAF, 0xFB, 0x00, 0x4C, 0x1F};*/
void alg_loop(node * bitfield[], int current_byte, short nibble_selector);
uint16 rng_table[0x100][0x100];
node* all_bits[8 * 128];
union
{
	uint8 as_uint8[4];
	uint16 as_uint16[2];
	uint32 as_uint32;
} code_backup;

void code_backup_algorithm(uint8 algorithm_number, uint8 map_number);
void code_backup_algorithm_0(uint8 map_number);
void code_backup_algorithm_1(uint8 map_number);
void code_backup_algorithm_2(uint8 map_number);
void code_backup_algorithm_3(uint8 map_number);
void code_backup_algorithm_4(uint8 map_number);
void code_backup_algorithm_5(uint8 map_number);
void code_backup_algorithm_6(uint8 map_number);
void code_backup_algorithm_7(uint8 map_number);

unsigned char rng(unsigned char *rng1, unsigned char *rng2)
{
	//return rng_real(rng1,rng2);
	unsigned char rngA = *rng1;
	unsigned char rngB = *rng2;
	unsigned short result = rng_table[rngA][rngB];
	rngA = (result >> 8) & 0xFF;
	rngB = result & 0xFF;

	*rng1 = rngA;
	*rng2 = rngB;
	return rngA ^ rngB;

	
	
	
	return *rng1 ^ *rng2;
}

uint8 rnga, rngb;
unsigned char rng_simple()
{
	return rng(&rnga,&rngb);
}

void seed_rng(uint8 val1,uint8 val2)
{
	rnga = val1;
	rngb = val2;
}
unsigned char rng_real(unsigned char *rng1, unsigned char *rng2)
{
	int rngA = *rng1;
	int rngB = *rng2;	
	// LDA $0436
    // CLC
	unsigned char carry = 0;
    // ADC $0437
    // STA $0437
	// Basically, add rng1 and rng2 together w/o carry and store it to rng2
	rngB = (unsigned char)(rngB + rngA);

	// LDA #$89
    // CLC
    // ADC $0436
	// STA $0436
	// Basically, add #$89 to rng1, and remember the carry for the next addition
	rngA = rngA + 0x89;
	if (rngA > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngA = (unsigned char)rngA;

    // LDA #$2A
    // ADC $0437 = #$AE
    // STA $0437 = #$AE
	rngB = rngB + 0x2A + carry;
	if (rngB > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngB = (unsigned char)rngB;

    // LDA $0436 = #$71
    // ADC #$21
    // STA $0436 = #$71
	rngA = rngA + 0x21 + carry;
	if (rngA > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngA = (unsigned char)rngA;

    // LDA $0437 = #$AE
    // ADC #$43
    // STA $0437 = #$AE
	rngB = rngB + 0x43 + carry;
	if (rngB > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rngB = (unsigned char)rngB;

    // EOR $0436 = #$71
	*rng1 = (unsigned char)rngA;
	*rng2 = (unsigned char)rngB;

	return *rng1 ^ *rng2;
}


void add_byte(int pos,unsigned char val,node * bit_field[])
{
	unsigned char rng_val = val;
	bool r0 = (rng_val & 1) == 1 ? true : false;
	bool r1 = ((rng_val >> 1) & 1) == 1 ? true : false;
	bool r2 = ((rng_val >> 2) & 1) == 1 ? true : false;
	bool r3 = ((rng_val >> 3) & 1) == 1 ? true : false;
	bool r4 = ((rng_val >> 4) & 1) == 1 ? true : false;
	bool r5 = ((rng_val >> 5) & 1) == 1 ? true : false;
	bool r6 = ((rng_val >> 6) & 1) == 1 ? true : false;
	bool r7 = ((rng_val >> 7) & 1) == 1 ? true : false;

	node * inbit0 = bit_field[pos*8];
	node * inbit1 = bit_field[pos*8+1];
	node * inbit2 = bit_field[pos*8+2];
	node * inbit3 = bit_field[pos*8+3];
	node * inbit4 = bit_field[pos*8+4];
	node * inbit5 = bit_field[pos*8+5];
	node * inbit6 = bit_field[pos*8+6];
	node * inbit7 = bit_field[pos*8+7];

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


	g0 = new_node(AND,new_node(inbit0),r0);
	p0 = new_node(OR,new_node(inbit0),r0);
	// Since there is no carry, the (p & c) term would drop out and you are left with g
	carry0 = g0;
	// No carry for the 0th bit
	bit_field[pos*8] = new_node(XOR,inbit0,r0);
	
	g1 = new_node(AND,new_node(inbit1),r1);
	p1 = new_node(OR,new_node(inbit1),r1);
	carry1 = new_node(OR,g1,new_node(AND,p1,new_node(carry0)));
	bit_field[pos*8+1] = new_node(XOR,new_node(XOR,inbit1,r1),carry0);

	g2 = new_node(AND,new_node(inbit2),r2);
	p2 = new_node(OR,new_node(inbit2),r2);
	carry2 = new_node(OR,g2,new_node(AND,p2,new_node(carry1)));
	bit_field[pos*8+2] = new_node(XOR,new_node(XOR,inbit2,r2),carry1);

	g3 = new_node(AND,new_node(inbit3),r3);
	p3 = new_node(OR,new_node(inbit3),r3);
	carry3 = new_node(OR,g3,new_node(AND,p3,new_node(carry2)));
	bit_field[pos*8+3] = new_node(XOR,new_node(XOR,inbit3,r3),carry2);

	g4 = new_node(AND,new_node(inbit4),r4);
	p4 = new_node(OR,new_node(inbit4),r4);
	carry4 = new_node(OR,g4,new_node(AND,p4,new_node(carry3)));
	bit_field[pos*8+4] = new_node(XOR,new_node(XOR,inbit4,r4),carry3);

	g5 = new_node(AND,new_node(inbit5),r5);
	p5 = new_node(OR,new_node(inbit5),r5);
	carry5 = new_node(OR,g5,new_node(AND,p5,new_node(carry4)));
	bit_field[pos*8+5] = new_node(XOR,new_node(XOR,inbit5,r5),carry4);

	g6 = new_node(AND,new_node(inbit6),r6);
	p6 = new_node(OR,new_node(inbit6),r6);
	carry6 = new_node(OR,g6,new_node(AND,p6,new_node(carry5)));
	bit_field[pos*8+6] = new_node(XOR,new_node(XOR,inbit6,r6),carry5);

	bit_field[pos*8+7] = new_node(XOR,new_node(XOR,inbit7,r7),carry6);
}

void xor_byte(int pos,unsigned char val,node * bit_field[])
{
	unsigned char rng_val = val;
	bool r0 = (rng_val & 1) == 1 ? true : false;
	bool r1 = ((rng_val >> 1) & 1) == 1 ? true : false;
	bool r2 = ((rng_val >> 2) & 1) == 1 ? true : false;
	bool r3 = ((rng_val >> 3) & 1) == 1 ? true : false;
	bool r4 = ((rng_val >> 4) & 1) == 1 ? true : false;
	bool r5 = ((rng_val >> 5) & 1) == 1 ? true : false;
	bool r6 = ((rng_val >> 6) & 1) == 1 ? true : false;
	bool r7 = ((rng_val >> 7) & 1) == 1 ? true : false;

	bit_field[pos*8] = new_node(XOR,bit_field[pos*8],new_node(r0));
	bit_field[pos*8+1] = new_node(XOR,bit_field[pos*8+1],new_node(r1));
	bit_field[pos*8+2] = new_node(XOR,bit_field[pos*8+2],new_node(r2));
	bit_field[pos*8+3] = new_node(XOR,bit_field[pos*8+3],new_node(r3));
	bit_field[pos*8+4] = new_node(XOR,bit_field[pos*8+4],new_node(r4));
	bit_field[pos*8+5] = new_node(XOR,bit_field[pos*8+5],new_node(r5));
	bit_field[pos*8+6] = new_node(XOR,bit_field[pos*8+6],new_node(r6));
	bit_field[pos*8+7] = new_node(XOR,bit_field[pos*8+7],new_node(r7));
}


void alg0(node * bit_field[])
{
	for (int i = 0x7F; i >= 0; i--)
	{
		// get the next rng val
		unsigned char rng_val = rng_simple();
			
		//shift the current byte up once
		free_tree(bit_field[i*8 + 7]);
		bit_field[i*8 + 7] = bit_field[i*8 + 6];
		bit_field[i*8 + 6] = bit_field[i*8 + 5];
		bit_field[i*8 + 5] = bit_field[i*8 + 4];
		bit_field[i*8 + 4] = bit_field[i*8 + 3];
		bit_field[i*8 + 3] = bit_field[i*8 + 2];
		bit_field[i*8 + 2] = bit_field[i*8 + 1];
		bit_field[i*8 + 1] = bit_field[i*8];
		bit_field[i*8] = new_node((rng_val & 0x80) == 0x80 ? true : false);
	}
}

void alg1(node * bit_field[])
{
	for (int i = 0x7F; i >= 0; i--)
	{
		// get the next rng val
		unsigned char rng_val = rng_simple();
			
		add_byte(i,rng_val,bit_field);
	}
}


void alg2(node * bit_field[])
{
	node * carry = new_node(((rng_simple() >> 7) & 1) == 1 ? true : false);
	node * next_carry;
	for (int i = 0x7F; i >= 0; i-=2)
	{
		next_carry = bit_field[(i-1)*8];

		// roll i-1 right, make the highest bit = highest bit of i
		// roll i left, make lowest bit new carry
			
		bit_field[(i-1)*8] = bit_field[(i-1)*8+1];
		bit_field[(i-1)*8+1] = bit_field[(i-1)*8+2];
		bit_field[(i-1)*8+2] = bit_field[(i-1)*8+3];
		bit_field[(i-1)*8+3] = bit_field[(i-1)*8+4];
		bit_field[(i-1)*8+4] = bit_field[(i-1)*8+5];
		bit_field[(i-1)*8+5] = bit_field[(i-1)*8+6];
		bit_field[(i-1)*8+6] = bit_field[(i-1)*8+7];
		bit_field[(i-1)*8+7] = bit_field[i*8+7];

		bit_field[i*8+7] = bit_field[i*8+6];
		bit_field[i*8+6] = bit_field[i*8+5];
		bit_field[i*8+5] = bit_field[i*8+4];
		bit_field[i*8+4] = bit_field[i*8+3];
		bit_field[i*8+3] = bit_field[i*8+2];
		bit_field[i*8+2] = bit_field[i*8+1];
		bit_field[i*8+1] = bit_field[i*8];
		bit_field[i*8] = carry;

		carry = next_carry;

	}
	free_tree(next_carry);
}

void alg3(node * bit_field[])
{
	/*
	for (int i = 0x7F; i >= 0; i--)
	{
		// JSR $F1DA
		// EOR $0200,X *
		// STA $0200,X
		working_code.as_uint8[i] = working_code.as_uint8[i] ^ rng();
			
		// DEX
		// BPL $17756
	}
	*/
	for (int i = 0x7F; i >= 0; i--)
	{
		// get the next rng val
		unsigned char rng_val = rng_simple();
			
		xor_byte(i,rng_val,bit_field);
	}
}

void alg4(node * bit_field[])
{
	for (int i = 0x7F; i >= 0; i--)
	{
		// get the next rng val
		unsigned char rng_val = rng_simple();
			
		add_byte(i,((rng_val ^ 0xFF) + 1),bit_field);
	}
}

void alg5(node * bit_field[])
{
	node * carry = new_node(((rng_simple() >> 7) & 1) == 1 ? true : false);
	node * next_carry;
	for (int i = 0x7F; i >= 0; i-=2)
	{
		next_carry = bit_field[(i-1)*8+7];

		// roll i-1 right, make the highest bit = highest bit of i
		// roll i left, make lowest bit new carry

		bit_field[(i-1)*8+7] = bit_field[(i-1)*8+6];
		bit_field[(i-1)*8+6] = bit_field[(i-1)*8+5];
		bit_field[(i-1)*8+5] = bit_field[(i-1)*8+4];
		bit_field[(i-1)*8+4] = bit_field[(i-1)*8+3];
		bit_field[(i-1)*8+3] = bit_field[(i-1)*8+2];
		bit_field[(i-1)*8+2] = bit_field[(i-1)*8+1];
		bit_field[(i-1)*8+1] = bit_field[(i-1)*8];
		bit_field[(i-1)*8] = bit_field[i*8];
			
		bit_field[i*8] = bit_field[i*8+1];
		bit_field[i*8+1] = bit_field[i*8+2];
		bit_field[i*8+2] = bit_field[i*8+3];
		bit_field[i*8+3] = bit_field[i*8+4];
		bit_field[i*8+4] = bit_field[i*8+5];
		bit_field[i*8+5] = bit_field[i*8+6];
		bit_field[i*8+6] = bit_field[i*8+7];
		bit_field[i*8+7] = carry;

		carry = next_carry;

	}
	free_tree(next_carry);
}

void alg6(node * bit_field[])
{
	for (int i = 0x00; i < 0x80; i++)
	{
		// get the next rng val
		unsigned char rng_val = rng_simple();
			
		//shift the current byte up once
		free_tree(bit_field[i*8]);
		bit_field[i*8] = bit_field[i*8 + 1];
		bit_field[i*8 + 1] = bit_field[i*8 + 2];
		bit_field[i*8 + 2] = bit_field[i*8 + 3];
		bit_field[i*8 + 3] = bit_field[i*8 + 4];
		bit_field[i*8 + 4] = bit_field[i*8 + 5];
		bit_field[i*8 + 5] = bit_field[i*8 + 6];
		bit_field[i*8 + 6] = bit_field[i*8 + 7];
		bit_field[i*8 + 7] = new_node((rng_val & 0x80) == 0x80 ? true : false);
	}
}

void alg7(node * bit_field[])
{
	for (int i = 0x00; i < 128 * 8; i++)
	{
		bit_field[i] = new_node(XOR,bit_field[i],true);
	}
}

void code_backup_algorithm(uint8 algorithm_number, uint8 map_number)
{
	//printf("code_backup_algorithm: %i\n", algorithm_number);
	if (algorithm_number == 0x00)
	{
		code_backup_algorithm_0(map_number);
	}
	else if (algorithm_number == 0x01)
	{
		code_backup_algorithm_1(map_number);
	}
	else if (algorithm_number == 0x02)
	{
		code_backup_algorithm_2(map_number);
	}
	if (algorithm_number == 0x03)
	{
		code_backup_algorithm_3(map_number);
	}
	else if (algorithm_number == 0x04)
	{
		code_backup_algorithm_4(map_number);
	}
	else if (algorithm_number == 0x05)
	{
		code_backup_algorithm_5(map_number);
	}
	else if (algorithm_number == 0x06)
	{
		code_backup_algorithm_6(map_number);
	}
	else if	(algorithm_number == 0x07)
	{
		code_backup_algorithm_7(map_number);
	}
}

void code_backup_algorithm_0(uint8 map_number)
{
	unsigned char temp = map_number ^ code_backup.as_uint8[1];
	unsigned char carry = (((int)temp + code_backup.as_uint8[0]) >> 8) & 0x01;
	temp = temp + code_backup.as_uint8[0];
	unsigned char next_carry = (((int)temp - code_backup.as_uint8[2] - (1 - carry)) < 0 ? 0 : 1);
	temp = temp - code_backup.as_uint8[2] - (1 - carry);
	carry = next_carry;

	unsigned char rolling_sum = temp;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + code_backup.as_uint8[i] + carry) >> 8) & 0x01;
		rolling_sum += code_backup.as_uint8[i] + carry;
		code_backup.as_uint8[i] = rolling_sum;

		carry = next_carry;
	}
}

void code_backup_algorithm_1(uint8 map_number)
{
	unsigned char carry = 1;
	unsigned char rolling_sum = map_number;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + code_backup.as_uint8[i] + carry) >> 8) & 0x01;
		rolling_sum += code_backup.as_uint8[i] + carry;
		code_backup.as_uint8[i] = rolling_sum;
		carry = next_carry;
	}
}

void code_backup_algorithm_2(uint8 map_number)
{
	// Add the map number to the first byte
	code_backup.as_uint8[0] += map_number;

	unsigned char temp[4];
	temp[0] = code_backup.as_uint8[0];
	temp[1] = code_backup.as_uint8[1];
	temp[2] = code_backup.as_uint8[2];
	temp[3] = code_backup.as_uint8[3];

	// Reverse the code:
	code_backup.as_uint8[0] = temp[3];
	code_backup.as_uint8[1] = temp[2];
	code_backup.as_uint8[2] = temp[1];
	code_backup.as_uint8[3] = temp[0];
}

void code_backup_algorithm_3(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_2(map_number);
	//display_code_backup(code_backup);
	// Then alg 1
	code_backup_algorithm_1(map_number);
}

void code_backup_algorithm_4(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_2(map_number);
	//display_code_backup(code_backup);
	// Then alg 1
	code_backup_algorithm_0(map_number);
}

void code_backup_algorithm_5(uint8 map_number)
{
	// Do an ASL on the map number
	unsigned char temp = map_number << 1;

	// EOR #$FF
	temp = temp ^ 0xFF;
	temp = temp + code_backup.as_uint8[0];
	temp = temp - map_number;
	code_backup.as_uint8[0] = temp;
	
	unsigned char temp2[4];
	temp2[1] = code_backup.as_uint8[1];
	temp2[2] = code_backup.as_uint8[2];
	temp2[3] = code_backup.as_uint8[3];

	code_backup.as_uint8[1] = temp2[3];
	code_backup.as_uint8[2] = temp2[1];
	code_backup.as_uint8[3] = temp2[2];
}

void code_backup_algorithm_6(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_5(map_number);

	// Then alg 1
	code_backup_algorithm_1(map_number);
}

void code_backup_algorithm_7(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_5(map_number);

	// Then alg 1
	code_backup_algorithm_0(map_number);
}

void alg(unsigned algnum,node * bit_field[])
{
	switch (algnum)
	{
		case 0:
			alg0(bit_field);
			break;
		case 1:
			alg1(bit_field);
			break;
		case 2:
			alg2(bit_field);
			break;
		case 3:
			alg3(bit_field);
			break;
		case 4:
			alg4(bit_field);
			break;
		case 5:
			alg5(bit_field);
			break;
		case 6:
			alg6(bit_field);
			break;
		case 7:
			alg7(bit_field);
			break;
	}
}

void print_all_bits()
{
	for (int i = 0; i < 128 * 8; i++)
	{
		printf("%s\n",all_bits[i]->get_string().c_str());
		if (i % 8 == 7)
		{
			printf("\n");
		}
	}
}

int main()
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;

	for (int i = 0; i < 0x100; i++)
	{
		for (int j = 0; j < 0x100; j++)
		{
			unsigned char rng1 = i;
			unsigned char rng2 = j;
			rng_real(&rng1,&rng2);
			rng_table[i][j] = rng1 << 8 | rng2;
		}
	}
	
	
			
	uint8 value[8];

	// For the known working code, the payload is the following:
	value[0] = 0x2c;
	value[1] = 0xa5;
	value[2] = 0xb4;
	value[3] = 0x2d;
	value[4] = 0x27;
	value[5] = 0x3a;
	value[6] = 0x26;
	value[7] = 0x12;

	/*
	value[0] = 0x00;
	value[1] = 0x00;
	value[2] = 0x00;
	value[3] = 0x00;
	*/
	value[4] = 0x00;
	value[5] = 0x00;
	value[6] = 0x00;
	value[7] = 0x00;


	for (int i = 0; i < 0x80; i++)
	{
		if (i % 8 < 4)
		{
			for (int j = 0; j < 8; j++)
			{
				all_bits[i*8+j] = new_node(((value[i%8] >> j) & 1) == 1 ? true : false);
			}
		}
		else
		{
			for (int j = 0; j < 8; j++)
			{
				all_bits[i*8+j] = new_node(bit_names[i%8*8+j]);
			}
		}
	}

	// seed for key = 0 is 0x00 0x00
	//seed_rng(0x00,0x00);

	// seed for known key is 0x2C 0xa5
	//seed_rng(0x2C, 0xA5);

	printf("seeding RNG for expansion: %02X %02X\n",value[0],value[1]);
	seed_rng(value[0],value[1]);

	unsigned char sums[8] = {0,0,0,0,0,0,0,0};
	for (int i = 8; i < 0x80; i++)
	{
		sums[i%8] += rng_simple();
		add_byte(i,sums[i%8],all_bits);
	}
	


	// Expansion complete!
	code_backup.as_uint8[0] = value[0];
	code_backup.as_uint8[1] = value[1];
	code_backup.as_uint8[2] = value[2];
	code_backup.as_uint8[3] = value[3];

	unsigned char map_number = 0;

	unsigned char key_algorithm_number = (code_backup.as_uint8[(map_number >> 4) & 0x03] >> 2) & 0x07;
	printf("map num: %02X\n",map_number);
	printf("shift: %02X\n",(map_number >> 4));
	printf("and: %02X\n",(map_number >> 4) & 0x03);
	printf("byte: %02X\n",code_backup.as_uint8[(map_number >> 4) & 0x03]);
	printf("shift: %02X\n",(code_backup.as_uint8[(map_number >> 4) & 0x03] >> 2));		 
	printf("alg#: %02X\n",(code_backup.as_uint8[(map_number >> 4) & 0x03] >> 2) & 0x07);		 

	code_backup_algorithm(key_algorithm_number, map_number);

	//process_working_code(map_number);


	// From the key schedule for known key, seed rng to 0xB4 0x86
	//seed_rng(0xB4,0x86);
	printf("seeding RNG from key schedule: %02X %02X\n",code_backup.as_uint8[0],code_backup.as_uint8[1]);
	seed_rng(code_backup.as_uint8[0],code_backup.as_uint8[1]);

	short nibble_selector = (code_backup.as_uint8[3] << 8) + code_backup.as_uint8[2];
	// The code is pulled out of the PPU
	printf("%04x\n",nibble_selector);

	auto start = clock::now();
	alg_loop(all_bits,0,nibble_selector);

	for (int l = 0; l < 128 * 8; l++)
	{
		// where don't care is false, assign that bit to the value in implicant
		free_tree(all_bits[l]);
	}

	auto end = clock::now();
	std::cout << duration_cast<microseconds>(end-start).count() << "us\n";

	_CrtDumpMemoryLeaks();

	return 0;
}

vector<Implicant> reduce_table_es(int var_count, vector<unsigned int> minterms)
{
	vector<Implicant> implicants2;

	debug = 0;			/* default -d: no debugging info */
	verbose_debug = FALSE;	/* default -v: not verbose */
	print_solution = TRUE;	/* default -x: print the solution (!) */
	summary = FALSE;		/* default -s: no summary */
	trace = FALSE;		/* default -t: no trace information */
	remove_essential = TRUE;	/* default -e: */
	force_irredundant = TRUE;
	unwrap_onset = TRUE;
	single_expand = FALSE;
	pos = FALSE;
	recompute_onset = FALSE;
	use_super_gasp = FALSE;
	use_random_order = FALSE;
	kiss = FALSE;
	echo_comments = TRUE;
	echo_unknown_commands = TRUE;

	cube.num_binary_vars = var_count;
	cube.num_vars = cube.num_binary_vars + 1;
	cube.part_size = ALLOC(int, cube.num_vars);
	cube.part_size[cube.num_vars-1] = 1;
	cube_setup();

	pPLA PLA = new_PLA();
	PLA->pla_type = FD_type;

	if (PLA->F == NULL) {
		PLA->F = new_cover(10);
		PLA->D = new_cover(10);
		PLA->R = new_cover(10);
	}

	for (int minnum = 0; minnum < minterms.size(); minnum++)
	{
		pcube cf = cube.temp[0], cr = cube.temp[1], cd = cube.temp[2];
		set_clear(cf, cube.size);


		for (int varnum = 0; varnum < var_count; varnum++)
		{
			if (((minterms[minnum] >> varnum) & 1))
			{
				set_insert(cf, varnum*2+1);
			}
			else
			{
				set_insert(cf, varnum*2);
			}
		}
		set_copy(cr, cf);
		set_copy(cd, cf);
		set_insert(cf, cube.first_part[var_count]);
		PLA->F = sf_addset(PLA->F, cf);
	}

	PLA->R = complement(cube2list(PLA->F, PLA->D));

	//PLA->F = espresso(PLA->F, PLA->D, PLA->R);

//	fprint_pla(stdout, PLA, F_type);
//	pcover Fold = sf_save(PLA->F);


	PLA->F = minimize_exact(PLA->F, PLA->D, PLA->R, TRUE);
	//PLA->F = espresso(PLA->F, PLA->D, PLA->R);


//	fprint_pla(stdout, PLA, F_type);

	pcube last, p;
	foreach_set(PLA->F, last, p) {
		int imp = 0;
		int dc = 0;
		for(int var = 0; var < cube.num_binary_vars; var++) {
			int val = GETINPUT(p, var);
			switch (val){
			case 2:
				imp |= 1 << var;
				break;
			case 3:
				dc |= 1 << var;
				break;
			}
		}
		Implicant temp(var_count,imp,vector<unsigned int>(),dc);
		implicants2.push_back(temp);
	}

	free_PLA(PLA);
    FREE(cube.part_size);
    setdown_cube();             /* free the cube/cdata structure data */
    sf_cleanup();               /* free unused set structures */
    sm_cleanup();               /* sparse matrix cleanup */

	return implicants2;
}


int varcount = 0;

void alg_loop(node * bitfield[], int current_byte, short nibble_selector)
{
	for (int i = current_byte; i < 16; i++)
    {
        // Get the highest bit of the nibble selector to use as a flag
        unsigned char nibble = (nibble_selector >> (15-i)) & 0x01;

        // If the flag is a 1, get the high nibble of the current byte
        // Otherwise use the low nibble
		/*
        unsigned char current_byte = 0xFF; // GET REAL BYTE
        if (nibble == 1)
        {
                current_byte = current_byte >> 4;
        }

        // Mask off only 3 bits
		// do the magic to get the algorithm number
        unsigned char algorithm_number = (current_byte >> 1) & 0x07;
          */


		unsigned char result = 0;
		// we now need to get the 3 bits

		node * bit2;
		node * bit1;
		node * bit0;

		//cout << "Grabbing these bits: ";

		if (nibble == 1)
		{
			//cout << i*8+7 << " " << i*8+6 << " " << i*8+5 << "\n";
			bit2 = bitfield[i*8+7] = bitfield[i*8+7]->simplify();
			bit1 = bitfield[i*8+6] = bitfield[i*8+6]->simplify();
			bit0 = bitfield[i*8+5] = bitfield[i*8+5]->simplify();
		}
		else
		{
			//cout << i*8+3 << " " << i*8+2 << " " << i*8+1 << "\n";
			bit2 = bitfield[i*8+3] = bitfield[i*8+3]->simplify();
			bit1 = bitfield[i*8+2] = bitfield[i*8+2]->simplify();
			bit0 = bitfield[i*8+1] = bitfield[i*8+1]->simplify();
		}

		// If algorithm number is a constant, just run it and keep going
		if (bit2->type == VAL && bit1->type == VAL && bit0->type == VAL)
		{
			result = ((bit2->val ? 1 : 0) << 2) | ((bit1->val ? 1 : 0) << 1) | (bit0->val ? 1 : 0);
			unsigned char algorithm_number = result & 0x07;
			alg(algorithm_number,bitfield);
		}
		else
		{
			
			//printf("Need evaluation:\n");
			//printf("%s\n%s\n%s\n",bit2->get_string().c_str(),bit1->get_string().c_str(),bit0->get_string().c_str());
			
			
			/*
			bit2 = remove_xor(bit2);
			//printf("\n%s\n",bit2->get_string().c_str());

			bit1 = remove_xor(bit1);
			//printf("\n%s\n",bit1->get_string().c_str());

			bit0 = remove_xor(bit0);
			//printf("\n%s\n",bit0->get_string().c_str());
			
			bit2 = canon(remove_xor(bit2));
			//printf("\n%s\n",bit2->get_string().c_str());
			*/

			// Get the variables used in all three equations
			std::vector<char> vars_bit2 = bit2->get_vars();
			std::vector<char> vars_bit1 = bit1->get_vars();
			std::vector<char> vars_bit0 = bit0->get_vars();
			
			// Combine them into one vector
			std::vector<char> vars(vars_bit2);
			vars.insert(vars.end(),vars_bit1.begin(),vars_bit1.end());
			vars.insert(vars.end(),vars_bit0.begin(),vars_bit0.end());

			// Remove duplicates
			std::sort(vars.begin(), vars.end());
			vars.erase(std::unique(vars.begin(),vars.end()),vars.end());

			/*
			printf("Variables used: ");
			for(std::vector<char>::iterator it = vars.begin(); it != vars.end(); ++it) {
				printf("%c ",*it);
			}
			printf("\n");
			printf("%d vars\n",vars.size());
			*/
			
			if (vars.size() > 31)
			{
				// We'll need to see if this happens, and deal with it if/when it does
				printf("TOO MANY VARS\n");
				return;
			}

			// Create a storage unit for the implicants for each algorithm option
			std::vector<std::vector<unsigned int>> minterms;

			minterms.push_back(std::vector<unsigned int>());
			minterms.push_back(std::vector<unsigned int>());
			minterms.push_back(std::vector<unsigned int>());
			minterms.push_back(std::vector<unsigned int>());
			minterms.push_back(std::vector<unsigned int>());
			minterms.push_back(std::vector<unsigned int>());
			minterms.push_back(std::vector<unsigned int>());
			minterms.push_back(std::vector<unsigned int>());

			// if it is an equation, fill truth tables
			unsigned int space = 1 << vars.size();
			for (unsigned int j = 0; j < space; j++)
			{
				node* bit2_copy = new_node(bit2);
				node* bit1_copy = new_node(bit1);
				node* bit0_copy = new_node(bit0);
				
				for (size_t k = 0; k < vars.size(); k++)
				{
					bit2_copy->assign_var(vars[k],((j >> k) & 1) == 1 ? true : false);
					bit1_copy->assign_var(vars[k],((j >> k) & 1) == 1 ? true : false);
					bit0_copy->assign_var(vars[k],((j >> k) & 1) == 1 ? true : false);
					/*printf("%c = %d ",vars[k],((j >> k) & 1));*/
				}

				bit2_copy = bit2_copy->simplify();
				bit1_copy = bit1_copy->simplify();
				bit0_copy = bit0_copy->simplify();

				if (bit2_copy->type != VAL || bit1_copy->type != VAL || bit0_copy->type != VAL)
				{
					printf("\n\nEVALUATION FAILURE? THIS SHOULDN'T HAPPEN\n");
					return;
				}
				/*
				printf("result = %d %d %d\n",bit2_copy->val ? 1 : 0,bit1_copy->val ? 1 : 0,bit0_copy->val ? 1 : 0);
				*/
				unsigned char result = ((bit2_copy->val ? 1 : 0) << 2) | ((bit1_copy->val ? 1 : 0) << 1) | (bit0_copy->val ? 1 : 0);
				minterms[result].push_back(j);

				free_tree(bit2_copy);
				free_tree(bit1_copy);
				free_tree(bit0_copy);
			}

			/*
			for (int j = 0; j < 8; j++)
			{
				printf("Alg %d options: %d\n",j,minterms[j].size());
			}
			*/

			// loop through possible algorithms, pick an algorithm that is possible
			for (int j = 0; j < 8; j++)
			{
				char bits[26] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};
				if (minterms[j].size() > 0)
				{
					//printf("For alg %d\n",j);
					// reduce the truth table using qmp
					//vector<Implicant> implicants = reduce_table(vars.size(),minterms[j]);
					vector<Implicant> implicants = reduce_table_es(vars.size(),minterms[j]);

					/*
					// display the results...
					for (size_t k = 0; k < implicants.size(); k++)
					{

						for (int l = 0; l < vars.size(); l++)
						{
							if (((implicants[k].dont_care_mask >> l) & 1) == 0)
							{
								printf("%c",vars[l]);
								if (((implicants[k].implicant >> l) & 1) == 0)
								{
									printf("'");
								}
								printf(" ");
							}
						}
						printf("\n");
					}
					*/
					
					// loop through the implicants
					node* bitfield_clone[128 * 8];
					for (size_t k = 0; k < implicants.size(); k++)
					{
						int temp_varcount = 0;
						// clone the bitfield
						for (int l = 0; l < 128 * 8; l++)
						{
							// where don't care is false, assign that bit to the value in implicant
							bitfield_clone[l] = new_node(bitfield[l]);
							for (size_t m = 0; m < vars.size(); m++)
							{
								if (((implicants[k].dont_care_mask >> m) & 1) == 0)
								{
									if (((implicants[k].implicant >> m) & 1) == 0)
									{
										bitfield_clone[l]->assign_var(vars[m],false);
										//printf("%c = %d ",vars[m],0);
									}
									else
									{
										bitfield_clone[l]->assign_var(vars[m],true);
										//printf("%c = %d ",vars[m],1);
									}
									
								}
							}
							bitfield_clone[l] = bitfield_clone[l]->simplify();
							//printf("\n");
						}

						/*
						for (size_t m = 0; m < vars.size(); m++)
						{
							if (((implicants[k].dont_care_mask >> m) & 1) == 0)
							{
								cout << vars[m] << " = " << ((implicants[k].implicant >> m) & 1) << "\n";
								temp_varcount++;
							}
						}
						*/

						varcount += temp_varcount;
						//if (i == 15)
						//cout << varcount << " vars assigned\n";
						if (varcount == 32)
							cout << "ALL ASSIGNED\n";

						//printf("Running alg %d\n",j);
						// run the selected algorithm on the bitfield copy
						alg(j,bitfield_clone);
						//printf("Recursing. Our current level is: %d\n",i);
						printf("%d",i);
						for (int tabs = 0; tabs < i; tabs++)
						{
							printf("\t");
						}
						printf("%d/7 %d/%d\n",j,k+1,implicants.size());
						// RECURSE
						alg_loop(bitfield_clone,i+1,nibble_selector);
						varcount -= temp_varcount;
						// delete the bitfield
						for (int l = 0; l < 128 * 8; l++)
						{
							// where don't care is false, assign that bit to the value in implicant
							free_tree(bitfield_clone[l]);
						}
						//if (i == 4) return;
					}

					//return;
				}
			}
		}

					
					// LOOP
		
					
					// working_code_alg

					
					// when that comes back we have run through all the possibilities for this implicant
				// }
				// we will now go try the next algorithm
			// }
			// We have tried all possible algorithms. We need to step up a level in the recursion
			// return;
		// }

        // Run the selected algorithm
        //working_code_algorithm(algorithm_number, map_number);
    }
	//printf("Ended!\n");
	
	// If we reached here we have made it to the end of this step. We should reduce the bitfield and see what vars remains

	// I guess we'll end up with something like:
	// u = 1
	// v = 1
	// w = 0
	// x = x
	// y = y
	// z = -
	// A = -
	// B = -

	// u, v, w, were set at some point during our traversal
	// x, y, still exist in the equations. Those are the bits of randomness that the bruteforcer would have to go through
	// z, A, B were not assigned, and don't exist any more. In the end these don't affect the output, given the set variables above
	// so we have something like:
	
	// Count the number of still random bits (2 in this example)
	// for now add 2^that to a total to count how many unique we have, to see if this is working

	// Later...

	// We have:
	// uvwxyzAB
	// 110??---

	// where there is a -, set it to 0
	// 110??---

	// 110??000

	// For each random bit, assign and add it to a list of IVs to try
	// When we finish all this recursion they will be run
}