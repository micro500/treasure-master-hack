#include <stdio.h>

#include "data_sizes.h"
#include "rng.h"


uint16 rng_table[0x100][0x100];
uint8 rng_table_extended[0x10000][2048];

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

void generate_rng_table()
{
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

	for (int i = 0; i < 0x10000; i++)
	{
		RNG temp((i>>8) & 0xff, i& 0xff);
		for (int j = 0; j < 2048; j++)
		{
			rng_table_extended[i][j] = temp.run();
		}
	}
}
	
uint8 RNG::run()
{
	uint16 result = rng_table[rng1][rng2];
	rng1 = (result >> 8) & 0xFF;
	rng2 = result & 0xFF;

	return rng1 ^ rng2;
}

void RNG::seed(uint8 a, uint8 b)
{
	rng1 = a;
	rng2 = b;
}

bool get_bit_value(uint16 seed, int byte_num, uint8 bit_number)
{
	if (byte_num < 0)
	{
		printf("BAD\n");
	}
	return (((rng_table_extended[seed][byte_num] >> bit_number) & 0x01) == 0x01 ? true : false);
}