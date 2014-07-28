#ifndef CODE_H
#define CODE_H
#include "data_sizes.h"


//#define ORIGINAL


class Code
{
public:
        Code(uint8 value[8]);

        uint8 starting_value[8];
#ifdef ORIGINAL
		union
		{
			uint8 as_uint8[128];
			uint16 as_uint16[64];
			uint32 as_uint32[32];
			uint64 as_uint64[16];
		} working_code;
#endif

#ifndef ORIGINAL

		__declspec(align(16)) union
        {
                uint8 as_uint8[256];
                uint16 as_aligned16[128];
                uint32 as_uint32[64];
                uint64 as_uint64[32];
        } xor_table;

#endif

        union
        {
                uint8 as_uint8[4];
                uint16 as_uint16[2];
                uint32 as_uint32;
        } code_backup;

		

		/*__declspec(align(16)) union{
			uint8 as_uint8[128];
			uint16 as_aligned16[64];
			uint32 as_uint32[32];
			uint64 as_uint64[16];
		} xor_table;*/

        // RNG values
        uint8 rng1;
        uint8 rng2;

        uint8 rng();
        uint8 rng_real();

        void process_map_exit(uint8 map_number);

        void process_working_code(uint8 map_number);

        void working_code_algorithm(uint8 algorithm_number, uint8 map_number);

        void code_backup_algorithm(uint8 algorithm_number, uint8 map_number);
        void code_backup_algorithm_0(uint8 map_number);
        void code_backup_algorithm_1(uint8 map_number);
        void code_backup_algorithm_2(uint8 map_number);
        void code_backup_algorithm_3(uint8 map_number);
        void code_backup_algorithm_4(uint8 map_number);
        void code_backup_algorithm_5(uint8 map_number);
        void code_backup_algorithm_6(uint8 map_number);
        void code_backup_algorithm_7(uint8 map_number);


		void Code::permute_key_0(char level);
		void Code::permute_key_1(char level);
		void Code::permute_key_2(char level);
		void Code::permute_key_3(char level);
		void Code::permute_key_4(char level);
		void Code::permute_key_5(char level);
		void Code::permute_key_6(char level);
		void Code::permute_key_7(char level);

		void Code::permute_xor_table();
		void Code::expand_key(uint8 *iv);

		void Code::key_shedule(char level, char idx);
		void Code::permute_round(char idx);

        void display_working_code();

        bool operator==(const Code &other) const;
        bool operator<(const Code &other) const;
};

#endif //CODE_H