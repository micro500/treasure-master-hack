#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>

#include "data_sizes.h"
#include "rng.h"
#include "key_schedule.h"

#define MAX_SOURCE_SIZE (0x100000)

int main()
{
	int codes_at_once = 2097152;

	uint8 IV[4]; // = { 0x2C,0xA5,0xB4,0x2D };
	//int key = 0xC038194D;
	//int data = 0x1F8FFD27;

	// Passes carnival checksum, might generate the correct machine code
	int key = 0x2CA5B42D;
	int data = 0x0009BE9F;

	//unsigned int key = 0;
	//unsigned int data = 0;
	IV[0] = (key >> 24) & 0xFF;
	IV[1] = (key >> 16) & 0xFF;
	IV[2] = (key >> 8) & 0xFF;
	IV[3] = key & 0xFF;


	int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11 };


	key_schedule_data schedule_data;
	schedule_data.as_uint8[0] = IV[0];
	schedule_data.as_uint8[1] = IV[1];
	schedule_data.as_uint8[2] = IV[2];
	schedule_data.as_uint8[3] = IV[3];

	key_schedule_entry schedule_entries[27];

	int schedule_counter = 0;
	for (int i = 0; i < 26; i++)
	{
		schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i], &schedule_data);

		if (map_list[i] == 0x22)
		{
			schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i], &schedule_data, 4);
		}
	}

	uint8* schedule_data_h = new uint8[27*4];
	for (int i = 0; i < 27; i++)
	{
		schedule_data_h[i*4] = schedule_entries[i].rng1;
		schedule_data_h[i*4+1] = schedule_entries[i].rng2;
		schedule_data_h[i*4+2] = schedule_entries[i].nibble_selector >> 16;
		schedule_data_h[i*4+3] = schedule_entries[i].nibble_selector & 0xFF;
	}




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
	command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateCommandQueue, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	/*
	cl_ulong mem_size;
	char device_data[1024];
	cl_uint cores;
	ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &mem_size, NULL);
	ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &cores, NULL);
	//ret = clGetDeviceInfo(device_id, CL_DEVICE_EXTENSIONS, 1024, device_data, NULL);
	printf("Global mem size: %lu\n", mem_size);
	printf("compute cores: %lu\n", cores);
	
	//printf("Extensions: %s\n", device_data);
	*/

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
	//kernel = clCreateKernel(program, "tm_process", &ret);
	//kernel = clCreateKernel(program, "blah_test", &ret);
	
	//kernel = clCreateKernel(program, "test_expand", &ret);
	//kernel = clCreateKernel(program, "test_alg", &ret);
	//kernel = clCreateKernel(program, "full_process", &ret);
	kernel = clCreateKernel(program, "tm_stats", &ret);
	

	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateKernel, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	size_t global_item_size[3] = {32, codes_at_once, 1};
	//size_t global_item_size[3] = {32, 1, 1};
	//size_t global_item_size[3] = { 32, 10000, 1 };

	size_t local_item_size[3] = {32,1,1};
	//size_t local_item_size[3] = {32,1,1};

	/*
	cl_mem code_space_d = NULL;
	code_space_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 128 * codes_at_once, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	// Set OpenCL Kernel Parameters 
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&code_space_d);
	*/

	/*
	cl_mem test_data_d = NULL;
	test_data_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 3 * 80000, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	// Set OpenCL Kernel Parameters 
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&test_data_d);
	*/

	
	cl_mem result_data_d = NULL;
	result_data_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 2 * codes_at_once, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	// Set OpenCL Kernel Parameters
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&result_data_d);
	


	uint16 * rng_table = new uint16[256*256];
	generate_rng_table(rng_table);

	uint8 * regular_rng_values = new uint8[0x10000 * 128];
	generate_regular_rng_values_8(regular_rng_values, rng_table);

	cl_mem regular_rng_values_d = NULL;
	regular_rng_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 128, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, regular_rng_values_d, CL_TRUE, 0, 0x10000 * 128, regular_rng_values, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&regular_rng_values_d);

	uint8 * alg0_values = new uint8[0x10000 * 128];
	generate_alg0_values_8(alg0_values, rng_table);

	cl_mem alg0_values_d = NULL;
	alg0_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 128, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, alg0_values_d, CL_TRUE, 0, 0x10000 * 128, alg0_values, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&alg0_values_d);

	uint8 * alg6_values = new uint8[0x10000 * 128];
	generate_alg6_values_8(alg6_values, rng_table);

	cl_mem alg6_values_d = NULL;
	alg6_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 128, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, alg6_values_d, CL_TRUE, 0, 0x10000 * 128, alg6_values, 0, NULL, NULL);
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
	generate_alg2_values_32_8(alg2_values, rng_table);

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
	generate_alg5_values_32_8(alg5_values, rng_table);

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

	uint8* expansion_values_h = new uint8[0x10000 * 128];
	generate_expansion_values_8(expansion_values_h, rng_table);

	cl_mem expansion_values_d = NULL;
	expansion_values_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x10000 * 128, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, expansion_values_d, CL_TRUE, 0, 0x10000 * 128, expansion_values_h, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 8, sizeof(cl_mem), (void *)&expansion_values_d);

	cl_mem schedule_data_d = NULL;
	schedule_data_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 27 * 4, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, schedule_data_d, CL_TRUE, 0, 27 * 4, schedule_data_h, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 9, sizeof(cl_mem), (void*)&schedule_data_d);

	unsigned char carnival_data[128] =
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x3D, 0x5E, 0xA1, 0xA6, 0xC8, 0x23,
		0xD7, 0x6E, 0x3F, 0x7C, 0xD2, 0x46, 0x1B, 0x9F, 0xAB, 0xD2,
		0x5C, 0x9B, 0x32, 0x43, 0x67, 0x30, 0xA0, 0xA4, 0x23, 0xF3,
		0x27, 0xBF, 0xEA, 0x21, 0x0F, 0x13, 0x31, 0x1A, 0x15, 0xA1,
		0x39, 0x34, 0xE4, 0xD2, 0x52, 0x6E, 0xA6, 0xF7, 0xF6, 0x43,
		0xD1, 0x28, 0x41, 0xD8, 0xDC, 0x55, 0xE1, 0xC5, 0x49, 0xF5,
		0xD4, 0x84, 0x52, 0x1F, 0x90, 0xAB, 0x26, 0xE4, 0x2A, 0xC3,
		0xC2, 0x59, 0xAC, 0x81, 0x58, 0x35, 0x7A, 0xC3, 0x51, 0x9A,
		0x01, 0x04, 0xF5, 0xE2, 0xFB, 0xA7, 0xAE, 0x8B, 0x46, 0x9A,
		0x27, 0x41, 0xFA, 0xDD, 0x63, 0x72, 0x23, 0x7E, 0x1B, 0x44,
		0x5A, 0x0B, 0x2A, 0x3C, 0x09, 0xFA, 0xA3, 0x59, 0x3C, 0xA1,
		0xF0, 0x90, 0x4F, 0x46, 0x9E, 0xD1, 0xD7, 0xF4 };

	cl_mem carnival_data_d = NULL;
	carnival_data_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 128, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, carnival_data_d, CL_TRUE, 0, 128, carnival_data, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 10, sizeof(cl_mem), (void*)&carnival_data_d);
	ret = clSetKernelArg(kernel, 11, sizeof(cl_mem), (void*)&carnival_data_d);




	ret = clSetKernelArg(kernel, 12, sizeof(unsigned int), (void*)&key);
	ret = clSetKernelArg(kernel, 13, sizeof(unsigned int), (void*)&data);



	FILE* pFile;
	/*
	pFile = fopen("../common/TM_expansion_test_cases.txt", "r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return 0;
	}

	unsigned char* test_case_answers = (unsigned char*)malloc(128 * 10000);
	unsigned char* input_ivs_h = (unsigned char*)malloc(8 * 10000);
	unsigned char* code_space_h = (unsigned char*)malloc(128 * 10000);


	// Fetch all the test cases
	for (int j = 0; j < 10000; j++)
	{
		
		for (int i = 0; i < 8; i++)
		{
			int val;
			fscanf(pFile, "%x,", &val);
			*(input_ivs_h + j * 8 + i) = val & 0xFF;
		}
		
		for (int i = 0; i < 128; i++)
		{
			int val;
			fscanf(pFile, "%x,", &val);
			test_case_answers[j * 128 + i] = val;
		}
		
	}

	cl_mem input_ivs_d = NULL;
	input_ivs_d = clCreateBuffer(context, CL_MEM_READ_WRITE, 10000 * 8, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clEnqueueWriteBuffer(command_queue, input_ivs_d, CL_TRUE, 0, 10000 * 8, input_ivs_h, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&input_ivs_d);
	*/

	
	/*
	pFile = fopen ("../common/TM_alg_test_cases.txt","r+");
	if (pFile == NULL)
	{
		printf("File error\n");
		return 0;
	}

	unsigned char *test_case_answers = (unsigned char*)malloc((128 + 2) * 80000);
	unsigned char *code_space_h = (unsigned char*)malloc(128 * 80000);
	unsigned char* test_data_h = (unsigned char*)malloc(3 * 80000);
	
	// Fetch all the test cases
	for (int j = 0; j < 80000; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			int val;
			fscanf (pFile, "%i,", &val);
			*(test_data_h + j*3 + i) = val & 0xFF;
		}

		for (int i = 0; i < 128; i++)
		{
			int val;
			fscanf (pFile, "%i,", &val);
			*(unsigned char*)(code_space_h + j*128 + i) = val & 0xFF;
		}

		for (int i = 0; i < 130; i++)
		{
			int val;
			fscanf (pFile, "%i,", &val);
			test_case_answers[j*130 + i] = val;
		}
	}

	ret = clEnqueueWriteBuffer(command_queue, code_space_d, CL_TRUE, 0, 128 * 80000, code_space_h, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	
	ret = clEnqueueWriteBuffer(command_queue, test_data_d, CL_TRUE, 0, 3 * 80000, test_data_h, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	*/

	cl_event event;
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, &event);
	if (ret != CL_SUCCESS)
	{
		printf("(%i) Error in clEnqueueNDRangeKernel, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	clWaitForEvents(1, &event);
	clFinish(command_queue);
	cl_ulong time_start;
	cl_ulong time_end;

	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

	double nanoSeconds = time_end - time_start;
	printf("OpenCl Execution time is: %0.5f milliseconds \n", nanoSeconds / 1000000.0);
	
	// Copy results from the memory buffer
	unsigned char* result_data_h = (unsigned char*)malloc(2 * codes_at_once);
	//ret = clEnqueueReadBuffer(command_queue, code_space_d, CL_TRUE, 0, 132 * 80000, code_space_h, 0, NULL, &event);
	ret = clEnqueueReadBuffer(command_queue, result_data_d, CL_TRUE, 0, 2 * codes_at_once, result_data_h, 0, NULL, &event);
	if (ret != CL_SUCCESS)
	{
		printf("(%i) Error in clEnqueueReadBuffer, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	/*
	ret = clEnqueueReadBuffer(command_queue, test_data_d, CL_TRUE, 0, 3 * 80000, test_data_h, 0, NULL, &event);
	if (ret != CL_SUCCESS)
	{
		printf("(%i) Error in clEnqueueReadBuffer, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
		//Cleanup(EXIT_FAILURE);
	}
	*/

	// Check the results of all test cases
	bool all_tests_passed = true;

	/*
	for (int j = 0; j < 10000; j++)
	{
		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (*(unsigned char*)(code_space_h + j * 128 + i) != test_case_answers[j * 128 + i])
			{
				matching = 0;
				break;
			}
		}

		if (matching == 0)
		{
			printf("Test %i: --FAIL--\n", j);
			all_tests_passed = false;
		}
	}
	*/
	/*
	
	for (int j = 0; j < 80000; j++)
	{
		int matching = 1;
		for (int i = 0; i < 128; i++)
		{
			if (*(unsigned char*)(code_space_h + j*128 + i) != test_case_answers[j * 130 + 2 + i])
			{
				matching = 0;
				break;
			}
		}

		if (*(unsigned short*)(test_data_h + j*3 + 1) != *(unsigned short*)(test_case_answers  + j * 130))
		{
			matching = 0;
		}

		if (matching == 0)
		{
			printf("Test %i (alg ?): --FAIL--\n",j);
			all_tests_passed = false;
		}
	}
	*/
	
		
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
	/*
	fclose (pFile);

	
	if (all_tests_passed)
	{
		printf("All tests passed!\n");
	}
	

	*/
	

	


 
	// Finalization
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);

	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	//ret = clReleaseMemObject(code_space_d);

	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
 
	ret = clReleaseDevice(device_id);
	
	delete[] platforms;
	delete[] devices;

	//free(code_space_h);
	free(source_str);
	
	
	return 0;
}