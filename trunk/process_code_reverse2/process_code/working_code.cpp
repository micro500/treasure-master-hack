#include <stdio.h>
#include "data_sizes.h"
#include "working_code.h"
#include "rng.h"
#include "key_schedule.h"

working_code::working_code(uint8 value[8])
{
	// Copy the starting value in, and back it up for later
	for (int i = 0; i < 8; i++)
	{
		working_code_data.as_uint8[i] = starting_value[i] = value[i];
	}

	// Seed the RNG
	rng.seed(working_code_data.as_uint8[0], working_code_data.as_uint8[1]);

	// Expand the code to 128 bytes
	for (int i = 8; i < 0x80; i++)
	{
		working_code_data.as_uint8[i] = working_code_data.as_uint8[i-8] + rng.run();
	}
}

void working_code::display_working_code()
{
	for (int i = 127; i >= 0; i--)
	{
		printf("%02X",working_code_data.as_uint8[i]);
		if (i % 16 == 0)
		{
			//printf("\n");
		}
	}

	printf("\n");
}

void working_code::process_map_exit(uint8 map_number, key_schedule_entry schedule_entry)
{
	process_working_code(map_number, schedule_entry);
}

void working_code::process_working_code(uint8 map_number, key_schedule_entry schedule_entry)
{
	// The RNG is backed up
	// The RNG is seeded with the first two bytes of the code backup
	rng.seed(schedule_entry.rng1, schedule_entry.rng2);
	
	// Set up the nibble selection variables
	short nibble_selector = schedule_entry.nibble_selector;
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
		unsigned char current_byte = working_code_data.as_uint8[i];
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

void working_code::working_code_algorithm(uint8 algorithm_number, uint8 map_number)
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
			working_code_data.as_uint8[i] = (working_code_data.as_uint8[i] << 1) | ((rng.run() >> 7) & 0x01);

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
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] + rng.run();

			// DEX
			// BPL $17735
		}
	}
	else if (algorithm_number == 0x02)
	{
		// JSR $F1DA
		// ASL A
		unsigned char carry = (rng.run() >> 7) & 0x01;
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i-=2)
		{
			// This gets ugly. Just a warning...
			unsigned char next_carry = working_code_data.as_uint8[i-1] & 0x01;
			// ROL $0200,X
			// DEX
			// ROR $0200,X
			// DEX
			working_code_data.as_uint8[i-1] = (working_code_data.as_uint8[i-1] >> 1) | (working_code_data.as_uint8[i] & 0x80);
			working_code_data.as_uint8[i] = (working_code_data.as_uint8[i] << 1) | (carry & 0x01);

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
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] ^ rng.run();
			
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
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] + (rng.run()^0xFF) + 1;

			// DEX
			// BPL $17765
		}
	}
	else if (algorithm_number == 0x05)
	{
		// JSR $F1DA
		// ASL A
		unsigned char carry = rng.run() & 0x80;
		// LDX #$7F
		for (int i = 0x7F; i >= 0; i-=2)
		{
			// This gets ugly. Just a warning...
			unsigned char next_carry = working_code_data.as_uint8[i-1] & 0x80;
			// ROR $0200,X
			// DEX
			// ROL $0200,X
			// DEX
			
			working_code_data.as_uint8[i-1] = (working_code_data.as_uint8[i-1] << 1) | (working_code_data.as_uint8[i] & 0x01);
			working_code_data.as_uint8[i] = (working_code_data.as_uint8[i] >> 1) | carry;

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
			working_code_data.as_uint8[i] = (working_code_data.as_uint8[i] >> 1) | (rng.run() & 0x80);

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

			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] ^ 0xFF;
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

bool working_code::operator==(const working_code &other) const
{
	for (int i = 0; i < 16; i++)
	{
		if (working_code_data.as_uint64[i] != other.working_code_data.as_uint64[i])
		{
			return false;
		}
	}
	return true;
}

bool working_code::operator<(const working_code &other) const
{
	for (int i = 0; i < 16; i++)
	{
		if (working_code_data.as_uint64[i] != other.working_code_data.as_uint64[i])
		{
			return (working_code_data.as_uint64[i] < other.working_code_data.as_uint64[i]);		
		}
	}
	return false;
}