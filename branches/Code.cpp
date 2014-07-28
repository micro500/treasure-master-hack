#include <stdio.h>
#include "data_sizes.h"
#include "Code.h"
#include <emmintrin.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef u16 uxx;


u8 decoded_handler[128];

#define MAX_RANDS	2048

extern uxx rng_table[0x100][0x100];
#ifndef ORIGINAL
extern uxx rands[0x10000][2048];
extern u32 rands_cur_seed;
extern u32 rands_cur_pos;


#define rnd() (rands[rands_cur_seed][rands_cur_pos--])
#define rndd(a) (*(u32*)&rands[rands_cur_seed][rands_cur_pos - (a)])
#define rndq(a) (*(u64*)&rands[rands_cur_seed][rands_cur_pos - (a)])
#define rndx(a) (*(__m128i*)&rands[rands_cur_seed][rands_cur_pos - (a)])
#define srnd(seed) { rands_cur_pos = MAX_RANDS - 1; rands_cur_seed = (seed); }
#endif

Code::Code(uint8 value[8])
{
        // Copy the starting value in
        for (int i = 0; i < 8; i++)
        {
#ifdef ORIGINAL
			working_code.as_uint8[i] = starting_value[i] = value[i];
#else
			starting_value[i] = value[i];
#endif
        }

        
#ifndef ORIGINAL
		expand_key(value);
#else
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
#endif

		
}

//void Code::display_working_code()
//{
//        for (int i = 127; i >= 0; i--)
//        {
//                printf("%02X",working_code.as_uint8[i]);
//                if (i % 16 == 0)
//                {
//                        //printf("\n");
//                }
//        }
//
//        printf("\n");
//}

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

#ifndef ORIGINAL
	permute_xor_table();
#else
        // The RNG is backed up
        // The RNG is seeded with the first two bytes of the code backup
        rng1 = code_backup.as_uint8[0];
        rng2 = code_backup.as_uint8[1];
        // Set up the nibble selection variables
        short nibble_selector = (code_backup.as_uint8[3] << 8) + code_backup.as_uint8[2];
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

         //The working code is then put back into the PPU
         //Somehow the game decides which map to go to next, and our work here is done
#endif
}

#ifdef ORIGINAL
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
						//Swap in highest rng bit to lowest bit of current byte
						//Not perfectly invertible; top bit is unknown
					    //Can quickly check if possible via RNG, though.
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
						// Add rng value directly to current byte
					    // Invertible, somewhat depends on alg 0
                        working_code.as_uint8[i] = working_code.as_uint8[i] + rng();

                        // DEX
                        // BPL $17735
                }
        }
        else if (algorithm_number == 0x02)
        {
                // JSR $F1DA
                // ASL A
				//take highest RNG bit...
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
						//Not Invertible, spreads some effects from alg 0. Use mask to track?
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
						//XOR directly with RNG
						//Invertible
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
						//Two's complement with RNG
					    //Invertible
                        working_code.as_uint8[i] = working_code.as_uint8[i] + (rng()^0xFF) + 1;

                        // DEX
                        // BPL $17765
                }
        }
        else if (algorithm_number == 0x05)
        {
                // JSR $F1DA
                // ASL A
				//take high RNG bit...
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
                        //Not Invertible, but potential to change unknown mask
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
						//Place rng high bit on front of byte
						//Not Invertible; adds unknown pieces
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

						//One's complement, no RNG
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
#endif

void Code::code_backup_algorithm(uint8 algorithm_number, uint8 map_number)
{
        //printf("code_backup_algorithm: %i\n", algorithm_number);
        if (algorithm_number == 0x00)
        {
#ifdef ORIGINAL
            code_backup_algorithm_0(map_number);
#else
			permute_key_0(map_number);
#endif
        }
        else if (algorithm_number == 0x01)
        {
#ifdef ORIGINAL
			code_backup_algorithm_1(map_number);
#else
			permute_key_1(map_number);
#endif
        }
        else if (algorithm_number == 0x02)
        {
#ifdef ORIGINAL
			code_backup_algorithm_2(map_number);
#else
			permute_key_2(map_number);
#endif
        }
        if (algorithm_number == 0x03)
        {
#ifdef ORIGINAL
			code_backup_algorithm_3(map_number);
#else
			permute_key_3(map_number);
#endif
        }
        else if (algorithm_number == 0x04)
        {
#ifdef ORIGINAL
			code_backup_algorithm_4(map_number);
#else
			permute_key_4(map_number);
#endif
        }
        else if (algorithm_number == 0x05)
        {
#ifdef ORIGINAL
			code_backup_algorithm_5(map_number);
#else
			permute_key_5(map_number);
#endif
        }
        else if (algorithm_number == 0x06)
        {
#ifdef ORIGINAL
			code_backup_algorithm_6(map_number);
#else
			permute_key_6(map_number);
#endif
        }
        else if (algorithm_number == 0x07)
        {
#ifdef ORIGINAL
			code_backup_algorithm_7(map_number);
#else
			permute_key_7(map_number);
#endif
        }
}

void Code::code_backup_algorithm_0(uint8 map_number)
{


		//XOR map number and second byte of backup
        unsigned char temp = map_number ^ code_backup.as_uint8[1];
		//Add the prior XOR and backup 0, right shift... basically check if there's a carry
        unsigned char carry = (((int)temp + code_backup.as_uint8[0]) >> 8) & 0x01;
		//SUM byte 0 from backup with XOR val
        temp = temp + code_backup.as_uint8[0];
		//Subtract byte 2 from temp + prior carry. If result underflows, give 0, otherwise 1
        unsigned char next_carry = (((int)temp - code_backup.as_uint8[2] - (1 - carry)) < 0 ? 0 : 1);
		//Subtract byte 2 from temp + first carry, then swap next_carry into carry
        temp = temp - code_backup.as_uint8[2] - (1 - carry);
        carry = next_carry;

        unsigned char rolling_sum = temp;


		//Sum all of the backup bytes while paying attention to carries. Replace current byte with new sum byte
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

		//Sum all of the backup bytes while paying attention to carries. Initial carry is the same. Replace current byte with new sum byte
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
		//Reverse order
        code_backup_algorithm_2(map_number);
        //display_code_backup(code_backup);
        // Then alg 1
		//Rolling sum, set carry
        code_backup_algorithm_1(map_number);
}

void Code::code_backup_algorithm_4(uint8 map_number)
{
        // Run alg 2 first
		//Reverse order
        code_backup_algorithm_2(map_number);
        //display_code_backup(code_backup);
        // Then alg 1
		//Rolling sum, calc carry
        code_backup_algorithm_0(map_number);
}

void Code::code_backup_algorithm_5(uint8 map_number)
{
        // Do an ASL on the map number
        unsigned char temp = map_number << 1;

        // EOR #$FF
		//Take map number, mult by 2, add to byte 0
		//Subtract map number
		//Is this just byte0 + map num?
        temp = temp ^ 0xFF;
        temp = temp + code_backup.as_uint8[0];
        temp = temp - map_number;
        code_backup.as_uint8[0] = temp;
        

        unsigned char temp2[4];
        temp2[1] = code_backup.as_uint8[1];
        temp2[2] = code_backup.as_uint8[2];
        temp2[3] = code_backup.as_uint8[3];

		//re-order remaining bytes
        code_backup.as_uint8[1] = temp2[3];
        code_backup.as_uint8[2] = temp2[1];
        code_backup.as_uint8[3] = temp2[2];
}

void Code::code_backup_algorithm_6(uint8 map_number)
{
        // Run alg 2 first
		// Mix map number and reorder
        code_backup_algorithm_5(map_number);

        // Then alg 1
		//rolling sum with set carry
        code_backup_algorithm_1(map_number);
}

void Code::code_backup_algorithm_7(uint8 map_number)
{
        // Run alg 2 first
		//Mix map number and reorder
        code_backup_algorithm_5(map_number);

        // Then alg 1
		//rolling sum with no set carry
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
        
#ifndef ORIGINAL
	for (int i = 0; i < 32; i++)
	{
                if (xor_table.as_uint64[i] != other.xor_table.as_uint64[i])
                {
                        return false;
                }
#else
	for (int i = 0; i < 16; i++)
	{
			if (working_code.as_uint64[i] != other.working_code.as_uint64[i])
			{
				return false;
			}
#endif
        }
        return true;
}

bool Code::operator<(const Code &other) const
{
#ifndef ORIGINAL
        for (int i = 0; i < 32; i++)
        {
			if (xor_table.as_uint64[i] != other.xor_table.as_uint64[i])
			{
				return (xor_table.as_uint64[i] < other.xor_table.as_uint64[i]);
			}
#else
	for (int i = 0; i < 16; i++)
	{

            if (working_code.as_uint64[i] != other.working_code.as_uint64[i])
            {
				return (working_code.as_uint64[i] < other.working_code.as_uint64[i]);
            }
#endif
        }
        return false;
}




//void save_rands() {
//	int i, j;
//	printf("uxx rands[65536][2048] = {\n");
//	for(i = 0; i < 65536; i++) {
//		printf("	{");
//		for(j = 0; j < MAX_RANDS; j++) {
//			if((j & 0x1F) == 0)
//				printf("\n");
//			printf("%02x,", rands[i][j]);
//		}
//		printf("\n	},\n");
//	}
//}

#ifndef ORIGINAL
void Code::permute_key_0(char level) {	\
register uxx tmp = (level ^ code_backup.as_uint8[1]) + code_backup.as_uint8[0];	\
tmp = ((tmp & 0xFF) - code_backup.as_uint8[2] - (((tmp >> 8) & 1) ^ 1)) ^ 0x100;	\
tmp = (tmp & 0xFF) + code_backup.as_uint8[3] + ((tmp >> 8) & 1);	\
	code_backup.as_uint8[3] = tmp & 0xFF;	\
	tmp = (tmp & 0xFF) + code_backup.as_uint8[2] + ((tmp >> 8) & 1);	\
	code_backup.as_uint8[2] = tmp & 0xFF;	\
	tmp = (tmp & 0xFF) + code_backup.as_uint8[1] + ((tmp >> 8) & 1);	\
	code_backup.as_uint8[1] = tmp & 0xFF;	\
	tmp = (tmp & 0xFF) + code_backup.as_uint8[0] + ((tmp >> 8) & 1);	\
	code_backup.as_uint8[0] = tmp & 0xFF;	\
}

void Code::permute_key_1(char level) {	\
	register uxx tmp = level + code_backup.as_uint8[3] + 1;	\
	code_backup.as_uint8[3] = tmp & 0xFF;	\
	tmp = (tmp & 0xFF) + code_backup.as_uint8[2] + ((tmp >> 8) & 1);	\
	code_backup.as_uint8[2] = tmp & 0xFF;	\
	tmp = (tmp & 0xFF) + code_backup.as_uint8[1] + ((tmp >> 8) & 1);	\
	code_backup.as_uint8[1] = tmp & 0xFF;	\
	tmp = (tmp & 0xFF) + code_backup.as_uint8[0] + ((tmp >> 8) & 1);	\
	code_backup.as_uint8[0] = tmp & 0xFF;	\
}

void Code::permute_key_2(char level) {
	register uxx tmp = (code_backup.as_uint8[0] + level) & 0xFF;	\
	code_backup.as_uint8[0] = code_backup.as_uint8[3];	\
	code_backup.as_uint8[3] = tmp;	\
	tmp = code_backup.as_uint8[1];	\
	code_backup.as_uint8[1] = code_backup.as_uint8[2];	\
	code_backup.as_uint8[2] = tmp;	\
}

void Code::permute_key_3(char level) {
	permute_key_2(level);	\
	permute_key_1(level);	\
}

void Code::permute_key_4(char level) {
	permute_key_2(level);	\
	permute_key_0(level);	\
}

void Code::permute_key_5(char level) {
	register uxx tmp = code_backup.as_uint8[1];	\
	code_backup.as_uint8[0] = ((((level << 1) ^ 0xFF) + code_backup.as_uint8[0]) - level) & 0xFF;	\
	code_backup.as_uint8[1] = code_backup.as_uint8[3];	\
	code_backup.as_uint8[3] = code_backup.as_uint8[2];	\
	code_backup.as_uint8[2] = tmp;	\
}

void Code::permute_key_6(char level) {
	permute_key_5(level);	\
	permute_key_1(level);	\
}

void Code::permute_key_7(char level) {
	permute_key_5(level);	\
	permute_key_0(level);	\
}

void Code::key_shedule(char level, char idx) {	\
	switch(idx) {	\
	case 0: permute_key_0(level); break;	\
	case 1: permute_key_1(level); break;	\
	case 2: permute_key_2(level); break;	\
	case 3: permute_key_3(level); break;	\
	case 4: permute_key_4(level); break;	\
	case 5: permute_key_5(level); break;	\
	case 6: permute_key_6(level); break;	\
	case 7: permute_key_7(level); break;	\
	}	\
}

#define rnddwswap64(a) ((((u64)(rndd(a) >> 16) | (rndd(a) << 16)) << 32) | ((rndd(a - 2) >> 16) | (rndd(a - 2) << 16)))

void Code::permute_round(char idx) {
	__declspec(align(16)) static u32 saturate_mask[4] = {0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF};
	__declspec(align(16)) static u32 and_mask[4] = {0x00010001, 0x00010001, 0x00010001, 0x00010001};
	switch(idx) {
	case 0: {
		register int i;
		__m128i smsk = _mm_loadu_si128((__m128i*)&saturate_mask[0]);
		for(i = 0x7F; i >= 0; i -= 8) {
			__m128i rndx = _mm_loadu_si128((__m128i*)&rands[rands_cur_seed][rands_cur_pos - 7]);
			__m128i tmp0 = _mm_slli_epi16(*(__m128i*)&xor_table.as_aligned16[i - 7], 1);
			__m128i tmp1 = _mm_srli_epi16(rndx, 7);
			tmp0 = _mm_or_si128(tmp0, tmp1);
			*(__m128i*)&xor_table.as_aligned16[i - 7] = _mm_and_si128(tmp0, smsk);
			rands_cur_pos -= 8;
		}
		break;
	}
	case 1: {
		register int i;
		for(i = 0x7F; i>= 0; i -= 8) {
			__m128i rndx = _mm_loadu_si128((__m128i*)&rands[rands_cur_seed][rands_cur_pos - 7]);
			*(__m128i*)&xor_table.as_aligned16[i - 7] = _mm_add_epi8(*(__m128i*)&xor_table.as_aligned16[i - 7], rndx);
			rands_cur_pos -= 8;
		}
		break;
	}
	case 2: {
		register int i;
		register uxx tmp = (rnd() >> 7) & 1;
		for(i = 0x7F; i >= 0; i -= 4) {
			tmp = (xor_table.as_aligned16[i - 0] << 1) | tmp;
			xor_table.as_aligned16[i - 0] = tmp & 0xFF;
			xor_table.as_aligned16[i - 1] |= (tmp & 0x100);
			tmp = xor_table.as_aligned16[i - 1] & 1;
			xor_table.as_aligned16[i - 1] >>= 1;
			tmp = (xor_table.as_aligned16[i - 2] << 1) | tmp;
			xor_table.as_aligned16[i - 2] = tmp & 0xFF;
			xor_table.as_aligned16[i - 3] |= (tmp & 0x100);
			tmp = xor_table.as_aligned16[i - 3] & 1;
			xor_table.as_aligned16[i - 3] >>= 1;
		}
		break;
	}
	case 3: {
		register int i;
		for(i = 0x7F; i >= 0; i -= 8) {
			__m128i rndx = _mm_loadu_si128((__m128i*)&rands[rands_cur_seed][rands_cur_pos - 7]);
			*(__m128i*)&xor_table.as_aligned16[i - 7] = _mm_xor_si128(*(__m128i*)&xor_table.as_aligned16[i - 7], rndx);
			rands_cur_pos -= 8;
		}
		break;
	}
	case 4: {
		register int i;
		__m128i smsk = _mm_loadu_si128((__m128i*)&saturate_mask[0]);
		__m128i amsk = _mm_loadu_si128((__m128i*)&and_mask[0]);
		for(i = 0x7F; i >= 0; i -= 8) {
			__m128i rndx = _mm_loadu_si128((__m128i*)&rands[rands_cur_seed][rands_cur_pos - 7]);
			__m128i tmp0; 
			rndx = _mm_xor_si128(rndx, smsk);
			tmp0 = _mm_add_epi8(*(__m128i*)&xor_table.as_aligned16[i - 7], rndx);
			*(__m128i*)&xor_table.as_aligned16[i - 7] = _mm_add_epi8(tmp0, amsk);
			rands_cur_pos -= 8;
		}
		break;
	}
	case 5: {
		register int i;
		register uxx tmp = rnd() << 1;
		for(i = 0x7F; i >= 0; i -= 4) {
			xor_table.as_aligned16[i - 0] |= (tmp & 0x100);
			tmp = xor_table.as_aligned16[i - 0] & 1;
			xor_table.as_aligned16[i - 0] >>= 1;
			tmp = (xor_table.as_aligned16[i - 1] << 1) | tmp;
			xor_table.as_aligned16[i - 1] = tmp & 0xFF;
			xor_table.as_aligned16[i - 2] |= (tmp & 0x100);
			tmp = xor_table.as_aligned16[i - 2] & 1;
			xor_table.as_aligned16[i - 2] >>= 1;
			tmp = (xor_table.as_aligned16[i - 3] << 1) | tmp;
			xor_table.as_aligned16[i - 3] = tmp & 0xFF;
		}
		break;
	}
	case 6: {
		register int i;
		for(i = 0; i < 0x80; i += 8) {
			*(u64*)&xor_table.as_aligned16[i + 0] = ((*(u64*)&xor_table.as_aligned16[i + 0] >> 1) | (rnddwswap64(3) & 0x0080008000800080)) & 0x00FF00FF00FF00FF;
			*(u64*)&xor_table.as_aligned16[i + 4] = ((*(u64*)&xor_table.as_aligned16[i + 4] >> 1) | (rnddwswap64(7) & 0x0080008000800080)) & 0x00FF00FF00FF00FF;
			rands_cur_pos -= 8;
		}
		break;
	}
	case 7: {
		register int i;
		__m128i smsk = _mm_loadu_si128((__m128i*)&saturate_mask[0]);
		for(i = 0; i < 0x80; i += 8) {
			*(__m128i*)&xor_table.as_aligned16[i] = _mm_xor_si128(*(__m128i*)&xor_table.as_aligned16[i], smsk);
		}
		break;
	}
	}
}

void Code::permute_xor_table() {
	u16 crc = (code_backup.as_uint8[2] & 0xFF) | ((code_backup.as_uint8[3] & 0xFF) << 8);
	srnd((code_backup.as_uint8[0] & 0xFF) | ((code_backup.as_uint8[1] & 0xFF) << 8));
	permute_round(((xor_table.as_aligned16[0] >> (((crc & 0x8000) >> 15) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[1] >> (((crc & 0x4000) >> 14) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[2] >> (((crc & 0x2000) >> 13) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[3] >> (((crc & 0x1000) >> 12) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[4] >> (((crc & 0x0800) >> 11) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[5] >> (((crc & 0x0400) >> 10) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[6] >> (((crc & 0x0200) >> 9) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[7] >> (((crc & 0x0100) >> 8) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[8] >> (((crc & 0x0080) >> 7) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[9] >> (((crc & 0x0040) >> 6) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[10] >> (((crc & 0x0020) >> 5) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[11] >> (((crc & 0x0010) >> 4) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[12] >> (((crc & 0x0008) >> 3) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[13] >> (((crc & 0x0004) >> 2) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[14] >> (((crc & 0x0002) >> 1) << 2)) & 0x0F) >> 1);
	permute_round(((xor_table.as_aligned16[15] >> (((crc & 0x0001) >> 0) << 2)) & 0x0F) >> 1);
}

#define permute(level) {	\
	key_shedule(level, (key[(level >> 4) & 3] >> 2) & 7);	\
	permute_xor_table();	\
}

void Code::expand_key(uint8 *iv) {
	int i;
	for (i = 0; i < 8; i++){
		xor_table.as_aligned16[i] = iv[i];
		//printf("Init Val: %d, in table: %d, 16-bit: %d\n", iv[i], xor_table.as_uint8[i], xor_table.as_aligned16[i/2]);
	}
	code_backup.as_uint8[0] = xor_table.as_aligned16[0];
	code_backup.as_uint8[1] = xor_table.as_aligned16[1];
	code_backup.as_uint8[2] = xor_table.as_aligned16[2];
	code_backup.as_uint8[3] = xor_table.as_aligned16[3];
	srnd((code_backup.as_uint8[0] & 0xFF) | ((code_backup.as_uint8[1] & 0xFF) << 8));
	for(i = 0; i < 0x78; i += 8) {
		*(u64*)&xor_table.as_aligned16[i + 8 + 0] = (*(u64*)&xor_table.as_aligned16[i + 0] + rnddwswap64(3)) & 0x00FF00FF00FF00FF;
		*(u64*)&xor_table.as_aligned16[i + 8 + 4] = (*(u64*)&xor_table.as_aligned16[i + 4] + rnddwswap64(7)) & 0x00FF00FF00FF00FF;
		rands_cur_pos -= 8;
	}
}

unsigned char level0_iv[] = {
	0x2C, 0xA5, 0xB4, 0x2D, 0xF7, 0x3A, 0x26, 0x12
};

void decode(u8 *enc, uxx *xor, u8 *dec, u8 size) {
	int i;
	for(i = 0; i < (size & 0xFC); i+=8) {
		register u64 tmp = xor[0x7f - i - 4] | (xor[0x7f - i - 5] << 8) | (xor[0x7f - i - 6] << 16) | ((u32)xor[0x7f - i - 7] << 24);
		tmp = (tmp << 32) | (xor[0x7f - i - 0] | (xor[0x7f - i - 1] << 8) | (xor[0x7f - i - 2] << 16) | ((u32)xor[0x7f - i - 3] << 24));
		*(u64*)&dec[i] = *(u64*)&enc[i] ^ tmp;
	}
	for(i = (size & 0xFC); i < size; i++) {
		dec[i] = enc[i] ^ xor[0x7f - i];
	}
}

int crccheck(u8 *buf, u8 size) {
	int i;
	u16 sum = 0;
	for(i = 0; i < (size & 0xFC); i+=4) {
		sum += buf[i + 0];
		sum += buf[i + 1];
		sum += buf[i + 2];
		sum += buf[i + 3];
	}
	for(i = (size & 0xFC); i < size; i++) {
		sum += buf[i];
	}
	return sum == (buf[size] | (buf[size + 1] << 8));
}
#endif