#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include <CL/cl.h>

extern "C" {
#include "../boinc_app/sha256.h"
}

#include "data_sizes.h"
#include "rng.h"
#include "key_schedule.h"

#define MAX_SOURCE_SIZE (0x100000)

static const int      CHALLENGE_COUNT   = 100;
static const int      RESULT_ENTRY_SIZE = 5;
static const int      SHA256_SIZE       = 32;
static const uint32_t BATCH_SIZE        = 1u << 20;  // 4M inputs per kernel launch

// -------------------------------------------------------------------
// Command-line arguments
// -------------------------------------------------------------------
struct Args {
	uint32_t    key_id        = 749057069u;
	uint32_t    range_start   = 4147783186u;
	uint32_t    workunit_size = 1u << 30;
	std::string seed;
};

static Args parse_args(int argc, char** argv)
{
	Args args;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--key_id") == 0 && i + 1 < argc)
			args.key_id = (uint32_t)strtoul(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "--range_start") == 0 && i + 1 < argc)
			args.range_start = (uint32_t)strtoul(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "--workunit_size") == 0 && i + 1 < argc)
			args.workunit_size = (uint32_t)strtoul(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
			args.seed = argv[++i];
	}
	return args;
}

// -------------------------------------------------------------------
// Utility
// -------------------------------------------------------------------
static void hex_encode(const uint8_t* data, int len, char* out)
{
	for (int i = 0; i < len; i++)
		sprintf(&out[i * 2], "%02x", (unsigned)data[i]);
	out[len * 2] = '\0';
}

// -------------------------------------------------------------------
// OpenCL helpers
// -------------------------------------------------------------------
template <typename T>
T* get_platform_param(cl_platform_id platform_id, cl_platform_info param_name)
{
	size_t ret_size;
	clGetPlatformInfo(platform_id, param_name, 0, NULL, &ret_size);
	T* val = new T[ret_size];
	clGetPlatformInfo(platform_id, param_name, ret_size, val, &ret_size);
	return val;
}

template <typename T>
T* get_device_param(cl_device_id device_id, cl_device_info param_name)
{
	size_t ret_size;
	clGetDeviceInfo(device_id, param_name, 0, NULL, &ret_size);
	T* val = new T[ret_size];
	clGetDeviceInfo(device_id, param_name, ret_size, val, &ret_size);
	return val;
}

// -------------------------------------------------------------------
// Main
// -------------------------------------------------------------------
int main(int argc, char** argv)
{
	Args args = parse_args(argc, argv);

	// -------------------------------------------------------------------
	// Build key schedule
	// -------------------------------------------------------------------
	uint32_t key = args.key_id;

	uint8 map_list[26] = {
		0x00, 0x02, 0x05, 0x04, 0x03, 0x1D, 0x1C, 0x1E, 0x1B,
		0x07, 0x08, 0x06, 0x09, 0x0C, 0x20, 0x21, 0x22, 0x23,
		0x24, 0x25, 0x26, 0x0E, 0x0F, 0x10, 0x12, 0x11
	};

	key_schedule_data schedule_data;
	schedule_data.as_uint8[0] = (key >> 24) & 0xFF;
	schedule_data.as_uint8[1] = (key >> 16) & 0xFF;
	schedule_data.as_uint8[2] = (key >>  8) & 0xFF;
	schedule_data.as_uint8[3] =  key        & 0xFF;

	key_schedule_entry schedule_entries[27];
	int schedule_count = 0;
	for (int i = 0; i < 26; i++) {
		schedule_entries[schedule_count++] = generate_schedule_entry(map_list[i], &schedule_data);
		if (map_list[i] == 0x22)
			schedule_entries[schedule_count++] = generate_schedule_entry(map_list[i], &schedule_data, 4);
	}

	uint8* schedule_data_h = new uint8[27 * 4];
	for (int i = 0; i < schedule_count; i++) {
		schedule_data_h[i*4+0] = schedule_entries[i].rng1;
		schedule_data_h[i*4+1] = schedule_entries[i].rng2;
		schedule_data_h[i*4+2] = (schedule_entries[i].nibble_selector >> 8) & 0xFF;
		schedule_data_h[i*4+3] =  schedule_entries[i].nibble_selector       & 0xFF;
	}

	// -------------------------------------------------------------------
	// OpenCL setup
	// -------------------------------------------------------------------
	cl_int ret;
	cl_uint ret_num_platforms;
	clGetPlatformIDs(0, NULL, &ret_num_platforms);
	cl_platform_id* platforms = new cl_platform_id[ret_num_platforms];
	clGetPlatformIDs(ret_num_platforms, platforms, &ret_num_platforms);

	cl_platform_id curPlatform = platforms[0];
	printf("Platform: %s\n", get_platform_param<char>(curPlatform, CL_PLATFORM_NAME));

	cl_uint ret_num_devices;
	clGetDeviceIDs(curPlatform, CL_DEVICE_TYPE_GPU, 0, NULL, &ret_num_devices);
	cl_device_id* devices = new cl_device_id[ret_num_devices];
	clGetDeviceIDs(curPlatform, CL_DEVICE_TYPE_DEFAULT, ret_num_devices, devices, nullptr);
	cl_device_id device_id = devices[0];
	printf("Device: %s\n", get_device_param<char>(device_id, CL_DEVICE_NAME));

	cl_context       context       = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	// Load and build kernel
	FILE* fp = fopen("./tm.cl", "r");
	if (!fp) { fprintf(stderr, "Failed to load kernel.\n"); return 1; }
	char* source_str = (char*)malloc(MAX_SOURCE_SIZE);
	size_t source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&source_str, &source_size, &ret);
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if (ret != CL_SUCCESS) {
		size_t log_size;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char* log = (char*)malloc(log_size);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		fprintf(stderr, "Build error:\n%s\n", log);
		return 1;
	}

	cl_kernel kernel = clCreateKernel(program, "tm_stats", &ret);
	if (ret != CL_SUCCESS) { fprintf(stderr, "Failed to create kernel.\n"); return 1; }

	// -------------------------------------------------------------------
	// Allocate GPU buffers
	// -------------------------------------------------------------------
	uint32_t batch_cap = (args.workunit_size < BATCH_SIZE) ? args.workunit_size : BATCH_SIZE;

	cl_mem result_data_d = clCreateBuffer(context, CL_MEM_READ_WRITE, batch_cap, NULL, &ret);

	uint16* rng_table = new uint16[256 * 256];
	generate_rng_table(rng_table);

	uint8* regular_rng_values = new uint8[0x10000 * 128];
	generate_regular_rng_values_8(regular_rng_values, rng_table);
	cl_mem regular_rng_values_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 0x10000 * 128, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, regular_rng_values_d, CL_TRUE, 0, 0x10000 * 128, regular_rng_values, 0, NULL, NULL);

	uint8* alg0_values = new uint8[0x10000 * 128];
	generate_alg0_values_8(alg0_values, rng_table);
	cl_mem alg0_values_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 0x10000 * 128, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, alg0_values_d, CL_TRUE, 0, 0x10000 * 128, alg0_values, 0, NULL, NULL);

	uint8* alg6_values = new uint8[0x10000 * 128];
	generate_alg6_values_8(alg6_values, rng_table);
	cl_mem alg6_values_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 0x10000 * 128, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, alg6_values_d, CL_TRUE, 0, 0x10000 * 128, alg6_values, 0, NULL, NULL);

	uint16* rng_seed_forward_1 = new uint16[256 * 256];
	generate_seed_forward_1(rng_seed_forward_1, rng_table);
	cl_mem rng_seed_forward_1_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 256 * 256 * 2, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, rng_seed_forward_1_d, CL_TRUE, 0, 256 * 256 * 2, rng_seed_forward_1, 0, NULL, NULL);

	uint16* rng_seed_forward_128 = new uint16[256 * 256];
	generate_seed_forward_128(rng_seed_forward_128, rng_table);
	cl_mem rng_seed_forward_128_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 256 * 256 * 2, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, rng_seed_forward_128_d, CL_TRUE, 0, 256 * 256 * 2, rng_seed_forward_128, 0, NULL, NULL);

	uint32* alg2_values = new uint32[0x10000];
	generate_alg2_values_32_8(alg2_values, rng_table);
	cl_mem alg2_values_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 0x10000 * 4, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, alg2_values_d, CL_TRUE, 0, 0x10000 * 4, alg2_values, 0, NULL, NULL);

	uint32* alg5_values = new uint32[0x10000];
	generate_alg5_values_32_8(alg5_values, rng_table);
	cl_mem alg5_values_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 0x10000 * 4, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, alg5_values_d, CL_TRUE, 0, 0x10000 * 4, alg5_values, 0, NULL, NULL);

	uint8* expansion_values_h = new uint8[0x10000 * 128];
	generate_expansion_values_8(expansion_values_h, rng_table);
	cl_mem expansion_values_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 0x10000 * 128, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, expansion_values_d, CL_TRUE, 0, 0x10000 * 128, expansion_values_h, 0, NULL, NULL);

	cl_mem schedule_data_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 27 * 4, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, schedule_data_d, CL_TRUE, 0, 27 * 4, schedule_data_h, 0, NULL, NULL);

	unsigned char carnival_data[128] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
		0xF0, 0x90, 0x4F, 0x46, 0x9E, 0xD1, 0xD7, 0xF4
	};
	cl_mem carnival_data_d = clCreateBuffer(context, CL_MEM_READ_ONLY, 128, NULL, &ret);
	clEnqueueWriteBuffer(command_queue, carnival_data_d, CL_TRUE, 0, 128, carnival_data, 0, NULL, NULL);

	// Set fixed kernel args
	clSetKernelArg(kernel,  0, sizeof(cl_mem), &result_data_d);
	clSetKernelArg(kernel,  1, sizeof(cl_mem), &regular_rng_values_d);
	clSetKernelArg(kernel,  2, sizeof(cl_mem), &alg0_values_d);
	clSetKernelArg(kernel,  3, sizeof(cl_mem), &alg6_values_d);
	clSetKernelArg(kernel,  4, sizeof(cl_mem), &rng_seed_forward_1_d);
	clSetKernelArg(kernel,  5, sizeof(cl_mem), &rng_seed_forward_128_d);
	clSetKernelArg(kernel,  6, sizeof(cl_mem), &alg2_values_d);
	clSetKernelArg(kernel,  7, sizeof(cl_mem), &alg5_values_d);
	clSetKernelArg(kernel,  8, sizeof(cl_mem), &expansion_values_d);
	clSetKernelArg(kernel,  9, sizeof(cl_mem), &schedule_data_d);
	clSetKernelArg(kernel, 10, sizeof(cl_mem), &carnival_data_d);
	clSetKernelArg(kernel, 11, sizeof(cl_mem), &carnival_data_d);
	clSetKernelArg(kernel, 12, sizeof(uint32_t), &key);

	// -------------------------------------------------------------------
	// Main processing loop
	// -------------------------------------------------------------------
	std::vector<uint8_t> stats_buf(args.workunit_size, 0);
	uint8_t* result_data_h = new uint8_t[batch_cap];

	for (uint32_t pos = 0; pos < args.workunit_size; pos += BATCH_SIZE) {
		uint32_t chunk = args.workunit_size - pos;
		if (chunk > BATCH_SIZE) chunk = BATCH_SIZE;

		uint32_t data_start = args.range_start + pos;
		clSetKernelArg(kernel, 13, sizeof(uint32_t), &data_start);

		size_t global_item_size[3] = { 32, chunk, 1 };
		size_t local_item_size[3]  = { 32, 1,     1 };

		ret = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL,
		                             global_item_size, local_item_size, 0, NULL, NULL);
		if (ret != CL_SUCCESS) {
			fprintf(stderr, "Kernel launch failed at pos=%u: %d\n", pos, ret);
			return 1;
		}
		clFinish(command_queue);

		clEnqueueReadBuffer(command_queue, result_data_d, CL_TRUE, 0,
		                    chunk, result_data_h, 0, NULL, NULL);

		memcpy(&stats_buf[pos], result_data_h, chunk);

		printf("Progress: %u / %u\n", pos + chunk, args.workunit_size);
	}

	// -------------------------------------------------------------------
	// Collect results
	// -------------------------------------------------------------------
	// Strip CHECKSUM_SENTINEL (0x08) — kernel ORs it in to distinguish
	// "no hit" (0) from "hit with flags==0"; host strips it before output.
	static const uint8_t CHECKSUM_SENTINEL = 0x08;

	struct ResultEntry { uint32_t lsb; uint8_t flags; };
	std::vector<ResultEntry> results;
	for (uint32_t i = 0; i < args.workunit_size; i++) {
		if (stats_buf[i] != 0) {
			ResultEntry e;
			e.lsb   = args.range_start + i;
			e.flags = stats_buf[i] & ~CHECKSUM_SENTINEL;
			results.push_back(e);
		}
	}

	// -------------------------------------------------------------------
	// Build entry bytes and compute SHA256
	// -------------------------------------------------------------------
	size_t entry_bytes_size = results.size() * RESULT_ENTRY_SIZE;
	uint8_t* entry_bytes = new uint8_t[entry_bytes_size > 0 ? entry_bytes_size : 1];
	for (size_t i = 0; i < results.size(); i++) {
		memcpy(&entry_bytes[i * RESULT_ENTRY_SIZE], &results[i].lsb, 4);
		entry_bytes[i * RESULT_ENTRY_SIZE + 4] = results[i].flags;
	}

	uint8_t hash[SHA256_SIZE];
	{
		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, entry_bytes, entry_bytes_size);
		sha256_final(&ctx, hash);
	}

	char result_hash_hex[65];
	hex_encode(hash, 32, result_hash_hex);

	// -------------------------------------------------------------------
	// Challenge computation
	// Challenges are within [range_start, range_start + workunit_size),
	// so their stats are already in stats_buf.
	// -------------------------------------------------------------------
	struct Challenge { uint32_t lsb; uint8_t carnival_flags; uint8_t other_flags; };
	Challenge challenges[CHALLENGE_COUNT];

	std::string commit_input = std::string(result_hash_hex) + args.seed;
	uint8_t x[SHA256_SIZE];
	{
		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, (const uint8_t*)commit_input.data(), commit_input.size());
		sha256_final(&ctx, x);
	}

	for (int i = 0; i < CHALLENGE_COUNT; i++) {
		uint32_t x_be = ((uint32_t)x[0] << 24) | ((uint32_t)x[1] << 16)
		              | ((uint32_t)x[2] <<  8) |  (uint32_t)x[3];
		uint32_t offset = x_be % args.workunit_size;
		challenges[i].lsb = args.range_start + offset;
		{
			uint8_t f = stats_buf[offset] & ~CHECKSUM_SENTINEL;
			if (f & 0x01) { // OTHER_WORLD bit set
				challenges[i].carnival_flags = 0;
				challenges[i].other_flags    = f;
			} else {
				challenges[i].carnival_flags = f;
				challenges[i].other_flags    = 0;
			}
		}

		char x_hex[65];
		hex_encode(x, 32, x_hex);
		{
			SHA256_CTX ctx;
			sha256_init(&ctx);
			sha256_update(&ctx, (const uint8_t*)x_hex, 64);
			sha256_final(&ctx, x);
		}
	}

	// -------------------------------------------------------------------
	// Write output file
	// Layout: [uint32 count][N*5 entries][32 SHA256][100*6 challenges]
	// -------------------------------------------------------------------
	FILE* out_f = fopen("out.bin", "wb");
	if (out_f) {
		uint32_t count = (uint32_t)results.size();
		fwrite(&count,      4, 1, out_f);
		fwrite(entry_bytes, 1, entry_bytes_size, out_f);
		fwrite(hash,        1, SHA256_SIZE, out_f);
		for (int i = 0; i < CHALLENGE_COUNT; i++) {
			fwrite(&challenges[i].lsb,           4, 1, out_f);
			fwrite(&challenges[i].carnival_flags, 1, 1, out_f);
			fwrite(&challenges[i].other_flags,    1, 1, out_f);
		}
		fclose(out_f);
		printf("Results: %u hits. Output written to out.bin\n", (uint32_t)results.size());
	}

	// -------------------------------------------------------------------
	// Cleanup
	// -------------------------------------------------------------------
	delete[] entry_bytes;
	delete[] result_data_h;
	delete[] schedule_data_h;
	delete[] rng_table;
	delete[] regular_rng_values;
	delete[] alg0_values;
	delete[] alg6_values;
	delete[] rng_seed_forward_1;
	delete[] rng_seed_forward_128;
	delete[] alg2_values;
	delete[] alg5_values;
	delete[] expansion_values_h;

	clReleaseMemObject(result_data_d);
	clReleaseMemObject(regular_rng_values_d);
	clReleaseMemObject(alg0_values_d);
	clReleaseMemObject(alg6_values_d);
	clReleaseMemObject(rng_seed_forward_1_d);
	clReleaseMemObject(rng_seed_forward_128_d);
	clReleaseMemObject(alg2_values_d);
	clReleaseMemObject(alg5_values_d);
	clReleaseMemObject(expansion_values_d);
	clReleaseMemObject(schedule_data_d);
	clReleaseMemObject(carnival_data_d);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	delete[] platforms;
	delete[] devices;
	free(source_str);

	return 0;
}
