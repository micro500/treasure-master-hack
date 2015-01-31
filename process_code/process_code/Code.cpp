#include <stdio.h>
#include "data_sizes.h"
#include "Code.h"

extern uint16 rng_table[0x100][0x100];

Code::Code(uint8 value[8])
{
	// Copy the starting value in
	for (int i = 0; i < 8; i++)
	{
		working_code.as_uint8[i] = starting_value[i] = value[i];
	}

	// Seed the RNG
	rng1 = working_code.as_uint8[0];
	rng2 = working_code.as_uint8[1];

	// Expand the code to 128 bytes
	for (int i = 8; i < 0x80; i++)
	{
		working_code.as_uint8[i] = working_code.as_uint8[i-8] + rng();
	}

	code_backup.as_uint8[0] = working_code.as_uint8[0];
	code_backup.as_uint8[1] = working_code.as_uint8[1];
	code_backup.as_uint8[2] = working_code.as_uint8[2];
	code_backup.as_uint8[3] = working_code.as_uint8[3];
}

void Code::display_working_code()
{
	for (int i = 127; i >= 0; i--)
	{
		printf("%02X",working_code.as_uint8[i]);
		if (i % 16 == 0)
		{
			//printf("\n");
		}
	}

	printf("\n");
}

void Code::process_map_exit(uint8 map_number)
{
	// In three instances do something special, otherwise do the same steps
	if (map_number == 0x1B)
	{
		code_backup_algorithm(6, 0x1B);
	}

	if (map_number == 0x06)
	{
		code_backup_algorithm(0, 0x06);

		// Then when the car leaves the screen and the screen fades away
		process_working_code(0x06);
	}
	else
	{
		// Pick an algorithm number to process the code backup
		// The steps are the following:
		//   Take the top 2 bits of the map number (shift it down 4 times, AND with 0x03)
		//   Use that as an index in the code backup
		//   Take that value, shift it down twice, and take only 3 bits
		//   That is the alg number

		unsigned char algorithm_number = (code_backup.as_uint8[(map_number >> 4) & 0x03] >> 2) & 0x07;

		code_backup_algorithm(algorithm_number, map_number);

		process_working_code(map_number);
	}

	if (map_number == 0x22)
	{
		// Do a code backup process? This happens as you enter the first machine part
		code_backup_algorithm(4, 0x22);

		// This happens when going down after walking past the ufo pulled down by the magnet, heading to the last map of the world
		process_working_code(0x22);
	}
}

void Code::process_working_code(uint8 map_number)
{

	// The RNG is backed up
	// The RNG is seeded with the first two bytes of the code backup
	rng1 = code_backup.as_uint8[0];
	rng2 = code_backup.as_uint8[1];
	// Set up the nibble selection variables
	short nibble_selector = (code_backup.as_uint8[3] << 8) + code_backup.as_uint8[2];
	//printf("rng1: %02x, rng2: %02X, nibble: %04X\n",rng1,rng2,nibble_selector);
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
		unsigned char current_byte = working_code.as_uint8[i];
		if (nibble == 1)
		{
			current_byte = current_byte >> 4;
		}

		// Mask off only 3 bits
		unsigned char algorithm_number = (current_byte >> 1) & 0x07;
		
		// Run the selected algorithm
		working_code_algorithm(algorithm_number, map_number);
	}

	// The working code is then put back into the PPU
	// Somehow the game decides which map to go to next, and our work here is done
}

void Code::working_code_algorithm(uint8 algorithm_number, uint8 map_number)
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
			working_code.as_uint8[i] = (working_code.as_uint8[i] << 1) | ((rng() >> 7) & 0x01);

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
			working_code.as_uint8[i] = working_code.as_uint8[i] + rng();

			// DEX
			// BPL $17735
		}
	}
	else if (algorithm_number == 0x02)
	{
		// JSR $F1DA
		// ASL A
		unsigned char carry = (rng() >> 7) & 0x01;
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i-=2)
		{
			// This gets ugly. Just a warning...
			unsigned char next_carry = working_code.as_uint8[i-1] & 0x01;
			// ROL $0200,X
			// DEX
			// ROR $0200,X
			// DEX
			working_code.as_uint8[i-1] = (working_code.as_uint8[i-1] >> 1) | (working_code.as_uint8[i] & 0x80);
			working_code.as_uint8[i] = (working_code.as_uint8[i] << 1) | (carry & 0x01);

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
			working_code.as_uint8[i] = working_code.as_uint8[i] ^ rng();
			
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
			working_code.as_uint8[i] = working_code.as_uint8[i] + (rng()^0xFF) + 1;

			// DEX
			// BPL $17765
		}
	}
	else if (algorithm_number == 0x05)
	{
		// JSR $F1DA
		// ASL A
		unsigned char carry = rng() & 0x80;
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i-=2)
		{
			// This gets ugly. Just a warning...
			unsigned char next_carry = working_code.as_uint8[i-1] & 0x80;
			// ROR $0200,X
			// DEX
			// ROL $0200,X
			// DEX
			
			working_code.as_uint8[i-1] = (working_code.as_uint8[i-1] << 1) | (working_code.as_uint8[i] & 0x01);
			working_code.as_uint8[i] = (working_code.as_uint8[i] >> 1) | carry;

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
			working_code.as_uint8[i] = (working_code.as_uint8[i] >> 1) | (rng() & 0x80);

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

			working_code.as_uint8[i] = working_code.as_uint8[i] ^ 0xFF;
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

void Code::code_backup_algorithm(uint8 algorithm_number, uint8 map_number)
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

void Code::code_backup_algorithm_0(uint8 map_number)
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

void Code::code_backup_algorithm_1(uint8 map_number)
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

void Code::code_backup_algorithm_2(uint8 map_number)
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

void Code::code_backup_algorithm_3(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_2(map_number);
	//display_code_backup(code_backup);
	// Then alg 1
	code_backup_algorithm_1(map_number);
}

void Code::code_backup_algorithm_4(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_2(map_number);
	//display_code_backup(code_backup);
	// Then alg 1
	code_backup_algorithm_0(map_number);
}

void Code::code_backup_algorithm_5(uint8 map_number)
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

void Code::code_backup_algorithm_6(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_5(map_number);

	// Then alg 1
	code_backup_algorithm_1(map_number);
}

void Code::code_backup_algorithm_7(uint8 map_number)
{
	// Run alg 2 first
	code_backup_algorithm_5(map_number);

	// Then alg 1
	code_backup_algorithm_0(map_number);
}

uint8 Code::rng()
{
	uint16 result = rng_table[rng1][rng2];
	rng1 = (result >> 8) & 0xFF;
	rng2 = result & 0xFF;

	return rng1 ^ rng2;
}

uint8 Code::rng_real()
{
	//int rngA = *rng1;
	//int rngB = *rng2;	


	// LDA $0436
    // CLC
	unsigned char carry = 0;
    // ADC $0437
    // STA $0437
	// Basically, add rng1 and rng2 together w/o carry and store it to rng2
	rng2 = (unsigned char)(rng2 + rng1);

	// LDA #$89
    // CLC
    // ADC $0436
	// STA $0436
	// Basically, add #$89 to rng1, and remember the carry for the next addition
	rng1 = rng1 + 0x89;
	if (rng1 > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rng1 = (unsigned char)rng1;

    // LDA #$2A
    // ADC $0437 = #$AE
    // STA $0437 = #$AE
	rng2 = rng2 + 0x2A + carry;
	if (rng2 > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rng2 = (unsigned char)rng2;

    // LDA $0436 = #$71
    // ADC #$21
    // STA $0436 = #$71
	rng1 = rng1 + 0x21 + carry;
	if (rng1 > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rng1 = (unsigned char)rng1;

    // LDA $0437 = #$AE
    // ADC #$43
    // STA $0437 = #$AE
	rng2 = rng2 + 0x43 + carry;
	if (rng2 > 0xFF)
	{
		carry = 1;
	}
	else
	{
		carry = 0;
	}
	rng2 = (unsigned char)rng2;

    // EOR $0436 = #$71

	return rng1 ^ rng2;
}

bool Code::operator==(const Code &other) const
{
	for (int i = 0; i < 16; i++)
	{
		if (working_code.as_uint64[i] != other.working_code.as_uint64[i])
		{
			return false;
		}
	}
	return true;
}

bool Code::operator<(const Code &other) const
{
	for (int i = 0; i < 16; i++)
	{
		if (working_code.as_uint64[i] != other.working_code.as_uint64[i])
		{
			return (working_code.as_uint64[i] < other.working_code.as_uint64[i]);		
		}
	}
	return false;
}