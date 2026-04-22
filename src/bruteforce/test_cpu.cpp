#include "tm_base.h"
#include "tm_8.h"
#include "tm_mmx_m64_8.h"
#include "tm_32_8.h"
#include "tm_32_16.h"
#include "tm_64_8.h"
#include "tm_64_16.h"
#include "tm_sse2_m128_8.h"
#include "tm_sse2_m128s_8.h"
#include "tm_sse2_m128_16.h"
#include "tm_avx_r256_16.h"
#include "tm_avx_m256_8.h"
#include "tm_avx_r128s_8.h"
#include "tm_avx_r128s_map_8.h"
#include "tm_ssse3_r128_8.h"
#include "tm_ssse3_r128s_8.h"
#include "tm_ssse3_r128_map_8.h"
#include "tm_ssse3_r128s_map_8.h"
#include "tm_avx_r256_8.h"
#include "tm_avx_r256s_8.h"
#include "tm_avx_m256_16.h"
#include "tm_avx2_r256s_8.h"
#include "tm_avx512bw_r512_8.h"
#include "tm_avx512bw_r512s_8.h"
#include "rng_obj.h"
#include "tester2.h"
#include "tm_tester.h"
#include "key_schedule.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <direct.h>

// ---------------------------------------------------------------------------
// dump_test_vectors — prints C array literals for classes_test.cpp.
// Uncomment the call in main(), build and run, redirect stdout to a file,
// then paste the output over the FIXME placeholders in classes_test.cpp.
// ---------------------------------------------------------------------------
static void print_hex_array(const char* name, const uint8* data, int len)
{
	printf("static const uint8 %s[128] = {", name);
	for (int i = 0; i < len; i++) {
		if (i % 16 == 0) printf("\n    ");
		printf("0x%02X%s", data[i], i < len - 1 ? ", " : "");
	}
	printf("\n};\n");
}

static void dump_test_vectors(RNG& rng)
{
	printf("// ===== PASTE INTO classes_test.cpp (replace FIXME blocks) =====\n\n");

	// --- Alg vectors from TM_alg_test_cases4.txt ---
	// Row layout: [alg_id, seed_hi, seed_lo, input[128], seed_out_hi, seed_out_lo, expected[128]]
	FILE* alg_file = fopen("../../../tm/TM_alg_test_cases4.txt", "r");
	if (alg_file == NULL)
	{
		printf("// WARNING: could not open TM_alg_test_cases4.txt — alg vectors not dumped\n\n");
	}
	else
	{
		int count[8] = {};
		const int alg_row_len = 1 + 2 + 128 + 2 + 128; // 261
		uint8 row[261];
		for (int j = 0; j < 1326336; j++)
		{
			for (int i = 0; i < alg_row_len; i++) {
				int val; fscanf(alg_file, "%i,", &val); row[i] = (uint8)val;
			}
			int alg_id = row[0];
			if (alg_id < 8 && count[alg_id] < 2)
			{
				int v = count[alg_id];
				uint16 seed_in  = ((uint16)row[1] << 8) | row[2];
				uint16 seed_out = ((uint16)row[131] << 8) | row[132];
				uint8* input    = &row[3];
				uint8* expected = &row[133];

				char iname[32], oname[32];
				sprintf(iname, "alg%d_v%d_in",  alg_id, v);
				sprintf(oname, "alg%d_v%d_out", alg_id, v);
				print_hex_array(iname,  input,    128);
				print_hex_array(oname,  expected, 128);
				printf("static const uint16 alg%d_v%d_seed_in  = 0x%04X;\n", alg_id, v, seed_in);
				printf("static const uint16 alg%d_v%d_seed_out = 0x%04X;\n\n", alg_id, v, seed_out);
				count[alg_id]++;
			}
			bool done = true;
			for (int i = 0; i < 8; i++) if (count[i] < 2) { done = false; break; }
			if (done) break;
		}
		fclose(alg_file);
	}

	// --- Expansion vectors from TM_expansion_test_cases4.txt ---
	// Row layout: [key_bytes[4], data_bytes[4], expected[128]]
	FILE* exp_file = fopen("../../../tm/TM_expansion_test_cases4.txt", "r");
	if (exp_file == NULL)
	{
		printf("// WARNING: could not open TM_expansion_test_cases4.txt — expansion vectors not dumped\n\n");
	}
	else
	{
		tm_8 ref8(&rng);
		uint8 row[136], result[128];
		for (int v = 0; v < 2; v++)
		{
			for (int i = 0; i < 136; i++) {
				int val; fscanf(exp_file, "%i,", &val); row[i] = (uint8)val;
			}
			uint32 key  = ((uint32)row[0] << 24) | ((uint32)row[1] << 16) | ((uint32)row[2] << 8) | row[3];
			uint32 data = ((uint32)row[4] << 24) | ((uint32)row[5] << 16) | ((uint32)row[6] << 8) | row[7];
			char oname[32]; sprintf(oname, "expansion_v%d_out", v);
			print_hex_array(oname, &row[8], 128);
			printf("static const uint32 expansion_v%d_key  = 0x%08X;\n", v, key);
			printf("static const uint32 expansion_v%d_data = 0x%08X;\n\n", v, data);
			(void)result;
		}
		fclose(exp_file);
	}

	// --- Pipeline vectors (expand + ALL_MAPS), computed from tm_8 ---
	{
		tm_8 ref8(&rng);
		key_schedule sched(0x2CA5B42D, key_schedule::ALL_MAPS);
		uint32 pp_keys[2]  = { 0x2CA5B42D, 0x2CA5B42D };
		uint32 pp_data[2]  = { 0x0009BE9F, 0x0735B1D2 };
		uint8 result[128];
		for (int v = 0; v < 2; v++)
		{
			ref8.test_expand_and_map(pp_keys[v], pp_data[v], sched, result);
			char oname[32]; sprintf(oname, "pipeline_v%d_out", v);
			print_hex_array(oname, result, 128);
			printf("static const uint32 pipeline_v%d_key   = 0x%08X;\n", v, pp_keys[v]);
			printf("static const uint32 pipeline_v%d_data  = 0x%08X;\n", v, pp_data[v]);
			printf("static const uint32 pipeline_v%d_sched = 0x%08X;\n\n", v, pp_keys[v]);
		}
	}

	// --- Find an other-world valid pair from TM_full_test_cases4.txt ---
	// Row layout (hex): [key_bytes[4], data_bytes[4], result[128]]
	{
		tm_8 ref8(&rng);
		FILE* full_file = fopen("../../../tm/TM_full_test_cases.txt", "r");
		if (full_file == NULL)
		{
			printf("// WARNING: could not open TM_full_test_cases.txt — other-world pair not found\n\n");
		}
		else
		{
			const int row_len = 8 + 128; // 136
			uint8 row[136];
			bool found = false;
			while (!found)
			{
				int i;
				for (i = 0; i < row_len; i++) {
					int val;
					if (fscanf(full_file, "%x,", &val) != 1) break;
					row[i] = (uint8)val;
				}
				if (i < row_len) break;
				uint32 key  = ((uint32)row[0] << 24) | ((uint32)row[1] << 16) | ((uint32)row[2] << 8) | row[3];
				uint32 data = ((uint32)row[4] << 24) | ((uint32)row[5] << 16) | ((uint32)row[6] << 8) | row[7];
				key_schedule sched(key, key_schedule::ALL_MAPS);
				if (ref8.test_pipeline_validate(key, data, sched, OTHER_WORLD))
				{
					printf("// Other-world valid pair found:\n");
					printf("static const uint32 validate_other_key   = 0x%08X;\n", key);
					printf("static const uint32 validate_other_data  = 0x%08X;\n", data);
					printf("static const uint32 validate_other_sched = 0x%08X;\n\n", key);
					found = true;
				}
			}
			fclose(full_file);
			if (!found)
				printf("// Other-world valid pair not found in TM_full_test_cases.txt\n\n");
		}
	}

	printf("// ===== END OF DUMP =====\n");
}

float prev_percent;
void report_progress(double percentage)
{
	if (((int)(percentage * 100) + 1) > prev_percent)
	{
		printf("%f\n", percentage*100);
		prev_percent = (int)(percentage * 100) + 1;
	}
}

int main()
{
	char cwd[500];
	_getcwd(cwd, 500);
	printf("CWD: %s\n", cwd);
	prev_percent = 0;

	// Passes carnival checksum, might generate the correct machine code
	//int key = 0x2CA5B42D;
	//int data = 0x0009BE9F;

	// Passes other world checksum
	//int key = 0x1218E45F;
	int key = 0x2CA5B42D;
	int data = 0x0735B1D2;
	
	//int key = 561930401;
	//int data = 3388997632;
	
	key_schedule schedule_data(key, key_schedule::ALL_MAPS);	

	RNG rng;

	// Uncomment to generate test vectors for classes_test.cpp, then re-comment:
	dump_test_vectors(rng);
	return 0;

	std::vector<TM_base*> tms;
	
	tm_8 _tm_8(&rng);
	tms.push_back(&_tm_8);
	/*
	tm_32_8 _tm_32_8(&rng);
	tms.push_back(&_tm_32_8);
	tm_32_16 _tm_32_16(&rng);
	tms.push_back(&_tm_32_16);
	
	tm_64_8 _tm_64_8(&rng);
	tms.push_back(&_tm_64_8);
	tm_64_16 _tm_64_16(&rng);
	tms.push_back(&_tm_64_16);

	tm_sse2_m128_8 _tm_sse2_m128_8(&rng);
	tms.push_back(&_tm_sse2_m128_8);
	tm_sse2_m128_16 _tm_sse2_m128_16(&rng);
	tms.push_back(&_tm_sse2_m128_16);
	tm_sse2_m128s_8 _tm_sse2_m128s_8(&rng);
	tms.push_back(&_tm_sse2_m128s_8);
	
	tm_avx_m256_8 _tm_avx_m256_8(&rng);
	tms.push_back(&_tm_avx_m256_8);
	
	tm_avx_r256_8 _tm_avx_r256_8(&rng);
	tms.push_back(&_tm_avx_r256_8);
	*/
	tm_avx_r256s_8 _tm_avx_r256s_8(&rng);
	//tms.push_back(&_tm_avx_r256s_8);

	tm_avx_r128s_8 _tm_avx_r128s_8(&rng);
	tms.push_back(&_tm_avx_r128s_8);

	tm_avx_r128s_map_8 _tm_avx_r128s_map_8(&rng);
	//tms.push_back(&_tm_avx_r128s_map_8);
	/*
	
	tm_avx_r256_16 _tm_avx_r256_16(&rng);
	tms.push_back(&_tm_avx_r256_16);
	
	
	tm_avx_m256_16 _tm_avx_m256_16(&rng);
	tms.push_back(&_tm_avx_m256_16);
	
	tm_avx2_r256s_8 _tm_avx2_r256s_8(&rng);
	tms.push_back(&_tm_avx2_r256s_8);
	
	tm_avx512bw_r512_8 _tm_avx512bw_r512_8(&rng);
	tms.push_back(&_tm_avx512bw_r512_8);

	tm_avx512bw_r512s_8 _tm_avx512bw_r512s_8(&rng);
	tms.push_back(&_tm_avx512bw_r512s_8);
	*/
	
	for (std::vector<TM_base*>::iterator it = tms.begin(); it != tms.end(); ++it)
	{
		std::cout << (*it)->obj_name << std::endl;
		tm_tester tester2(*it);
		
		//run_load_fetch_tests(tester2);
		//run_alg_validity_tests(tester2);
		//run_expansion_validity_tests(tester2);
		//run_full_validity_tests(tester2);
		
		//run_speed_tests2(tester2, 10000000);
		//run_full_speed_test(tester2, 0x10000000);
		//run_full_speed_test(tester2, 20000000);
		//run_result_speed_test(tester2, 0x00100000);
		//run_checksum_tests(tester2);
		
		//run_result_tests(tester2);
	}
	
	
	
	
	uint8 result_data[400000];
	uint32 result_size;
	//_tm_avx_8_in_cpu_shuffled.run_bruteforce_data(0x2CA5B42D, 0x01E190D8, schedule_entries, 0x04000000, &report_progress, result_data, 400000, &result_size);
	_tm_8.run_bruteforce_data(key, data, schedule_data, 0x04000000, &report_progress, result_data, 400000, &result_size);
	
	printf("%i", result_size);
	
	
	return 0;
}
