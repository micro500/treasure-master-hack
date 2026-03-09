#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>

#include "rng_obj.h"
#include "tm_opencl_32x.h"
#include "key_schedule.h"

#include "opencl.h"

#define MAX_SOURCE_SIZE (0x100000)

void test(opencl &_cl)
{
	cl_program program = _cl.create_program("./bruteforce/opencl_test/tm.cl");
	_cl.build_program(program);
	output_kernel_asm_to_file(program, "kernel.ptx");

	cl_kernel kernel = _cl.create_kernel(program, "expand_test");

	size_t* kernel_wg_size = _cl.get_kernel_preferred_work_group_size_multiple(kernel);
	std::cout << "Kernel prefered WG size: " << *kernel_wg_size << std::endl;

	uint32_t key = 0x2CA5B42D;
	uint32_t data = 0x0009BE9F;

	RNG rng;
	rng.generate_expansion_values_8();




	set_kernel_arg<uint32_t>(kernel, 0, &key);
	set_kernel_arg<uint32_t>(kernel, 1, &data);

	cl_mem expansion_values_d = _cl.create_readonly_buffer(0x100000 * 128);
	_cl.copy_mem_to_device(expansion_values_d, rng.expansion_values_8, 0x10000 * 128);
	set_kernel_arg<cl_mem>(kernel, 2, &expansion_values_d);

	cl_mem result_values_d = _cl.create_readwrite_buffer(128);
	set_kernel_arg<cl_mem>(kernel, 3, &result_values_d);

	size_t global_item_size[3] = { 32, 1, 1 };

	size_t local_item_size[3] = { 32, 1, 1 };

	cl_event event = _cl.run_kernel(kernel, 3, NULL, global_item_size, local_item_size);
	clWaitForEvents(1, &event);
	_cl.finish();

	cl_command_queue command_queue;
	cl_context context = NULL;
	cl_device_id device_id;







	//cl_event event;
	//ret = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, &event);
	//if (ret != CL_SUCCESS)
	//{
	//	printf("(%i) Error in clEnqueueNDRangeKernel, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
	//	//Cleanup(EXIT_FAILURE);
	//}
	//clWaitForEvents(1, &event);
	//clFinish(command_queue);
	cl_ulong time_start;
	cl_ulong time_end;

	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

	double nanoSeconds = time_end - time_start;
	printf("OpenCl Execution time is: %0.5f milliseconds \n", nanoSeconds / 1000000.0);

	uint8_t results[128];
	_cl.copy_mem_from_device(result_values_d, results, 128);

		//// Finalization
	//ret = clFlush(command_queue);
	//ret = clFinish(command_queue);

	//ret = clReleaseKernel(kernel);
	//ret = clReleaseProgram(program);
	////ret = clReleaseMemObject(code_space_d);

	//ret = clReleaseCommandQueue(command_queue);
	//ret = clReleaseContext(context);
	//
	   //ret = clReleaseDevice(device_id);

	   //free(code_space_h);
	   //delete[] source_str;

}


void run_alg_validity_tests(tm_opencl_32x& tm)
{
	std::ifstream test_data_file("../../tm/TM_alg_test_cases.txt", std::ios::binary);
	if (!test_data_file.is_open()) {
		std::cout << "Error opening test data file!" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	uint8 test_case[128 + 128 + 3 + 2];

	bool all_tests_passed = true;
	uint8_t result_data[128];
	std::string input_str;
	int j = 0;
	while (std::getline(test_data_file, input_str))
	{
		std::stringstream ss(input_str);

		int i;
		int cnt = 0;
		while (ss >> i)
		{
			test_case[cnt++] = i;

			if (ss.peek() == ',')
				ss.ignore();
		}

		uint16 rng_seed = (test_case[1] << 8) | test_case[2];
		uint16 output_rng_seed = (test_case[3 + 128] << 8) | test_case[3 + 128 + 1];

		uint8 test_data[128];
		for (int i = 0; i < 128; i++)
		{
			test_data[i] = test_case[3 + i];
		}

		/*
		if (test_case[0] != 7 && test_case[0] != 3 && test_case[0] != 1 && test_case[0] != 4 && test_case[0] != 0 && test_case[0] != 6)
		{
			continue;
		}
		*/

		tm.load_data(test_data);

		tm.run_alg(test_case[0], &rng_seed, 1);

		tm.fetch_data(result_data);

		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (result_data[i] != test_case[3 + 128 + 2 + i])
			{
				matching = 0;
				break;
			}
		}

		/*if (rng_seed != output_rng_seed)
		{
			matching = 0;
		}*/

		if (matching == 0)
		{
			printf("Alg test %i (alg %i): --FAIL--\n", j, test_case[0]);
			all_tests_passed = false;
		}
		j++;
	}
	test_data_file.close();

	if (all_tests_passed)
	{
		printf("Alg tests passed.\n");
	}
}

void run_map_tests(tm_opencl_32x& tm)
{
	uint8_t result_data[128];

	uint32 key = 0x2CA5B42D;
	key_schedule schedule_data(key, key_schedule::ALL_MAPS);

	tm.run_all_maps(key, 0, schedule_data);

	tm.fetch_data(result_data);
}



int main()
{
	cl_int ret;

	opencl _cl(0, 0);

	char* plat_name = _cl.get_platform_name();
	char* plat_vendor = _cl.get_platform_vendor();

	std::cout << "Platform: " << plat_name << std::endl;
	std::cout << "Platform vendor: " << plat_vendor << std::endl;
	//printf("Platform: %s\n", plat_name);
	//printf("Platform vendor: %s\n", plat_vendor);

	char* dev_name = _cl.get_device_name();
	cl_device_type* dev_type = _cl.get_device_type();
	cl_uint* dev_compute_units = _cl.get_device_max_compute_units();
	cl_uint* dev_max_wu_dim = _cl.get_device_max_work_item_dimensions();
	size_t* dev_max_wu_sizes = _cl.get_device_max_work_item_sizes();
	size_t* dev_max_wg_size = _cl. get_device_max_work_group_sizes();

	std::cout << "Device: " << dev_name << std::endl;
	std::cout << "Device type: " << (int)*dev_type << std::endl;
	std::cout << "Device compute units: " << (int)*dev_compute_units << std::endl;
	std::cout << "Device workitem dims: " << (int)*dev_max_wu_dim << std::endl;

	std::cout << "Device workitem sizes: {";
	for (int i = 0; i < *dev_max_wu_dim; i++)
	{
		if (i > 0)
		{
			std::cout << ", ";
		}
		std::cout << (int)(dev_max_wu_sizes[i]);
	}
	std::cout << "}" << std::endl;
	std::cout << "Device workgroup max: " << (int)*dev_max_wg_size << std::endl;


	RNG rng;
	tm_opencl_32x _tm(&rng, &_cl);
	
	


	//std::ifstream test_data_file("../../tm/TM_expansion_test_cases2.txt", std::ios::binary);
	//if (!test_data_file.is_open()) {
	//	std::cout << "Error opening test data file!" << std::endl;
	//	std::exit(EXIT_FAILURE);
	//}

	//uint8_t test_case[8 + 128];

	//uint8_t result_data[128];
	//bool all_tests_passed = true;
	//std::string input_str;
	//int j = 0;
	//while (std::getline(test_data_file, input_str))
	////for (int j = 0; j < 1000000; j++)
	//{
	//	//test_data_file.read((char*)test_case, 8 + 128);

	//	

	//	std::stringstream ss(input_str);

	//	int i;
	//	int cnt = 0;
	//	while (ss >> i)
	//	{
	//		test_case[cnt++] = i;

	//		if (ss.peek() == ',')
	//			ss.ignore();
	//	}

	//	uint32 key = (test_case[3] << 24) | (test_case[2] << 16) | (test_case[1] << 8) | test_case[0];
	//	uint32 data = (test_case[7] << 24) | (test_case[6] << 16) | (test_case[5] << 8) | test_case[4];

	//	_tm.expand(key, data);
	//	_tm.fetch_data(result_data);
	//	//tester.run_expansion(key, data, result_data);

	//	int matching = 1;
	//	for (int i = 0; i < 128; i++)
	//	{
	//		if (result_data[i] != test_case[8 + i])
	//		{
	//			matching = 0;
	//			break;
	//		}
	//	}

	//	if (matching == 0)
	//	{
	//		printf("Expansion test %i: --FAIL--\n", j);
	//		all_tests_passed = false;
	//	}
	//	j++;
	//}
	//
	//test_data_file.close();

	//if (all_tests_passed)
	//{
	//	printf("Expansion tests passed.\n");
	//}

	//run_alg_validity_tests(_tm);
	run_map_tests(_tm);
	
	return 0;
}