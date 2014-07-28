#include <stdio.h>
//#include <chrono>
//#include <boost/chrono.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <time.h>

#include "data_sizes.h"
#include "Code.h"
#include "MurmurHash3.h"


uint16 rng_table[0x100][0x100];
char hash_tables[0x20000000];
const uint32_t myseed=2822038001;
char rand_seed[2];



//Original memory block to trigger Prize World?
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

//Block to trigger Prize World 2?
unsigned char other_world_code[0x53] = { 0x50, 0xF1, 0xFB, 0x44, 0xBD, 0xC1, 0xB1, 0x5E, 0xE4, 0x18, 
                                                                                 0x03, 0x52, 0x1A, 0x1C, 0x93, 0x36, 0x6E, 0x2D, 0x2B, 0x2B, 
                                                                                 0xB9, 0x5A, 0xA1, 0x58, 0x7B, 0x32, 0xDB, 0x9A, 0xA3, 0x49, 
                                                                                 0x40, 0x12, 0x06, 0x9C, 0xBB, 0x49, 0xAE, 0xB3, 0xFF, 0x67, 
                                                                                 0xF0, 0xD2, 0x8F, 0x6E, 0x45, 0xB7, 0xE5, 0x9A, 0x80, 0xAB, 
                                                                                 0xFF, 0xD3, 0x98, 0x9A, 0x94, 0x0A, 0x72, 0x81, 0xCF, 0x0A, 
                                                                                 0xFF, 0xFB, 0x54, 0xD9, 0x0C, 0xE3, 0x22, 0xF1, 0xE2, 0xD2, 
                                                                                 0xF4, 0xC7, 0x86, 0x81, 0x90, 0x0B, 0x04, 0xD2, 0x44, 0x66, 
                                                                                 0xC1, 0x68, 0xCA };

unsigned char opcode_table[]={
	1,2,0,0,0,2,2,0,1,2,1,0,0,3,3,0,2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,3,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0,2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0,2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,0,2,0,0,2,2,2,0,1,0,1,0,3,3,3,0,2,2,0,0,2,2,2,0,1,3,1,0,0,3,0,0,2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,2,2,0,0,2,2,2,0,1,3,1,0,3,3,3,0,2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0};


void flush_tables(){

	int i=0;
	int j=0;

	for(j=0;j<1;j++)
		for(i=0;i<0x20000000;i++)
			hash_tables[i]=0;

}

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

bool check_other_code(uint8 * working_code)
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
				//printf("GOOD!\n");
                return true;
        }
		return false;
}

bool check_other_code(uint16 * working_code)
{
	unsigned char decrypted_code[0x53];
	unsigned short sum = 0;
	for (int i = 0; i < 0x53; i++)
	{
		decrypted_code[i] = other_world_code[i] ^ working_code[127 - i];
		if (i < 0x53 - 2)
		{
			sum += decrypted_code[i];
		}
	}

	if ((sum >> 8) == decrypted_code[0x52] && (sum & 0xFF) == decrypted_code[0x51])
	{
		//printf("GOOD!\n");
		return true;
	}
	return false;
}

unsigned char * get_decrypted_code(uint8 * working_code){
	unsigned char decrypted_code[0x53];
	for (int i = 0; i < 0x53; i++)
		decrypted_code[i] = other_world_code[i] ^ working_code[127 - i];
	return decrypted_code;
}

unsigned char * get_decrypted_code(uint16 * working_code){
	unsigned char decrypted_code[0x53];
	for (int i = 0; i < 0x53; i++)
		decrypted_code[i] = other_world_code[i] ^ (working_code[127 - i] & 0xFF);
	return decrypted_code;
}

void dump(unsigned char *buf, unsigned char *key, char size, int iter) {
	FILE *o = NULL;
	char n[64];
	sprintf(n, "Results/%d-%02X%02X%02X%02X%02X%02X%02X%02X.bin",iter,  key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
	o=fopen(n, "wb");
	fwrite(buf, 1, size, o);
	fclose(o);
}

bool checkHash(uint8 *code, int map_number){

	uint32_t *hash = (uint32_t*)malloc(sizeof(uint32_t*));
	int index;
	char bit;
	char val;
	MurmurHash3_x86_32(code,128,myseed,hash);
	index = hash[0] >> 3;
	bit = hash[0] & 0x07;

	val = hash_tables[index];

	//printf("Index: %x val: %x bit: %d\n bit: ",index, val, hash_tables[index] | (0x01<<bit));

	if(((val >> bit) & 0x01) == 0)
	{
		hash_tables[index]= hash_tables[index] | (0x01<<bit);
		//printf("%X\n",index);
		return 0;
	}

	return 1;
}

bool checkHash(uint16 *code, int map_number){

	uint32_t *hash = (uint32_t*)malloc(sizeof(uint32_t*));

	int i = 0;
	uint8 newcode[128];
	for (i = 0; i < 0x80; i++)
		newcode[i] = code[i];
	int index;
	char bit;
	char val;
	MurmurHash3_x86_32(newcode, 128, myseed, hash);
	index = hash[0] >> 3;
	bit = hash[0] & 0x07;

	val = hash_tables[index];

	//printf("Index: %x val: %x bit: %d\n bit: ",index, val, hash_tables[index] | (0x01<<bit));

	if (((val >> bit) & 0x01) == 0)
	{
		hash_tables[index] = hash_tables[index] | (0x01 << bit);
		//printf("%X\n",index);
		return 0;
	}

	return 1;
}


#ifndef ORIGINAL
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef u16 uxx;
#define MAX_RANDS	2048

void rnd_calc() {	\
	register int seed;	\
	rand_seed[1] += rand_seed[0];	\
	seed = *(u16*)&rand_seed[0] + 0x2A89;	\
	*(u16*)&rand_seed[0] = seed & 0xFFFF;	\
	seed = *(u16*)&rand_seed[0] + 0x4321 + ((seed >> 16) & 1);	\
	*(u16*)&rand_seed[0] = seed & 0xFFFF;	\
}


uxx rands[65536][MAX_RANDS];
u32 rands_cur_seed, rands_cur_pos;






//Generate table of random values
void gen_rands() {
	int i, j;
	for (i = 0; i < 65536; i++) {
		*(u16*)&rand_seed[0] = i & 0xffff;
		for (j = 0; j < MAX_RANDS; j += 4) {
			rnd_calc();
			rands[i][MAX_RANDS - 1 - j - 0] = (rand_seed[1] ^ rand_seed[0]) & 0xFF;
			rnd_calc();
			rands[i][MAX_RANDS - 1 - j - 1] = (rand_seed[1] ^ rand_seed[0]) & 0xFF;
			rnd_calc();
			rands[i][MAX_RANDS - 1 - j - 2] = (rand_seed[1] ^ rand_seed[0]) & 0xFF;
			rnd_calc();
			rands[i][MAX_RANDS - 1 - j - 3] = (rand_seed[1] ^ rand_seed[0]) & 0xFF;
		}
	}
}
#endif

int main(void)
{
        time_t myTimer=time(NULL);
       
#ifndef ORIGINAL
		gen_rands();
#else
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
#endif

		time_t newTimer = time(NULL);
		double seconds = difftime(myTimer,newTimer);
		printf("Finished with RNG table in %.f\n",seconds);
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

		std::vector<Code> code_list(1, value);
		//std::vector<Code> successes(1,value);
		flush_tables();
		printf("Starting initialization\n");

		for (int k=0;k<0x100;k+=0x01){
			//std::vector<Code>().swap(code_list);
			
			code_list.clear();
			code_list.shrink_to_fit();
			code_list.reserve(20000000);
			printf("Starting block %d\n", k);
			
			time_t loop1 = time(NULL);
		//for (int j = 1; j<0x100;j++){
			//time_t loop2 = time(NULL);
        for (int i = 0; i < 0x1000000; i++) //originally 1000000
        {
                //value[4] = (i >> 24) & 0xFF;
				//value[5] = (i >> 16) & 0xFF;
				value[4] = ((i >> 24) & 0xFF )| k;
				value[5] = (i >> 16) & 0xFF;
                value[6] = (i >> 8) & 0xFF;
                value[7] = i & 0xFF;
#ifndef ORIGINAL
                code_list.emplace_back(value);
#else
				code_list.push_back(value);
#endif

				code_list.at(code_list.size()-1).process_map_exit(0x00);
#ifndef ORIGINAL
				if (checkHash(code_list.at(code_list.size() - 1).xor_table.as_aligned16, 0x00))
				{
					code_list.pop_back();
				}
#else
				if (checkHash(code_list.at(code_list.size() - 1).working_code.as_uint8, 0x00))
				{
					code_list.pop_back();
				}
#endif

				if(i%0x1000000==0){
					printf("%.f Since starting\n",difftime(loop1,time(NULL)));
					printf("%d\n",i/0x1000000);
					//code_list.shrink_to_fit();
				}
				
        }

		time_t now = time(NULL);
		seconds = difftime(loop1,now);
		printf("Finished initialization in %.f\n",seconds);
		//printf("Finished initialization.\n");

		

        int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11};

        //using boost::chrono::duration_cast;
        //using boost::chrono::microseconds;
        //typedef boost::chrono::high_resolution_clock clock;
        int blah = 0;
        //uint64 full_sum = 0;
		//time_t process = time(NULL);
        for (int i = 1; i < 26; i++)
        {
                // Step through the vector and do the map exit on each entry
                uint64 sum = 0;
				std::cout << code_list.size() << " running\n";

                for (std::vector<Code>::iterator it = code_list.begin(); it != code_list.end(); ++it)
                {
                        //auto start = clock::now();
                        it->process_map_exit(map_list[i]);
						
                        //auto end = clock::now();
                        //sum += duration_cast<microseconds>(end-start).count();
                }

                //std::cout << code_list.size() << " run\n";
                //std::cout << sum << "us\n";
                //full_sum += sum;
                
                blah++;

                size_t x = code_list.size();
                //auto start = clock::now();

				time_t presort=time(NULL);

                std::sort(code_list.begin(), code_list.end());
                code_list.erase(std::unique(code_list.begin(),code_list.end()),code_list.end());
				code_list.shrink_to_fit();

				time_t postsort=time(NULL);
				//printf("Finished Sort and Unique in %.f\n",difftime(postsort,presort));
				//printf("Finished Round in %.f\n",difftime(postsort,now));

                //auto end = clock::now();
                x -= code_list.size();
                //sum = duration_cast<microseconds>(end-start).count();
                //full_sum += sum;
                std::cout << x << " deleted\n";
                //std::cout << sum << " us\n";

				
                printf("\n");
        }

		newTimer = time(NULL);
		seconds = difftime(loop1,newTimer);
		printf("24-bit chunk processed in %.f\n",seconds);

        //std::cout << full_sum << "\n";

		int x = code_list.size();

		//printf("first_val: %d \n",code_list.at(1).working_code.as_uint8[4]);
		printf("%d codes remaining.\n",x);

		for(int m=code_list.size()-1;m>=0;m--){
#ifndef ORIGINAL
					if(!check_other_code(code_list.at(m).xor_table.as_aligned16))
						code_list.erase(code_list.begin() + m);
					else
					{
						int iter=0;
						unsigned char * decrypted = get_decrypted_code(code_list.at(m).xor_table.as_aligned16);
						bool success = true;
#else
			if (!check_other_code(code_list.at(m).working_code.as_uint8))
				code_list.erase(code_list.begin() + m);
			else
			{
				int iter = 0;
				unsigned char * decrypted = get_decrypted_code(code_list.at(m).working_code.as_uint8);
				bool success = true;
#endif
						while(iter<0x51){
							
									if(opcode_table[decrypted[iter]]==0){
										//code_list.erase(code_list.begin() + m);
										success=false;
										break;
									}
									else if((decrypted[iter]==0x20 || decrypted[iter]==0x4C) || decrypted[iter]==0x6C)
									{
										iter+=opcode_table[decrypted[iter]];
										break;
									}
									else
										iter+=opcode_table[decrypted[iter]];
								}
								//dump(decoded_handler, k, handler_size, ent);

								if(success){
									unsigned char * k = code_list.at(m).starting_value;
									printf("  key: %02X %02X %02X %02X %02X %02X %02X %02X valid up to %d bytes \n",k[0], k[1], k[2], k[3], k[4], k[5], k[6], k[7], iter);
									dump(decrypted, k, 0x53, iter);
								}
					}
				}

				//x -= code_list.size();
				//std::cout << x << " deleted from checksum mismatch and machine code\n";

        
		//size_t x = code_list.size();
        
        std::cout << code_list.size() << " Successful \n";
		//}
		//newTimer = time(NULL);
		//seconds = difftime(loop1,newTimer);
		//printf("3-byte chunk finished in %.f\n",seconds);
		}

		time_t current = time(NULL);
		seconds = difftime(newTimer,current);
		printf("4-byte chunk finished in %.f\n",seconds);
		system("pause");
		//}
        //return 0;

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
        
        //using boost::chrono::duration_cast;
        //using boost::chrono::microseconds;
        //typedef boost::chrono::high_resolution_clock clock;

        //auto sum = 0;
        //auto full_sum = 0;

        /*for (int i = 0; i < 0xFFFF; i++)
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
        
        display_working_code(working_code);*/
}