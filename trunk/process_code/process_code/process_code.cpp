#include <stdio.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>

#include "data_sizes.h"
#include "Code.h"
#include "key_schedule.h"

uint16 rng_table[0x100][0x100];

unsigned char carnival_code[0x72] = { 0xF4, 0xD7, 0xD1, 0x9E, 0x46, 0x4F, 0x90, 0xF0, 0xA1, 0x3C, 
									  0x59, 0xA3, 0xFA, 0x09, 0x3C, 0x2A, 0x0B, 0x5A, 0x44, 0x1B, 
									  0x7E, 0x23, 0x72, 0x63, 0xDD, 0xFA, 0x41, 0x27, 0x9A, 0x46, 
									  0x8B, 0xAE, 0xA7, 0xFB, 0xE2, 0xF5, 0x04, 0x01, 0x9A, 0x51,
									  0xC3, 0x7A, 0x35, 0x58, 0x81, 0xAC, 0x59, 0xC2, 0xC3, 0x2A, 
									  0xE4, 0x26, 0xAB, 0x90, 0x1F, 0x52, 0x84, 0xD4, 0xF5, 0x49, 
									  0xC5, 0xE1, 0x55, 0xDC, 0xD8, 0x41, 0x28, 0xD1, 0x43, 0xF6, 
									  0xF7, 0xA6, 0x6E, 0x52, 0xD2, 0xE4, 0x34, 0x39, 0xA1, 0x15, 
									  0x1A, 0x31, 0x13, 0x0F, 0x21, 0xEA, 0xBF, 0x27, 0xF3, 0x23, 
									  0xA4, 0xA0, 0x30, 0x67, 0x43, 0x32, 0x9B, 0x5C, 0xD2, 0xAB, 
									  0x9F, 0x1B, 0x46, 0xD2, 0x7C, 0x3F, 0x6E, 0xD7, 0x23, 0xC8, 
									  0xA6, 0xA1, 0x5E, 0x3D };

unsigned char other_world_code[0x53] = { 0x50, 0xF1, 0xFB, 0x44, 0xBD, 0xC1, 0xB1, 0x5E, 0xE4, 0x18, 
										 0x03, 0x52, 0x1A, 0x1C, 0x93, 0x36, 0x6E, 0x2D, 0x2B, 0x2B, 
										 0xB9, 0x5A, 0xA1, 0x58, 0x7B, 0x32, 0xDB, 0x9A, 0xA3, 0x49, 
										 0x40, 0x12, 0x06, 0x9C, 0xBB, 0x49, 0xAE, 0xB3, 0xFF, 0x67, 
										 0xF0, 0xD2, 0x8F, 0x6E, 0x45, 0xB7, 0xE5, 0x9A, 0x80, 0xAB, 
										 0xFF, 0xD3, 0x98, 0x9A, 0x94, 0x0A, 0x72, 0x81, 0xCF, 0x0A, 
										 0xFF, 0xFB, 0x54, 0xD9, 0x0C, 0xE3, 0x22, 0xF1, 0xE2, 0xD2, 
										 0xF4, 0xC7, 0x86, 0x81, 0x90, 0x0B, 0x04, 0xD2, 0x44, 0x66, 
										 0xC1, 0x68, 0xCA };

void display_working_code(unsigned char * working_code)
{
	for (int i = 127; i >= 0; i--)
	{
		printf("%02X",working_code[i]);
		if (i % 16 == 0)
		{
			//printf("\n");
		}
	}

	printf("\n");
}

void display_code_backup(unsigned char * code_backup)
{
	for (int i = 0; i < 4; i++)
	{
		printf("%02X ",code_backup[i]);
	}

	printf("\n\n");
}
unsigned char rng_real(unsigned char *rng1, unsigned char *rng2);
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


void check_carnival_code(unsigned char * working_code)
{
	unsigned char decrypted_code[0x72];
	unsigned short sum = 0;
	for (int i = 0; i < 0x72; i++)
	{
		decrypted_code[i] = carnival_code[i] ^ working_code[127 - i];
		if (i < 0x72-2)
		{
			sum += decrypted_code[i];
		}
	}

	if ((sum >> 8) == decrypted_code[0x71] && (sum & 0xFF) == decrypted_code[0x70])
	{
		printf("GOOD!\n");
	}
}

void check_other_code(unsigned char * working_code)
{
	unsigned char decrypted_code[0x53];
	unsigned short sum = 0;
	for (int i = 0; i < 0x53; i++)
	{
		decrypted_code[i] = other_world_code[i] ^ working_code[127 - i];
		if (i < 0x53-2)
		{
			sum += decrypted_code[i];
		}
	}

	if ((sum >> 8) == decrypted_code[0x52] && (sum & 0xFF) == decrypted_code[0x51])
	{
		printf("GOOD!\n");
	}
}

int main(void)
{
	// generate RNG table
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

	/*
	unsigned char working_code[128];

	// For the known working code, the payload is the following:
	working_code[0] = 0x2c;
	working_code[1] = 0xa5;
	working_code[2] = 0xb4;
	working_code[3] = 0x2d;
	working_code[4] = 0xf7;
	working_code[5] = 0x3a;
	working_code[6] = 0x26;
	working_code[7] = 0x12;
	*/

	int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11};

	uint8 value[8];

	// For the known working code, the payload is the following:
	value[0] = 0x2c;
	value[1] = 0xa5;
	value[2] = 0xb4;
	value[3] = 0x2d;
	value[4] = 0xf7;
	value[5] = 0x3a;
	value[6] = 0x26;
	value[7] = 0x12;

	key_schedule_data schedule_data;
	schedule_data.as_uint8[0] = value[0];
	schedule_data.as_uint8[1] = value[1];
	schedule_data.as_uint8[2] = value[2];
	schedule_data.as_uint8[3] = value[3];

	key_schedule_entry schedule_entries[27];

	int schedule_counter = 0;
	for (int i = 0; i < 26; i++)
	{
		schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data);

		if (map_list[i] == 0x22)
		{
			schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data,4);
		}
	}

	std::vector<Code> code_list(1,value);

	for (int i = 1; i < 0x1000000; i++)
	{
		value[5] = (i >> 16) & 0xFF;
		value[6] = (i >> 8) & 0xFF;
		value[7] = i & 0xFF;
		code_list.push_back(value);
	}

	schedule_counter = 0;

	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;
	int blah = 0;
	uint64 full_sum = 0;
	for (int i = 0; i < 26; i++)
	{
		/*
		printf("map: %02x\n",map_list[i]);
		printf("rng1: %02x, rng2: %02X, nibble: %04X\t<--NEW\n",schedule_entries[schedule_counter].rng1,schedule_entries[schedule_counter].rng2,schedule_entries[schedule_counter].nibble_selector);
		
		// process working code
		
		schedule_counter++;
		
		if (map_list[i] == 0x22)
		{
			printf("rng1: %02x, rng2: %02X, nibble: %04X\t<--NEW\n",schedule_entries[schedule_counter].rng1,schedule_entries[schedule_counter].rng2,schedule_entries[schedule_counter].nibble_selector);
			
			// process working code
			
			schedule_counter++;
			
		}
		*/

		// Step through the vector and do the map exit on each entry
		uint64 sum = 0;
		for (std::vector<Code>::iterator it = code_list.begin(); it != code_list.end(); ++it)
		{
			auto start = clock::now();
			it->process_map_exit(map_list[i]);
			auto end = clock::now();
			sum += duration_cast<microseconds>(end-start).count();
		}

		std::cout << code_list.size() << " run\n";
		std::cout << sum << "us\n";
		full_sum += sum;
		
		blah++;

		size_t x = code_list.size();
		auto start = clock::now();

		std::sort(code_list.begin(), code_list.end());
		code_list.erase(std::unique(code_list.begin(),code_list.end()),code_list.end());

		auto end = clock::now();
		x -= code_list.size();
		sum = duration_cast<microseconds>(end-start).count();
		full_sum += sum;
		std::cout << x << " deleted\n";
		std::cout << sum << " us\n";

		printf("\n");
	}

	std::cout << full_sum << "\n";

	int x = 0;
	
	std::cout << code_list.size() << "\n";

	return 0;

	/*
	working_code[0] = 0x00;
	working_code[1] = 0x00;
	working_code[2] = 0x00;
	working_code[3] = 0x00;
	working_code[4] = 0x00;
	working_code[5] = 0x00;
	working_code[6] = 0x00;
	working_code[7] = 0x00;
	*/
	/*
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	typedef std::chrono::high_resolution_clock clock;

	auto sum = 0;
	auto full_sum = 0;

	for (int i = 0; i < 0xFFFF; i++)
	{
		working_code[6] = (i & 0xFF00) >> 8;
		working_code[7] = i & 0xFF;
		auto start = clock::now();
		process_code(working_code);
		check_carnival_code(working_code);
		auto end = clock::now();
		
		sum += duration_cast<microseconds>(end-start).count();
		if (i % 0x100 == 0)
		{
			std::cout << i << " " << sum << "ms\n";
			full_sum += sum;
			sum = 0;
		}
	}

	std::cout << full_sum / 256 << "\n";
	*/
	//display_working_code(working_code);
}
