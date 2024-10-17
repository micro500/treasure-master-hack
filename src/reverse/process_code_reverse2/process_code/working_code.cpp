#include <vector>
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

working_code::working_code(working_code * original)
{
	this->rng.seed(original->rng.rng1,original->rng.rng2);

	for (int i = 0; i < 8; i++)
	{
		this->starting_value[i] = original->starting_value[i];
	}

	for (int i = 0; i < 0x80; i++)
	{
		this->working_code_data.as_uint8[i] = original->working_code_data.as_uint8[i];
		this->trust_mask.as_uint8[i] = original->trust_mask.as_uint8[i];
	}
}

void working_code::copy(working_code * original)
{
	this->rng.seed(original->rng.rng1,original->rng.rng2);

	/*
	for (int i = 0; i < 8; i++)
	{
		this->starting_value[i] = original->starting_value[i];
	}
	*/

	for (int i = 0; i < 0x80; i++)
	{
		this->working_code_data.as_uint8[i] = original->working_code_data.as_uint8[i];
		this->trust_mask.as_uint8[i] = original->trust_mask.as_uint8[i];
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

void working_code::inverse_working_code_algorithm(uint8 algorithm_number, int &rng_pos, rng_info rng_bits[], int &bits_used, uint8 reverse_rng_table[])
{
	if (algorithm_number == 0x00)
	{
		for (int i = 0; i < 0x80; i++)
		{
			// Save the lowest bit if we trust it

			if ((trust_mask.as_uint8[i] & 0x01) == 0x01)
			{
				rng_bits[bits_used].byte_pos = rng_pos;
				rng_bits[bits_used].bit_pos = 7;
				rng_bits[bits_used].bit = working_code_data.as_uint8[i] & 0x01;
				bits_used++;
			}
			rng_pos++;

			// Shift right, drop the lowest bit, gain an unknown bit 
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] >> 1;
			trust_mask.as_uint8[i] = trust_mask.as_uint8[i] >> 1;			
		}
	}
	else if (algorithm_number == 0x01)
	{
		for (int i = 0; i < 0x80; i++)
		{
			// TODO: Run RNG Back
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] - reverse_rng_table[rng_pos++];

			// If we think of the subtraction as a twos-complement addition the same rules as 4 apply:
			// The first untrusted bit propogates through the whole byte
			//   ex: 11101111 becomes 00001111
			// Formula: x & ((x^0xff)-1)
			trust_mask.as_uint8[i] = trust_mask.as_uint8[i] & ((trust_mask.as_uint8[i]^0xff) - 1);
		}
	}
	else if (algorithm_number == 0x02)
	{
		// Gain an untrusted bit
		unsigned char wc_carry = 0;
		unsigned char tm_carry = 0;

		for (int i = 0; i < 0x80; i+=2)
		{
			// Save bit 0 of byte 1 as the new carry
			unsigned char next_wc_carry = working_code_data.as_uint8[i+1] & 0x01;
			unsigned char next_tm_carry = trust_mask.as_uint8[i+1] & 0x01;

			// Shift byte 1 right
			// Set byte 1, bit 7 as byte 0, bit 7
			working_code_data.as_uint8[i+1] = (working_code_data.as_uint8[i+1] >> 1) | (working_code_data.as_uint8[i] & 0x80);
			trust_mask.as_uint8[i+1] = (trust_mask.as_uint8[i+1] >> 1) | (trust_mask.as_uint8[i] & 0x80);

			// Shift byte 0 left
			// Set byte 0, bit 0 as old carry
			working_code_data.as_uint8[i] = (working_code_data.as_uint8[i] << 1) | wc_carry;
			trust_mask.as_uint8[i] = (trust_mask.as_uint8[i] << 1) | wc_carry;

			wc_carry = next_wc_carry;
			tm_carry = next_tm_carry;
		}

		// Remaining carry is from the RNG 
		// Check if we trust it
		if (tm_carry == 0x01)
		{
			rng_bits[bits_used].byte_pos = rng_pos;
			rng_bits[bits_used].bit_pos = 7;
			rng_bits[bits_used].bit = wc_carry & 0x01;
			bits_used++;
		}
		rng_pos++;
	}
	else if (algorithm_number == 0x03)
	{
		for (int i = 0; i < 0x80; i++)
		{
			// TODO: Run RNG Back
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] ^ reverse_rng_table[rng_pos++];

			// No change to the trust mask
		}
	}
	else if (algorithm_number == 0x04)
	{
		for (int i = 0; i < 0x80; i++)
		{
			// The forward algorithm was just a twos-complement subtraction. Reverse is addition
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] + reverse_rng_table[rng_pos++];

			// The first untrusted bit propogates through the whole byte
			//   ex: 11101111 becomes 00001111
			// Formula: x & ((x^0xff)-1)
			trust_mask.as_uint8[i] = trust_mask.as_uint8[i] & ((trust_mask.as_uint8[i]^0xff) - 1);
		}
	}
	else if (algorithm_number == 0x05)
	{
		// Gain an untrusted bit
		unsigned char wc_carry = 0;
		unsigned char tm_carry = 0;

		for (int i = 0; i < 0x80; i+=2)
		{
			// Save bit 7 of byte 1 as the new carry
			unsigned char next_wc_carry = working_code_data.as_uint8[i+1] & 0x80;
			unsigned char next_tm_carry = trust_mask.as_uint8[i+1] & 0x80;

			// Shift byte 1 left
			// Set byte 1, bit 0 as byte 0, bit 0
			working_code_data.as_uint8[i+1] = (working_code_data.as_uint8[i+1] << 1) | (working_code_data.as_uint8[i] & 0x01);
			trust_mask.as_uint8[i+1] = (trust_mask.as_uint8[i+1] << 1) | (trust_mask.as_uint8[i] & 0x01);

			// Shift byte 0 right
			// Set byte 0, bit 7 as old carry
			working_code_data.as_uint8[i] = (working_code_data.as_uint8[i] >> 1) | wc_carry;
			trust_mask.as_uint8[i] = (trust_mask.as_uint8[i] >> 1) | tm_carry;

			wc_carry = next_wc_carry;
			tm_carry = next_tm_carry;
		}

		// Remaining carry is from the RNG 
		// Check if we trust it
		if (tm_carry == 0x01)
		{
			rng_bits[bits_used].byte_pos = rng_pos;
			rng_bits[bits_used].bit_pos = 7;
			rng_bits[bits_used].bit = (wc_carry >> 7) & 0x01;
			bits_used++;
		}
		rng_pos++;
	}
	else if (algorithm_number == 0x06)
	{
		for (int i = 0x7F; i >= 0; i--)
		{
			// Save the lowest bit if we trust it
			if ((trust_mask.as_uint8[i] & 0x80) == 0x80)
			{
				rng_bits[bits_used].byte_pos = rng_pos;
				rng_bits[bits_used].bit_pos = 7;
				rng_bits[bits_used].bit = (working_code_data.as_uint8[i] & 0x80) >> 7;
				bits_used++;
			}
			rng_pos++;

			// Shift left, drop the highest bit, gain an unknown bit 
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] << 1;
			trust_mask.as_uint8[i] = trust_mask.as_uint8[i] << 1;			
		}
	}
	else if (algorithm_number == 0x07)
	{
		for (int i = 0x7F; i >= 0; i--)
		{
			working_code_data.as_uint8[i] = working_code_data.as_uint8[i] ^ 0xFF;

			// No change to the trust mask
		}
	}
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