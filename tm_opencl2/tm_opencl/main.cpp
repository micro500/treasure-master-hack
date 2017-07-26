#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>

#include "data_sizes.h"
#include "rng.h"

#define MAX_SOURCE_SIZE (0x100000)

int main()
{
	cl_device_id *devices = NULL;
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_platform_id * platforms = NULL;
	cl_platform_id curPlatform;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;

	FILE *fp;
	char fileName[] = "./tm.cl";
	char *source_str;
	size_t source_size;
 
	// Load the source code containing the kernel
	fp = fopen(fileName, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		return 1;
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);
 
	cl_int status = clGetPlatformIDs(0, NULL, &ret_num_platforms);
 
	platforms = new cl_platform_id[ret_num_platforms];
	
	// Get Platform and Device Info
	ret = clGetPlatformIDs(1, platforms, &ret_num_platforms);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clGetPlatformIDs, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	
	curPlatform = platforms[0];

	ret = clGetDeviceIDs(curPlatform, CL_DEVICE_TYPE_DEFAULT, 0, NULL, &ret_num_devices);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clGetDeviceIDs, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	devices = new cl_device_id[ret_num_devices];

	ret = clGetDeviceIDs(curPlatform, CL_DEVICE_TYPE_DEFAULT, ret_num_devices, devices, nullptr);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clGetDeviceIDs, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	device_id = devices[0];

	
	// Create OpenCL context
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	 if (ret != CL_SUCCESS)
	{

		printf("Error in clCreateContext, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	// Create Command Queue
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateCommandQueue, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	// Create Kernel Program from the source 
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str,
	(const size_t *)&source_size, &ret);

	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateProgramWithSource, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
 
	// Build Kernel Program 
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	if (ret != CL_SUCCESS)
	{
		printf("Error in clBuildProgram, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);

		if (ret == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log
			char *log = (char *) malloc(log_size);

			// Get the log
			clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			// Print the log
			printf("%s\n", log);
		}
	}
 
	// Create OpenCL Kernel 
	kernel = clCreateKernel(program, "tm_process", &ret);
	//kernel = clCreateKernel(program, "blah_test", &ret);
	

	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateKernel, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
 
	size_t global_item_size[3] = {512, 10000, 1};
	//size_t global_item_size[3] = {32, 1, 1};

	size_t local_item_size[3] = {64,1,1};
	//size_t local_item_size[3] = {32,1,1};

	
	cl_mem code_space_d = NULL;
	code_space_d = clCreateBuffer(context, CL_MEM_READ_WRITE, (128 + 2 + 1)*2 * 80000, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	// Set OpenCL Kernel Parameters 
	//ret = clSetKernelArg(kernel, 0, 7);
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&code_space_d);

	uint16 * rng_table = new uint16[256*256];
	generate_rng_table(rng_table);

	uint16 * regular_rng_values = new uint16[0x10000 * 128];
	generate_regular_rng_values_16(regular_rng_values, rng_table);

	cl_mem regular_rng_values_d = NULL;
	regular_rng_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 128 * 2, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, regular_rng_values_d, CL_TRUE, 0, 0x10000 * 128 * 2, regular_rng_values, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&regular_rng_values_d);

	uint16 * alg0_values = new uint16[0x10000 * 128];
	generate_alg0_values_16(alg0_values, rng_table);

	cl_mem alg0_values_d = NULL;
	alg0_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 128 * 2, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, alg0_values_d, CL_TRUE, 0, 0x10000 * 128 * 2, alg0_values, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&alg0_values_d);

	uint16 * alg6_values = new uint16[0x10000 * 128];
	generate_alg6_values_16(alg6_values, rng_table);

	cl_mem alg6_values_d = NULL;
	alg6_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 128 * 2, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, alg6_values_d, CL_TRUE, 0, 0x10000 * 128 * 2, alg6_values, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&alg6_values_d);



	uint16* rng_seed_forward_1 = new uint16[256*256];
	generate_seed_forward_1(rng_seed_forward_1, rng_table);
	
	cl_mem rng_seed_forward_1_d = NULL;
	rng_seed_forward_1_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 256*256*2, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, rng_seed_forward_1_d, CL_TRUE, 0, 256*256 * 2, rng_seed_forward_1, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&rng_seed_forward_1_d);


	uint16* rng_seed_forward_128 = new uint16[256*256];
	generate_seed_forward_128(rng_seed_forward_128, rng_table);
	
	cl_mem rng_seed_forward_128_d = NULL;
	rng_seed_forward_128_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 256*256*2, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, rng_seed_forward_128_d, CL_TRUE, 0, 256*256 * 2, rng_seed_forward_128, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&rng_seed_forward_128_d);


	uint32 * alg2_values = new uint32[0x10000];
	generate_alg2_values_32(alg2_values, rng_table);

	cl_mem alg2_values_d = NULL;
	alg2_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 4, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, alg2_values_d, CL_TRUE, 0, 0x10000 * 4, alg2_values, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&alg2_values_d);

	uint32 * alg5_values = new uint32[0x10000];
	generate_alg5_values_32(alg5_values, rng_table);

	cl_mem alg5_values_d = NULL;
	alg5_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 4, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, alg5_values_d, CL_TRUE, 0, 0x10000 * 4, alg5_values, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 7, sizeof(cl_mem), (void *)&alg5_values_d);

	FILE * pFile;

	pFile = fopen ("TM_test_cases_OUT.txt","r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return 0;
	}

	unsigned char *test_case_answers = (unsigned char*)malloc((128 + 2) * 80000);
	unsigned char *code_space_h = (unsigned char*)malloc((128*2 + 2 + 1) * 80000);

	// Fetch all the test cases
	for (int j = 0; j < 80000; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			int val;
			fscanf (pFile, "%i,", &val);
			*(code_space_h + j*(128*2+2+1) + i) = val & 0xFF;
		}

		for (int i = 0; i < 128; i++)
		{
			int val;
			fscanf (pFile, "%i,", &val);
			*(unsigned short*)(code_space_h + j*(128*2+2+1) + i*2 + 3) = val & 0xFF;
		}

		for (int i = 0; i < 130; i++)
		{
			int val;
			fscanf (pFile, "%i,", &val);
			test_case_answers[j*130 + i] = val;
		}
	}

	ret = clEnqueueWriteBuffer(command_queue, code_space_d, CL_TRUE, 0, (128*2 + 2 + 1) * 80000, code_space_h, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	






	cl_event event;
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, &event);
	if (ret != CL_SUCCESS)
	{
		printf("(%i) Error in clEnqueueNDRangeKernel, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	clWaitForEvents(1, &event);

	// Copy results from the memory buffer
	ret = clEnqueueReadBuffer(command_queue, code_space_d, CL_TRUE, 0, (128*2 + 2 + 1) * 80000, code_space_h, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("(%i) Error in clEnqueueReadBuffer, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	// Check the results of all test cases
	bool all_tests_passed = true;
	for (int j = 0; j < 80000; j++)
	{
		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (*(unsigned short*)(code_space_h + j*(128*2+2+1) + i*2 + 3) != test_case_answers[j * 130 + 2 + i])
			{
				matching = 0;
				break;
			}
		}

		if (*(unsigned short*)(code_space_h + j*(128*2+2+1) + 1) != *(unsigned short*)(test_case_answers  + j * 130))
		{
			matching = 0;
		}

		if (matching == 0)
		{
			printf("Test %i (alg ?): --FAIL--\n",j);
			all_tests_passed = false;
		}
	}
	
		
	/*
	int matching = 1;
	for (int j = 0; j < 128; j++)
	{
		if (test_data[j] != test_case[3+128+2+j])
		{
			matching = 0;
			break;
		}
	}

	if (rng_seed != output_rng_seed)
	{
		matching = 0;
	}

	if (matching == 0)
	{
		printf("Test %i (alg %i): --FAIL--\n",j,test_case[0]);
		all_tests_passed = false;
	}
	*/
	/*
	else
	{
		printf("Test %i: Pass\n",j);
	}
	*/

	fclose (pFile);

	
	if (all_tests_passed)
	{
		printf("All tests passed!\n");
	}
	


	

	


 
	// Finalization
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);

	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(code_space_d);

	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
 
	ret = clReleaseDevice(device_id);
	
	delete[] platforms;
	delete[] devices;

	free(code_space_h);
	free(source_str);
	
	
	return 0;
}