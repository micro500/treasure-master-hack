#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include <CL/cl.h>

#include "data_sizes.h"
#include "key_schedule.h"
#include "rng.h"

#define MAX_SOURCE_SIZE (0x100000)
 
struct rng_data
{
	// The resulting seed after advancing the RNG X number of steps
    short seed_forward_1;
	short seed_forward_128;

	// Results from the RNG simply put into an int container
	int regular_values[32];

	// Results from the RNG summated to be used for code expansion
	int expansion_values_high[32];
	int expansion_values_low[32];

	// Bitmasked values for algorithm 0
	int alg0_values[32];

	// Bitmasked values for algorithm 6
	int alg6_values[32];

	// High byte values for use with algorithms 1 and 4 (addition/subtraction)
	int arth_high_values[32];

	// Low byte values for use with algorithms 1 and 4 (addition/subtraction)
	int arth_low_values[32];

	// Bitmasked values for algorithms 2 and 5
	int alg2_value;
	int alg5_value;
};

int main()
{
cl_device_id device_id = NULL;
cl_context context = NULL;
cl_command_queue command_queue = NULL;
cl_program program = NULL;
cl_kernel kernel = NULL;
cl_platform_id platform_id = NULL;
cl_uint ret_num_devices;
cl_uint ret_num_platforms;
cl_int ret;
 
FILE *fp;
char fileName[] = "./tm.cl";
char *source_str;
size_t source_size;
 
/* Load the source code containing the kernel*/
fp = fopen(fileName, "r");
if (!fp) {
fprintf(stderr, "Failed to load kernel.\n");
exit(1);
}
source_str = (char*)malloc(MAX_SOURCE_SIZE);
source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
fclose(fp);
 
/* Get Platform and Device Info */
ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
if (ret != CL_SUCCESS)
{
    printf("Error in clGetPlatformIDs, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}
ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 2, &device_id, &ret_num_devices);
if (ret != CL_SUCCESS)
{
    printf("Error in clGetDeviceIDs, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}

/* Create OpenCL context */
context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
 if (ret != CL_SUCCESS)
{

    printf("Error in clCreateContext, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}

/* Create Command Queue */
command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
if (ret != CL_SUCCESS)
{
    printf("Error in clCreateCommandQueue, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}

cl_mem rng_space_d = NULL;
rng_space_d = clCreateBuffer(context, CL_MEM_READ_WRITE,sizeof(struct rng_data) * 0x10000, NULL, &ret);
if (ret != CL_SUCCESS)
{
    printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}

printf("Calculating rng cache data\n");
generate_rng_table();
struct rng_data *rng_space_h = (struct rng_data*)malloc(sizeof(struct rng_data) * 0x10000);
// Todo: Check endianness of all of these calculations
for (int i = 0; i < 0x10000; i++)
{
	unsigned short rng_seed = i & 0xFFFF;
	RNG rng;

	rng.seed((rng_seed >> 8) & 0xFF, rng_seed & 0xFF);
	rng.run();
	rng_space_h[i].seed_forward_1 = (rng.rng1 << 8) | rng.rng2;

	rng.seed((rng_seed >> 8) & 0xFF, rng_seed & 0xFF);
	for (int j = 0; j < 128; j++)
	{
		rng.run();
	}
	rng_space_h[i].seed_forward_128 = (rng.rng1 << 8) | rng.rng2;

	rng.seed((rng_seed >> 8) & 0xFF, rng_seed & 0xFF);
	for (int j = 31; j >= 0; j--)
	{
		rng_space_h[i].regular_values[j] = (rng.run() << 24);
		rng_space_h[i].regular_values[j] = rng_space_h[i].regular_values[j] | (rng.run() << 16);
		rng_space_h[i].regular_values[j] = rng_space_h[i].regular_values[j] | (rng.run() << 8);
		rng_space_h[i].regular_values[j] = rng_space_h[i].regular_values[j] | rng.run();
	}
	
	rng.seed((rng_seed >> 8) & 0xFF, rng_seed & 0xFF);
	uint8 expansion_values[8] = {0,0,0,0,0,0,0,0};
	rng_space_h[i].expansion_values_high[0] = 0;
	rng_space_h[i].expansion_values_low[1] = 0;
	for (int j = 1; j < 16; j++)
	{
		for (int k = 0; k < 8; k++)
		{
			expansion_values[k] = expansion_values[k] + rng.run();
		}
		
		rng_space_h[i].expansion_values_high[j*2] = (expansion_values[3] << 24) | (expansion_values[1] << 8);
		rng_space_h[i].expansion_values_low[j*2] = (expansion_values[2] << 16) | expansion_values[0];

		rng_space_h[i].expansion_values_high[j*2+1] = (expansion_values[7] << 24) | (expansion_values[5] << 8);
		rng_space_h[i].expansion_values_low[j*2+1] = (expansion_values[6] << 16) | expansion_values[4];
	}

	for (int j = 0; j < 32; j++)
	{
		rng_space_h[i].alg0_values[j] = (rng_space_h[i].regular_values[j] & 0x80808080) >> 7;

		// Alg6 is not reversed..wtf
		rng_space_h[i].alg6_values[j] = rng_space_h[i].regular_values[31-j] & 0x80808080;
		rng_space_h[i].alg6_values[j] = ((rng_space_h[i].alg6_values[j] & 0xFF) << 24) | ((rng_space_h[i].alg6_values[j] & 0xFF00) << 8) | ((rng_space_h[i].alg6_values[j] & 0xFF0000) >> 8) | ((rng_space_h[i].alg6_values[j] & 0xFF000000) >> 24);

		rng_space_h[i].arth_high_values[j] = rng_space_h[i].regular_values[j] & 0xFF00FF00;
		rng_space_h[i].arth_low_values[j] = rng_space_h[i].regular_values[j] & 0x00FF00FF;
	}

	rng.seed((rng_seed >> 8) & 0xFF, rng_seed & 0xFF);
	rng_space_h[i].alg2_value = (rng.run() >> 7) & 0x01;
	rng.seed((rng_seed >> 8) & 0xFF, rng_seed & 0xFF);
	rng_space_h[i].alg5_value = rng.run() & 0x80;

	/*
	if (rng_seed == 0x2ca5)
	{
		for (int j = 0; j < 32; j++)
		{
			//printf("%02X %02X %02X %02X ", rng_space_h[i].expansion_values_high[j*2], rng_space_h[i].expansion_values_low[j*2], rng_space_h[i].expansion_values_high[j*2+1], rng_space_h[i].expansion_values_low[j*2+1]);
			printf("%02X ", rng_space_h[i].alg0_values[j]);
		} 

		printf("\n");

		printf("%02X\n", rng_space_h[i].regular_values[31]);
	}
	*/
}
//printf("%04X\n", rng_space_h[0xe0bb].seed_forward_128);
printf("Finished calculating rng cache data\n");


ret = clEnqueueWriteBuffer(command_queue, rng_space_d, CL_TRUE, 0, sizeof(struct rng_data) * 0x10000, rng_space_h, 0, NULL, NULL);
if (ret != CL_SUCCESS)
{
    printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}



cl_mem code_space_d = NULL;
code_space_d = clCreateBuffer(context, CL_MEM_READ_WRITE,sizeof(unsigned char) * 8, NULL, &ret);
if (ret != CL_SUCCESS)
{
    printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}


unsigned char *code_space_h = (unsigned char*)malloc(sizeof(unsigned char) * 8);
code_space_h[0] = 0x2c;
code_space_h[1] = 0xa5;
code_space_h[2] = 0xb6;
code_space_h[3] = 0x2d;
code_space_h[4] = 0x00;
code_space_h[5] = 0x00;
code_space_h[6] = 0x00;
code_space_h[7] = 0x00;

ret = clEnqueueWriteBuffer(command_queue, code_space_d, CL_TRUE, 0,8,code_space_h, 0, NULL, NULL);
if (ret != CL_SUCCESS)
{
    printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}


cl_mem key_schedule_d = NULL;
key_schedule_d = clCreateBuffer(context, CL_MEM_READ_WRITE,sizeof(key_schedule_entry_new) * 27, NULL, &ret);
if (ret != CL_SUCCESS)
{
    printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}

int map_list[26] = { 0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B, 0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11};
key_schedule_data schedule_data;
schedule_data.as_uint8[0] = code_space_h[0];
schedule_data.as_uint8[1] = code_space_h[1];
schedule_data.as_uint8[2] = code_space_h[2];
schedule_data.as_uint8[3] = code_space_h[3];

key_schedule_entry schedule_entries[27];
int schedule_counter = 0;
for (int i = 0; i < 26; i++)
{
	schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data);

	if (map_list[i] == 0x22)
	{
		schedule_entries[schedule_counter++] = generate_schedule_entry(map_list[i],&schedule_data,4);
	}
}

key_schedule_entry_new schedule_entries_new[27];
for (int i = 0; i < 27; i++)
{
	schedule_entries_new[i].rng_seed = (schedule_entries[i].rng1 << 8) | schedule_entries[i].rng2;
	schedule_entries_new[i].nibble_selector = schedule_entries[i].nibble_selector;
}

ret = clEnqueueWriteBuffer(command_queue, key_schedule_d, CL_TRUE, 0, sizeof(key_schedule_entry_new) * 27, schedule_entries_new, 0, NULL, NULL);
if (ret != CL_SUCCESS)
{
    printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}



cl_mem result_space_d = NULL;
result_space_d = clCreateBuffer(context, CL_MEM_READ_WRITE,sizeof(unsigned char) * 128, NULL, &ret);
if (ret != CL_SUCCESS)
{
    printf("Error in clCreateBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}


unsigned char *result_space_h = (unsigned char*)malloc(sizeof(unsigned char) * 128);
for (int i = 0; i < 128; i++)
{
	result_space_h[i] = 0;
}

ret = clEnqueueWriteBuffer(command_queue, result_space_d, CL_TRUE, 0,128,result_space_h, 0, NULL, NULL);
if (ret != CL_SUCCESS)
{
    printf("Error in clEnqueueWriteBuffer, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
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


if (ret != CL_SUCCESS)
{
    printf("Error in clCreateKernel, Line %u in file %s !!!\n\n", __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}
 
// Set OpenCL Kernel Parameters 
ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&code_space_d);
ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&rng_space_d);
ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&key_schedule_d);
ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&result_space_d);

size_t global_item_size[3] = {2048, 1024, 16};
//size_t global_item_size[3] = {32, 1, 1};

size_t local_item_size[3] = {32,4,1};
//size_t local_item_size[3] = {32,1,1};


using std::chrono::duration_cast;
using std::chrono::microseconds;
typedef std::chrono::high_resolution_clock clock;
auto start = clock::now();

for (int i = 0; i < 10; i++)
{
	cl_event event;
ret = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, &event);
if (ret != CL_SUCCESS)
{
    printf("(%i) Error in clEnqueueNDRangeKernel, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}
clWaitForEvents(1, &event);
}

//ret = clEnqueueTask(command_queue, kernel, 0, NULL,NULL);
 

// Copy results from the memory buffer

unsigned int blah = 0;
ret = clEnqueueReadBuffer(command_queue, code_space_d, CL_TRUE, 0,4,&blah, 0, NULL, NULL);
if (ret != CL_SUCCESS)
{
    printf("(%i) Error in clEnqueueReadBuffer, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}
auto end = clock::now();
long long sum = duration_cast<microseconds>(end-start).count();
	
printf("%i\n",sum);


/* Display Result */
//puts(string);
printf("%i\n",blah);


ret = clEnqueueReadBuffer(command_queue,result_space_d, CL_TRUE, 0,128,result_space_h, 0, NULL, NULL);
if (ret != CL_SUCCESS)
{
    printf("(%i) Error in clEnqueueReadBuffer, Line %u in file %s !!!\n\n", ret, __LINE__, __FILE__);
    //Cleanup(EXIT_FAILURE);
}

for (int i = 0; i < 128; i++)
{
	printf("%02X ", result_space_h[i]);
}
printf("\n");



// Query binary (PTX file) size
size_t bin_sz;
ret = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &bin_sz, NULL);
 
// Read binary (PTX file) to memory buffer
unsigned char *bin = (unsigned char *)malloc(bin_sz);
ret = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char *), &bin, NULL);
 
// Save PTX to add_vectors_ocl.ptx
fp = fopen("add_vectors_ocl.ptx", "wb");
fwrite(bin, sizeof(char), bin_sz, fp);
fclose(fp);
free(bin);


 
/* Finalization */
ret = clFlush(command_queue);
ret = clFinish(command_queue);
ret = clReleaseKernel(kernel);
ret = clReleaseProgram(program);
//ret = clReleaseMemObject(memobj);
ret = clReleaseCommandQueue(command_queue);
ret = clReleaseContext(context);
 
free(source_str);
 
return 0;
}