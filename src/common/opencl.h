#pragma once
#include <CL/cl.h>

class opencl
{
public:
	opencl(int platform_index, int device_index);

	cl_program create_program(std::string filepath);
	cl_kernel create_kernel(cl_program program, std::string entry_name);
	void build_program(cl_program program);
	cl_event run_kernel(cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size);
	void finish();

	cl_mem create_buffer(size_t mem_size, cl_mem_flags flags);
	cl_mem create_readwrite_buffer(size_t mem_size);
	cl_mem create_readonly_buffer(size_t mem_size);
	void copy_mem_to_device(cl_mem& device_mem, void* host_mem, size_t mem_size);
	void copy_mem_from_device(cl_mem& device_mem, void* host_mem, size_t mem_size);

	char* get_platform_name();
	char* get_platform_vendor();
	char* get_device_name();
	cl_device_type* get_device_type();
	cl_uint* get_device_max_compute_units();
	cl_uint* get_device_max_work_item_dimensions();
	size_t* get_device_max_work_item_sizes();
	size_t* get_device_max_work_group_sizes();
	size_t* get_kernel_preferred_work_group_size_multiple(cl_kernel kernel);

	cl_platform_id platform_id;
	cl_device_id device_id;
	cl_context context;
	cl_command_queue command_queue;
};

std::string get_kernel_asm(cl_program program);
void output_kernel_asm_to_file(cl_program program, std::string filename);

template <typename T>
T* get_platform_param(cl_platform_id& platform_id, cl_platform_info param_name)
{
	cl_int ret;

	size_t ret_size;

	ret = clGetPlatformInfo(
		platform_id,
		param_name,
		NULL,
		NULL,
		&ret_size);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetPlatformInfo, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	T* val_ret = new T[ret_size];

	ret = clGetPlatformInfo(
		platform_id,
		param_name,
		ret_size,
		val_ret,
		&ret_size);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetPlatformInfo, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return val_ret;
}

template <typename T>
T* get_device_param(cl_device_id& device_id, cl_device_info param_name)
{
	cl_int ret;

	size_t ret_size;

	ret = clGetDeviceInfo(
		device_id,
		param_name,
		NULL,
		NULL,
		&ret_size);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetDeviceInfo, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	T* val_ret = new T[ret_size];

	ret = clGetDeviceInfo(
		device_id,
		param_name,
		ret_size,
		val_ret,
		&ret_size);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetDeviceInfo, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return val_ret;
}

template <typename T>
T* get_kernel_param(cl_kernel& kernel, cl_device_id& device, cl_device_info param_name)
{
	cl_int ret;

	size_t ret_size;

	ret = clGetKernelWorkGroupInfo(
		kernel,
		device,
		param_name,
		NULL,
		NULL,
		&ret_size);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetKernelWorkGroupInfo, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	T* val_ret = new T[ret_size];

	ret = clGetKernelWorkGroupInfo(
		kernel,
		device,
		param_name,
		ret_size,
		val_ret,
		&ret_size);
	if (ret != CL_SUCCESS)
	{
		std::cout << "Error in clGetKernelWorkGroupInfo, Ret: " << ret << "Line " << __LINE__ << " in file " << __FILE__ << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return val_ret;
}

template <typename T>
void set_kernel_arg(cl_kernel& kernel, cl_uint arg_index, void* arg)
{
	clSetKernelArg(kernel, arg_index, sizeof(T), arg);
}