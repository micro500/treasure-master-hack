#include <stdio.h>
#include <chrono>
#include <iostream>

unsigned short rng_table[0x100][0x100];

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

void working_code_algorithm(unsigned char algorithm_number, unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	//printf("alg #: %i\n",algorithm_number);
	//unsigned char temp_bytes[128];
	//memcpy(temp_bytes,bytes,128);
	if (algorithm_number == 0x00)
	{
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i--)
		{
			// JSR $F1DA
			// ASL A
			// ROL $0200,X
			working_code[i] = (working_code[i] << 1) | ((rng(rng1,rng2) >> 7) & 0x01);

			// DEX
			// BPL $17728
		}
	}
	else if (algorithm_number == 0x01)
	{
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i--)
		{
			// JSR $F1DA
			// CLC
			// ADC $0200,X *
			// STA $0200,X
			working_code[i] = working_code[i] + rng(rng1,rng2);

			// DEX
			// BPL $17735
		}
	}
	else if (algorithm_number == 0x02)
	{
		// JSR $F1DA
		// ASL A
		unsigned char carry = (rng(rng1,rng2) >> 7) & 0x01;
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i-=2)
		{
			// This gets ugly. Just a warning...
			unsigned char next_carry = working_code[i-1] & 0x01;
			// ROL $0200,X
			// DEX
			// ROR $0200,X
			// DEX
			working_code[i-1] = (working_code[i-1] >> 1) | (working_code[i] & 0x80);
			working_code[i] = (working_code[i] << 1) | (carry & 0x01);

			carry = 0x00 | next_carry;

			// BPL $17749
		}
	}
	else if (algorithm_number == 0x03)
	{
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i--)
		{
			// JSR $F1DA
			// EOR $0200,X *
			// STA $0200,X
			working_code[i] = working_code[i] ^ rng(rng1,rng2);
			
			// DEX
			// BPL $17756
		}
	}
	else if (algorithm_number == 0x04)
	{
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i--)
		{
			// JSR $F1DA
			// EOR #$FF
			// SEC
			// ADC $0200,X *
			// STA $0200,X
			working_code[i] = working_code[i] + (rng(rng1,rng2)^0xFF) + 1;

			// DEX
			// BPL $17765
		}
	}
	else if (algorithm_number == 0x05)
	{
		// JSR $F1DA
		// ASL A
		unsigned char carry = rng(rng1,rng2) & 0x80;
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i-=2)
		{
			// This gets ugly. Just a warning...
			unsigned char next_carry = working_code[i-1] & 0x80;
			// ROR $0200,X
			// DEX
			// ROL $0200,X
			// DEX
			
			working_code[i-1] = (working_code[i-1] << 1) | (working_code[i] & 0x01);
			working_code[i] = (working_code[i] >> 1) | carry;

			carry = next_carry;

			// BPL $1777B
		}
	}
	else if (algorithm_number == 0x06)
	{
		// LDX #$00
		for (int i = 0x00; i <= 0x7f; i++)
		{
			// JSR $F1DA
			// ASL A
			// ROR $0200,X
			working_code[i] = (working_code[i] >> 1) | (rng(rng1,rng2) & 0x80);

			// INX
			// BPL $17788	
		}
	}
	else if (algorithm_number == 0x07)
	{
		// LDX #$7F
		for (int i = 0x00; i < 0x80; i++)
		{
			// LDA $0200,X *
			// EOR #$FF
			// STA $0200,X

			working_code[i] = working_code[i] ^ 0xFF;
			//printf("%08llX\n",*((unsigned long long *)(&working_code[i*8])));
			//printf("%04X",*((unsigned int *)(&working_code[i*8])));
			//printf(" %04X\n",*((unsigned int *)(&working_code[i*8+4])));
			//printf("%i\n",sizeof(unsigned long long));
			//*((unsigned long long *)(&working_code[i*8])) ^= (unsigned long long)(0xFFFFFFFFFFFFFFFF);
			//*((unsigned char *)(&working_code[i*8])) ^= (unsigned char)(0xFF);
			

			// DEX
			// BPL $17795
		}
		//printf("\n");
	}
	//memcpy(bytes,temp_bytes,128);
	//printf("%02X\n",working_code[0x7f]);
}

void code_backup_algorithm_0(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	unsigned char temp = map_number ^ code_backup[1];
	unsigned char carry = (((int)temp + code_backup[0]) >> 8) & 0x01;
	temp = temp + code_backup[0];
	unsigned char next_carry = (((int)temp - code_backup[2] - (1 - carry)) < 0 ? 0 : 1);
	temp = temp - code_backup[2] - (1 - carry);
	carry = next_carry;

	unsigned char rolling_sum = temp;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + code_backup[i] + carry) >> 8) & 0x01;
		rolling_sum += code_backup[i] + carry;
		code_backup[i] = rolling_sum;

		carry = next_carry;
	}
}

void code_backup_algorithm_1(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	unsigned char carry = 1;
	unsigned char rolling_sum = map_number;

	for (char i = 3; i >= 0; i--)
	{
		unsigned char next_carry = (((int)rolling_sum + code_backup[i] + carry) >> 8) & 0x01;
		rolling_sum += code_backup[i] + carry;
		code_backup[i] = rolling_sum;

		carry = next_carry;
	}
}

void code_backup_algorithm_2(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	// Add the map number to the first byte
	code_backup[0] += map_number;

	unsigned char temp[4];
	temp[0] = code_backup[0];
	temp[1] = code_backup[1];
	temp[2] = code_backup[2];
	temp[3] = code_backup[3];

	// Reverse the code:
	code_backup[0] = temp[3];
	code_backup[1] = temp[2];
	code_backup[2] = temp[1];
	code_backup[3] = temp[0];
}

void code_backup_algorithm_3(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	// Run alg 2 first
	code_backup_algorithm_2(map_number, working_code, code_backup, rng1, rng2);
	//display_code_backup(code_backup);
	// Then alg 1
	code_backup_algorithm_1(map_number, working_code, code_backup, rng1, rng2);
}

void code_backup_algorithm_4(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	// Run alg 2 first
	code_backup_algorithm_2(map_number, working_code, code_backup, rng1, rng2);
	//display_code_backup(code_backup);
	// Then alg 1
	code_backup_algorithm_0(map_number, working_code, code_backup, rng1, rng2);
}

void code_backup_algorithm_5(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	// Do an ASL on the map number
	unsigned char temp = map_number << 1;

	// EOR #$FF
	temp = temp ^ 0xFF;
	temp = temp + code_backup[0];
	temp = temp - map_number;
	code_backup[0] = temp;
	
	unsigned char temp2[4];
	temp2[1] = code_backup[1];
	temp2[2] = code_backup[2];
	temp2[3] = code_backup[3];

	code_backup[1] = temp2[3];
	code_backup[2] = temp2[1];
	code_backup[3] = temp2[2];
}

void code_backup_algorithm_6(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	// Run alg 2 first
	code_backup_algorithm_5(map_number, working_code, code_backup, rng1, rng2);

	// Then alg 1
	code_backup_algorithm_1(map_number, working_code, code_backup, rng1, rng2);
}

void code_backup_algorithm_7(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	// Run alg 2 first
	code_backup_algorithm_5(map_number, working_code, code_backup, rng1, rng2);

	// Then alg 1
	code_backup_algorithm_0(map_number, working_code, code_backup, rng1, rng2);
}

void code_backup_algorithm(unsigned char algorithm_number, unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	//printf("code_backup_algorithm: %i\n", algorithm_number);
	if (algorithm_number == 0x00)
	{
		code_backup_algorithm_0(map_number, working_code, code_backup, rng1, rng2);
	}
	else if (algorithm_number == 0x01)
	{
		code_backup_algorithm_1(map_number, working_code, code_backup, rng1, rng2);
	}
	else if (algorithm_number == 0x02)
	{
		code_backup_algorithm_2(map_number, working_code, code_backup, rng1, rng2);
	}
	if (algorithm_number == 0x03)
	{
		code_backup_algorithm_3(map_number, working_code, code_backup, rng1, rng2);
	}
	else if (algorithm_number == 0x04)
	{
		code_backup_algorithm_4(map_number, working_code, code_backup, rng1, rng2);
	}
	else if (algorithm_number == 0x05)
	{
		code_backup_algorithm_5(map_number, working_code, code_backup, rng1, rng2);
	}
	else if (algorithm_number == 0x06)
	{
		code_backup_algorithm_6(map_number, working_code, code_backup, rng1, rng2);
	}
	else if	(algorithm_number == 0x07)
	{
		code_backup_algorithm_7(map_number, working_code, code_backup, rng1, rng2);
	}
}

void process_working_code(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{

	// The RNG is backed up
	// The RNG is seeded with the first two bytes of the code backup
	*rng1 = code_backup[0];
	*rng2 = code_backup[1];
	// Set up the nibble selection variables
	short nibble_selector = (code_backup[3] << 8) + code_backup[2];
	// The code is pulled out of the PPU

	// Next, the working code is processed with the same steps 16 times:
	for (int i = 0; i < 16; i++)
	{
		// Get the highest bit of the nibble selector to use as a flag
		unsigned char nibble = (nibble_selector >> 15) & 0x01;
		// Shift the nibble selector up one bit
		nibble_selector = nibble_selector << 1;

		// If the flag is a 1, get the high nibble of the current byte
		// Otherwise use the low nibble
		unsigned char current_byte = working_code[i];
		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char algorithm_number = (current_byte >> 1) & 0x07;
		
		// Run the selected algorithm
		working_code_algorithm(algorithm_number, map_number, working_code, code_backup, rng1, rng2);
	}

	// The working code is then put back into the PPU
	// Somehow the game decides which map to go to next, and our work here is done
}

void process_map_exit(unsigned char map_number, unsigned char * working_code, unsigned char * code_backup, unsigned char * rng1, unsigned char * rng2)
{
	// Pick an algorithm number to process the code backup
	// The steps are the following:
	//   Take the top 2 bits of the map number (shift it down 4 times, AND with 0x03)
	//   Use that as an index in the code backup
	//   Take that value, shift it down twice, and take only 3 bits
	//   That is the alg number

	unsigned char algorithm_number = (code_backup[(map_number >> 4) & 0x03] >> 2) & 0x07;

	code_backup_algorithm(algorithm_number, map_number, working_code, code_backup, rng1, rng2);

	process_working_code(map_number, working_code, code_backup, rng1, rng2);
}

void process_code(unsigned char * working_code)
{
	// The first and second bytes are copied to seed the RNG
	unsigned char rng1 = working_code[0];
	unsigned char rng2 = working_code[1];

	// Next the working code is expanded to 128 bytes
	// RNG is run, the return value is added to a byte, then stored 8 bytes later
	for (unsigned char i = 0; i <= (0x7F - 8); i++)
	{
		working_code[i+8] = working_code[i] + rng(&rng1,&rng2);
	}

	// Now a code backup is created and the first 4 bytes are copied to it
	unsigned char code_backup[4];
	code_backup[0] = working_code[0];
	code_backup[1] = working_code[1];
	code_backup[2] = working_code[2];
	code_backup[3] = working_code[3];

	// ---- GAME PLAY RESUMES ----
	// ---- ONCE THE FIRST MAP IS EXITED... ----

	// Now the steps are based on the map exited
	// Only certain maps are processed

	// The map number is checked for two things:
	//   do we need to process for this map?
	//   have we already processed this map?

	// If this is our first time exiting the map, we set a bit flag to prevent processing again
	// Then we do certain steps

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x00,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x02,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x05,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x04,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x03,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x1D,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x1C,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x1E,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	// Do a code backup process? This happens while entering the rocket screen
	code_backup_algorithm(6, 0x1B, working_code, code_backup, &rng1, &rng2);

	// This happens as the rocket screen fades away
	process_map_exit(0x1B,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x07,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x08,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	// Do a code backup process? This happens as you enter the car
	code_backup_algorithm(0, 0x06, working_code, code_backup, &rng1, &rng2);

	// Then when the car leaves the screen and the screen fades away
	process_working_code(0x06, working_code, code_backup, &rng1, &rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x09,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x0C,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x20,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x21,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x22,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	// Do a code backup process? This happens as you enter the first machine part
	code_backup_algorithm(4, 0x22, working_code, code_backup, &rng1, &rng2);

	// This happens when going down after walking past the ufo pulled down by the magnet, heading to the last map of the world
	process_working_code(0x22, working_code, code_backup, &rng1, &rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x23,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x24,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x25,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x26,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x0E,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x0F,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x10,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x12,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);

	process_map_exit(0x11,working_code,code_backup,&rng1,&rng2);

	//display_working_code(working_code);
	//display_code_backup(code_backup);
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
	
	//display_working_code(working_code);
}
