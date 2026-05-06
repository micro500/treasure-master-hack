#include <cuda_runtime.h>
#include "device_launch_parameters.h"
#include <cuda.h>

#include <iostream>
#include <fstream>

#include "rng_obj.h"

#ifndef checkCudaErrors
#define checkCudaErrors(err) __checkCudaErrors(err, __FILE__, __LINE__)

// These are the inline versions for all of the SDK helper functions
inline void __checkCudaErrors(CUresult err, const char* file, const int line) {
	if (CUDA_SUCCESS != err) {
		const char* errorStr = NULL;
		cuGetErrorString(err, &errorStr);
		fprintf(stderr,
			"checkCudaErrors() Driver API error = %04d \"%s\" from file <%s>, "
			"line %i.\n",
			err, errorStr, file, line);
		exit(EXIT_FAILURE);
	}
}
#endif

int main()
{
	cuInit(0);

	int device_count = 0;
	cuDeviceGetCount(&device_count);

	std::cout << device_count << std::endl;

	CUdevice cuDevice;
	cuDeviceGet(&cuDevice, 0);

	CUcontext cuContext;
	checkCudaErrors(cuCtxCreate(&cuContext, 0, cuDevice));

	//std::ifstream kernel_src_file("x64/Release/tm_cuda.cu.obj", std::ios::binary);
	std::ifstream kernel_src_file("tm_cuda.cu.obj", std::ios::binary);
	if (!kernel_src_file.is_open()) {
		std::cout << "Error opening file!" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	kernel_src_file.seekg(0, std::ios::end);
	size_t source_size = kernel_src_file.tellg();
	kernel_src_file.seekg(0, std::ios::beg);

	// Load the source code containing the kernel
	if (source_size > 100000)
	{
		source_size = 100000;
	}
	char* source_str = new char[source_size + 1];

	kernel_src_file.read(source_str, source_size);
	kernel_src_file.close();

	CUmodule cuModule;
	checkCudaErrors(cuModuleLoadData(&cuModule, source_str));

	CUfunction kernel;
	checkCudaErrors(cuModuleGetFunction(&kernel, cuModule, "expand_test"));

	RNG rng;
	rng.generate_expansion_values_8();

	// expand_test(uint32_t key, uint32_t data_start, uint8_t * expansion_values, uint8_t * result_storage)

	CUdeviceptr d_expansion_values;
	checkCudaErrors(cuMemAlloc(&d_expansion_values, 0x10000 * 128));
	checkCudaErrors(cuMemcpyHtoD(d_expansion_values, rng.expansion_values_8, 0x10000 * 128));

	CUdeviceptr d_result_storage;
	checkCudaErrors(cuMemAlloc(&d_result_storage, 0x100000));

	uint32_t key = 0x12345678;
	uint32_t data = 0x00000000;

	void* args[] = { &key, &data, &d_expansion_values, &d_result_storage };

	checkCudaErrors(cuLaunchKernel(kernel, 1, 1, 1, 32, 1, 1, 0, NULL, args, NULL));

	cuCtxSynchronize();

	uint8_t x[128];
	checkCudaErrors(cuMemcpyDtoH(x, d_result_storage, 128));

	cuCtxSynchronize();

	return 0;
}