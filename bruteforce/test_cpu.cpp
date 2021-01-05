#include "tm_base.h"
#include "tm_8.h"
#include "tm_mmx_8.h"
#include "tm_32_8.h"
#include "tm_32_16.h"
#include "tm_64_8.h"
#include "tm_64_16.h"
#include "tm_128_8.h"
#include "tm_sse2_8_shuffled.h"
#include "tm_128_16.h"
#include "tm_avx_in_cpu.h"
#include "tm_avx_8.h"
#include "tm_avx_8_in_cpu.h"
#include "tm_avx_8_in_cpu_shuffled.h"
#include "tm_avx_16.h"
#include "tm_avx2_8_in_cpu_shuffled.h"
#include "tm_avx512_8_in_cpu_shuffled.h"
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
	prev_percent = 0;
	
	// Passes carnival checksum, might generate the correct machine code
	//int key = 0x2CA5B42D;
	//int data = 0x0009BE9F;

	// Passes other world checksum
	//int key = 0x1218E45F;
	int key = 0x2CA5B42D;
	int data = 0x0735B1D2;
	
	key_schedule schedule_data(key, key_schedule::ALL_MAPS);	

	RNG rng;

	std::vector<TM_base*> tms;
	/*
	tm_8 _tm_8(&rng);
	tms.push_back(&_tm_8);
	
	tm_32_8 _tm_32_8(&rng);
	tms.push_back(&_tm_32_8);
	tm_32_16 _tm_32_16(&rng);
	tms.push_back(&_tm_32_16);

	tm_64_8 _tm_64_8(&rng);
	tms.push_back(&_tm_64_8);
	tm_64_16 _tm_64_16(&rng);
	tms.push_back(&_tm_64_16);

	tm_128_8 _tm_128_8(&rng);
	tms.push_back(&_tm_128_8);
	tm_128_16 _tm_128_16(&rng);
	tms.push_back(&_tm_128_16);
	tm_sse2_8_shuffled _tm_sse2_8_shuffled(&rng);
	tms.push_back(&_tm_sse2_8_shuffled);
	
	tm_avx_8 _tm_avx_8(&rng);
	tms.push_back(&_tm_avx_8);
	tm_avx_8_in_cpu _tm_avx_8_in_cpu(&rng);
	tms.push_back(&_tm_avx_8_in_cpu);*/
	tm_avx_8_in_cpu_shuffled _tm_avx_8_in_cpu_shuffled(&rng);
	tms.push_back(&_tm_avx_8_in_cpu_shuffled);/*
	tm_avx_in_cpu _tm_avx_in_cpu(&rng);
	tms.push_back(&_tm_avx_in_cpu);
	
	tm_avx_16 _tm_avx_16(&rng);
	tms.push_back(&_tm_avx_16);
	
	tm_avx2_8_in_cpu_shuffled _tm_avx2_8_in_cpu_shuffled(&rng);
	tms.push_back(&_tm_avx2_8_in_cpu_shuffled);
	
	tm_avx512_8_in_cpu_shuffled _tm_avx512_8_in_cpu_shuffled(&rng);
	tms.push_back(&_tm_avx512_8_in_cpu_shuffled);
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
		//run_full_speed_test(tester2, 0x01000000);
		run_result_speed_test(tester2, 0x00100000);
		//run_checksum_tests(tester2);
		
		//run_result_tests(tester2);
	}
	
	
	/*
	uint8 result_data[400000];
	uint32 result_size;
	//_tm_avx_8_in_cpu_shuffled.run_bruteforce_data(0x2CA5B42D, 0x01E190D8, schedule_entries, 0x04000000, &report_progress, result_data, 400000, &result_size);
	_tm_avx_8_in_cpu_shuffled.run_bruteforce_data(0x2CA5B42D, 0xf73a2612, schedule_entries, 0x04000000, &report_progress, result_data, 400000, &result_size);
	
	printf("%i", result_size);
	*/
	return 0;
}
