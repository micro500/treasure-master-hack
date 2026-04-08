#include <stdio.h>
#include <stdlib.h>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#define getcwd_impl(buf, sz) _getcwd(buf, sz)
#else
#include <unistd.h>
#define getcwd_impl(buf, sz) getcwd(buf, sz)
#endif

#include "rng_obj.h"
#include "key_schedule.h"
#include "opencl.h"
#include "tm_gpu_base.h"
#include "tm_gpu_tester.h"
#include "tester_gpu.h"
#include "gpu/tm_opencl_32x.h"
#include "gpu/tm_opencl_seq.h"

int main()
{
	char cwd[500];
	getcwd_impl(cwd, 500);
	fprintf(stderr, "CWD: %s\n", cwd);

	fprintf(stderr, "Initializing OpenCL...\n");
	opencl cl(0, 0);
	fprintf(stderr, "Platform:        %s\n", cl.get_platform_name());
	fprintf(stderr, "Device:          %s\n", cl.get_device_name());
	fprintf(stderr, "Platform version:%s\n", get_platform_param<char>(cl.platform_id, CL_PLATFORM_VERSION));
	fprintf(stderr, "Device version:  %s\n", get_device_param<char>(cl.device_id, CL_DEVICE_VERSION));
	fprintf(stderr, "Extensions:      %s\n", get_device_param<char>(cl.device_id, CL_DEVICE_EXTENSIONS));

	fprintf(stderr, "Building RNG tables...\n");
	RNG rng;

	fprintf(stderr, "Building GPU implementation...\n");
	std::vector<TM_GPU_base*> impls;
	//tm_opencl_32x impl0(&rng, &cl);
	//impls.push_back(&impl0);

	tm_opencl_seq impl1(&rng, &cl);
	impls.push_back(&impl1);
	fprintf(stderr, "Ready.\n");

	for (TM_GPU_base* impl : impls)
	{
		fprintf(stderr, "\n=== Testing: %s ===\n", impl->obj_name.c_str());
		tm_gpu_tester tester(impl);

		//run_expansion_validity_tests_gpu(tester);
		//run_alg_validity_tests_gpu(tester);
		//run_full_validity_tests_gpu(tester);
		//run_result_tests_gpu(tester);
		run_full_speed_test_gpu(tester);
	}

	return 0;
}
