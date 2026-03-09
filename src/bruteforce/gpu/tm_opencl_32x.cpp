#include <stdio.h>
#include <iostream>
#include "tm_opencl_32x.h"
#include "key_schedule.h"

tm_opencl_32x::tm_opencl_32x(RNG* rng_obj, opencl* _cl_init) : rng(rng_obj), _cl(_cl_init)
{
	initialize();
	_kernel_expand = NULL;
	_kernel_alg = NULL;

	_expansion_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_expansion_values_d, rng->expansion_values_8, 0x10000 * 128);

	_rng_seed_forward_1_d = _cl->create_readonly_buffer(0x10000 * 2);
	_cl->copy_mem_to_device(_rng_seed_forward_1_d, rng->seed_forward_1, 0x10000 * 2);

	_rng_seed_forward_128_d = _cl->create_readonly_buffer(0x10000 * 2);
	_cl->copy_mem_to_device(_rng_seed_forward_128_d, rng->seed_forward_128, 0x10000 * 2);

	_regular_rng_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_regular_rng_values_d, rng->regular_rng_values_8, 0x10000 * 128);

	_alg0_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_alg0_values_d, rng->alg0_values_8, 0x10000 * 128);

	_alg2_values_d = _cl->create_readonly_buffer(0x10000 * 4);
	_cl->copy_mem_to_device(_alg2_values_d, rng->alg2_values_32_8, 0x10000 * 4);

	_alg5_values_d = _cl->create_readonly_buffer(0x10000 * 4);
	_cl->copy_mem_to_device(_alg5_values_d, rng->alg5_values_32_8, 0x10000 * 4);

	_alg6_values_d = _cl->create_readonly_buffer(0x10000 * 128);
	_cl->copy_mem_to_device(_alg6_values_d, rng->alg6_values_8, 0x10000 * 128);
}

void tm_opencl_32x::initialize()
{
	if (!initialized)
	{
		rng->generate_expansion_values_8();

		rng->generate_seed_forward_1();
		rng->generate_seed_forward_128();

		rng->generate_regular_rng_values_8();
		rng->generate_alg0_values_8();
		
		rng->generate_alg2_values_32_8();
		rng->generate_alg5_values_32_8();
		
		rng->generate_alg6_values_8();

		_program = _cl->create_program("./bruteforce/opencl_test/tm.cl");
		_cl->build_program(_program);
		output_kernel_asm_to_file(_program, "kernel.ptx");

		initialized = true;
	}
	obj_name = "tm_opencl_32x";
}

void tm_opencl_32x::init_kernel_expand()
{
	_kernel_expand = _cl->create_kernel(_program, "expand_test");
}

void tm_opencl_32x::expand(uint32_t key, uint32_t data)
{
	if (_kernel_expand == NULL)
	{
		init_kernel_expand();
	}

	set_kernel_arg<uint32_t>(_kernel_expand, 0, &key);
	set_kernel_arg<uint32_t>(_kernel_expand, 1, &data);

	set_kernel_arg<cl_mem>(_kernel_expand, 2, &_expansion_values_d);

	cl_mem result_values_d = _cl->create_readwrite_buffer(128);
	set_kernel_arg<cl_mem>(_kernel_expand, 3, &result_values_d);

	size_t global_item_size[3] = { 32, 1, 1 };
	size_t local_item_size[3] = { 32, 1, 1 };

	cl_event event = _cl->run_kernel(_kernel_expand, 3, NULL, global_item_size, local_item_size);
	clWaitForEvents(1, &event);
	_cl->finish();

	_cl->copy_mem_from_device(result_values_d, working_code_single, 128);
}

void tm_opencl_32x::fetch_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		new_data[i] = working_code_single[i];
	}
}

void tm_opencl_32x::load_data(uint8* new_data)
{
	for (int i = 0; i < 128; i++)
	{
		working_code_single[i] = new_data[i];
	}
}

void tm_opencl_32x::init_kernel_alg()
{
	_kernel_alg = _cl->create_kernel(_program, "alg_test");
}

void tm_opencl_32x::run_alg(int algorithm_id, uint16* rng_seed, int iterations)
{
	if (_kernel_alg == NULL)
	{
		init_kernel_alg();
	}

	//(uint algorithm_id, uint rng_seed, __global uchar * input_data, __global uchar * result_storage)

	set_kernel_arg<uint32_t>(_kernel_alg, 0, &algorithm_id);
	set_kernel_arg<uint16_t>(_kernel_alg, 1, rng_seed);

	cl_mem input_values_d = _cl->create_readwrite_buffer(128);
	_cl->copy_mem_to_device(input_values_d, working_code_single, 128);
	set_kernel_arg<cl_mem>(_kernel_alg, 2, &input_values_d);

	cl_mem result_values_d = _cl->create_readwrite_buffer(128);
	set_kernel_arg<cl_mem>(_kernel_alg, 3, &result_values_d);

	set_kernel_arg<cl_mem>(_kernel_alg, 4, &_rng_seed_forward_1_d);
	set_kernel_arg<cl_mem>(_kernel_alg, 5, &_rng_seed_forward_128_d);
	set_kernel_arg<cl_mem>(_kernel_alg, 6, &_regular_rng_values_d);
	set_kernel_arg<cl_mem>(_kernel_alg, 7, &_alg0_values_d);
	set_kernel_arg<cl_mem>(_kernel_alg, 8, &_alg2_values_d);
	set_kernel_arg<cl_mem>(_kernel_alg, 9, &_alg5_values_d);
	set_kernel_arg<cl_mem>(_kernel_alg, 10, &_alg6_values_d);

	size_t global_item_size[3] = { 32, 1, 1 };
	size_t local_item_size[3] = { 32, 1, 1 };

	cl_event event = _cl->run_kernel(_kernel_alg, 3, NULL, global_item_size, local_item_size);
	clWaitForEvents(1, &event);
	_cl->finish();

	_cl->copy_mem_from_device(result_values_d, working_code_single, 128);
}

void tm_opencl_32x::init_kernel_all_maps()
{
	_kernel_all_maps = _cl->create_kernel(_program, "all_maps_test");
}

void tm_opencl_32x::run_all_maps(uint32_t key, uint32_t data, const key_schedule& schedule_entries)
{
	if (_kernel_all_maps == NULL)
	{
		init_kernel_all_maps();
	}

	// all_maps_test(uint key, uint data_start, __global unsigned char* schedule_data, __global uchar* result_storage, __global uchar* rng_seed_forward_1, __global uchar* rng_seed_forward_128, __global uchar* regular_rng_values, __global uchar* alg0_values, __global uchar* alg2_values, __global uchar* alg5_values, __global uchar* alg6_values)

	set_kernel_arg<uint32_t>(_kernel_all_maps, 0, &key);
	set_kernel_arg<uint32_t>(_kernel_all_maps, 1, &data);

	cl_mem schedule_entries_d = _cl->create_readwrite_buffer(27 * 4);
	_cl->copy_mem_to_device(schedule_entries_d, working_code_single, 27 * 4);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 2, &schedule_entries_d);

	cl_mem result_values_d = _cl->create_readwrite_buffer(128);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 3, &result_values_d);

	set_kernel_arg<cl_mem>(_kernel_all_maps, 4, &_expansion_values_d);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 5, &_rng_seed_forward_1_d);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 6, &_rng_seed_forward_128_d);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 7, &_regular_rng_values_d);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 8, &_alg0_values_d);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 9, &_alg2_values_d);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 10, &_alg5_values_d);
	set_kernel_arg<cl_mem>(_kernel_all_maps, 11, &_alg6_values_d);

	size_t global_item_size[3] = { 32, 0x10000000, 1 };
	size_t local_item_size[3] = { 32, 1, 1 };

	cl_event event = _cl->run_kernel(_kernel_all_maps, 3, NULL, global_item_size, local_item_size);
	clWaitForEvents(1, &event);
	_cl->finish();

	_cl->copy_mem_from_device(result_values_d, working_code_single, 128);


	cl_ulong time_start;
	cl_ulong time_end;

	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

	double nanoSeconds = time_end - time_start;
	printf("OpenCl Execution time is: %0.5f milliseconds \n", nanoSeconds / 1000000.0);
}

cl_program tm_opencl_32x::_program = NULL;
bool tm_opencl_32x::initialized = false;