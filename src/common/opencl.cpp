#include <iostream>
#include <fstream>
#include <string>
#include <CL/cl.h>

#include "opencl.h"

opencl::opencl(int platform_index, int device_index)
{
	cl_int ret;

	// Get available platform count
	cl_uint ret_num_platforms;
	ret = clGetPlatformIDs(0, NULL, &ret_num_platforms);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetPlatformIDs, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}
	//printf("Platforms found: %i\n", ret_num_platforms);

	if (platform_index >= ret_num_platforms)
	{
		std::cout << "Platform index out of range. Provided: " << platform_index << " Available: " << ret_num_platforms << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// Get platforms
	cl_platform_id* platforms = new cl_platform_id[ret_num_platforms];
	ret = clGetPlatformIDs(ret_num_platforms, platforms, &ret_num_platforms);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetPlatformIDs, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	platform_id = platforms[platform_index];
	delete[] platforms;

	// Get available device count
	cl_uint ret_num_devices;
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 0, NULL, &ret_num_devices);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetDeviceIDs, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}
	//printf("Devices found: %i\n", ret_num_devices);

	if (platform_index >= ret_num_platforms)
	{
		std::cout << "Device index out of range. Provided: " << device_index << " Available: " << ret_num_devices << std::endl;
		std::exit(EXIT_FAILURE);
	}

	cl_device_id* devices = new cl_device_id[ret_num_devices];

	// Get devices in platform
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, ret_num_devices, devices, nullptr);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetDeviceIDs, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	device_id = devices[device_index];
	delete[] devices;

	// Create OpenCL context
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clCreateContext, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	cl_command_queue_properties props[3] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };

	command_queue = clCreateCommandQueueWithProperties(context, device_id, props, &ret);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clCreateCommandQueue, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

char* opencl::get_platform_name()
{
	return get_platform_param<char>(platform_id, CL_PLATFORM_NAME);
}

char* opencl::get_platform_vendor()
{
	return get_platform_param<char>(platform_id, CL_PLATFORM_VENDOR);
}

char* opencl::get_device_name()
{
	return get_device_param<char>(device_id, CL_DEVICE_NAME);
}

cl_device_type* opencl::get_device_type()
{
	return get_device_param<cl_device_type>(device_id, CL_DEVICE_TYPE);
}

cl_uint* opencl::get_device_max_compute_units()
{
	return get_device_param<cl_uint>(device_id, CL_DEVICE_MAX_COMPUTE_UNITS);
}

cl_uint* opencl::get_device_max_work_item_dimensions()
{
	return get_device_param<cl_uint>(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
}

size_t* opencl::get_device_max_work_item_sizes()
{
	return get_device_param<size_t>(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES);
}

size_t* opencl::get_device_max_work_group_sizes()
{
	return get_device_param<size_t>(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE);
}

size_t* opencl::get_kernel_preferred_work_group_size_multiple(cl_kernel kernel)
{
	return get_kernel_param<size_t>(kernel, device_id, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE);
}

cl_program opencl::create_program(std::string filepath)
{
	std::ifstream kernel_src_file(filepath, std::ios::binary);
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

	source_str[source_size] = 0;

	cl_int ret;
	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t*)&source_size, &ret);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clCreateProgramWithSource, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return program;
}

void opencl::build_program(cl_program program)
{
	cl_int build_ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	size_t log_size;
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

	// Allocate memory for the log
	char* log = new char[log_size];

	// Get the log
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

	// Print the log
	std::cout << log << std::endl;

	if (build_ret != CL_SUCCESS)
	{
		std::cout << "Error in clBuildProgram, Ret: " << build_ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;

		std::exit(EXIT_FAILURE);
	}
}

cl_kernel opencl::create_kernel(cl_program program, std::string entry_name)
{
	cl_int ret;
	cl_kernel kernel = clCreateKernel(program, entry_name.c_str(), &ret);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clCreateKernel, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return kernel;
}

cl_mem opencl::create_buffer(size_t mem_size, cl_mem_flags flags)
{
	cl_mem mem_result = NULL;
	cl_int ret;
	mem_result = clCreateBuffer(context, flags, mem_size, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clCreateBuffer, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return mem_result;
}

cl_mem opencl::create_readwrite_buffer(size_t mem_size)
{
	return create_buffer(mem_size, CL_MEM_READ_WRITE);
}

cl_mem opencl::create_readonly_buffer(size_t mem_size)
{
	return create_buffer(mem_size, CL_MEM_READ_ONLY);
}

void opencl::copy_mem_to_device(cl_mem& device_mem, void* host_mem, size_t mem_size)
{
	cl_int ret = clEnqueueWriteBuffer(command_queue, device_mem, CL_TRUE, 0, mem_size, host_mem, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clEnqueueWriteBuffer, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void opencl::copy_mem_from_device(cl_mem& device_mem, void* host_mem, size_t mem_size)
{
	cl_int ret = clEnqueueReadBuffer(command_queue, device_mem, CL_TRUE, 0, mem_size, host_mem, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clEnqueueReadBuffer, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

cl_event opencl::run_kernel(cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size)
{
	cl_event event;
	cl_int ret = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_work_size, local_work_size, 0, NULL, &event);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clEnqueueNDRangeKernel, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return event;
}

std::string get_kernel_asm(cl_program program)
{
	size_t binary_size;
	clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);

	char* binary = new char[binary_size];
	clGetProgramInfo(program, CL_PROGRAM_BINARIES, binary_size, &binary, NULL);
	std::string binary_str(binary);
	return binary_str;

	delete[] binary;
}

void output_kernel_asm_to_file(cl_program program, std::string filename)
{
	std::string binary_str = get_kernel_asm(program);

	// Save or inspect the binary
	std::ofstream outfile(filename, std::ios::binary);
	outfile << binary_str;
	outfile.close();
}

void opencl::finish()
{
	clFinish(command_queue);
}