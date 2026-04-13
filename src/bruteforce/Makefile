CC = g++
CFLAGS = -I../common -Icpu -Igpu -std=c++17 -O2 -m64
AVXFLAGS = -mavx
AVX2FLAGS = -mavx2
AVX512FFLAGS = -mavx512f
AVX512BWFLAGS = -mavx512bw
OPENCL_CFLAGS = -I/usr/local/cuda/include
OPENCL_LDFLAGS = -L/usr/local/cuda/lib64 -lOpenCL
all: cpu_test test_gpu

tm_tester.o:
	$(CC) $(CFLAGS) -c ../common/tm_tester.cpp

key_schedule.o:
	$(CC) $(CFLAGS) -c ../common/key_schedule.cpp

rng_obj.o:
	$(CC) $(CFLAGS) -c ../common/rng_obj.cpp

alignment2.o:
	$(CC) $(CFLAGS) -c ../common/alignment2.cpp

tm_base.o:
	$(CC) $(CFLAGS) -c ../common/tm_base.cpp

tm_8.o:
	$(CC) $(CFLAGS) -c cpu/tm_8.cpp

tm_32_8.o:
	$(CC) $(CFLAGS) -c cpu/tm_32_8.cpp

tm_32_16.o:
	$(CC) $(CFLAGS) -c cpu/tm_32_16.cpp

tm_64_8.o:
	$(CC) $(CFLAGS) -c cpu/tm_64_8.cpp

tm_64_16.o:
	$(CC) $(CFLAGS) -c cpu/tm_64_16.cpp

tm_128_8.o:
	$(CC) $(CFLAGS) -c cpu/tm_128_8.cpp

tm_128_16.o:
	$(CC) $(CFLAGS) -c cpu/tm_128_16.cpp

tm_avx_8.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_8.cpp

tm_avx_8_in_cpu.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_8_in_cpu.cpp

tm_avx_8_in_cpu_shuffled.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_8_in_cpu_shuffled.cpp

tm_avx_16.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_16.cpp

tm_avx_in_cpu.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_in_cpu.cpp

tm_avx2_8_in_cpu_shuffled.o:
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c cpu/tm_avx2_8_in_cpu_shuffled.cpp

tm_avx512_8_in_cpu_shuffled.o:
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -c cpu/tm_avx512_8_in_cpu_shuffled.cpp

tm_sse2_8_shuffled.o:
	$(CC) $(CFLAGS) -c cpu/tm_sse2_8_shuffled.cpp

test_cpu.o:
	$(CC) $(CFLAGS) -c test_cpu.cpp

cpu_test: tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o tm_avx512_8_in_cpu_shuffled.o test_cpu.o
	$(CC) $(CFLAGS) -o cpu_test tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o tm_avx512_8_in_cpu_shuffled.o test_cpu.o

opencl.o:
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c ../common/opencl.cpp

tm_opencl_seq.o:
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c gpu/tm_opencl_seq.cpp

test_gpu.o:
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c test_gpu.cpp

test_gpu: key_schedule.o rng_obj.o alignment2.o tm_base.o opencl.o tm_opencl_seq.o test_gpu.o
	$(CC) $(CFLAGS) -o test_gpu key_schedule.o rng_obj.o alignment2.o tm_base.o opencl.o tm_opencl_seq.o test_gpu.o $(OPENCL_LDFLAGS)
	cp opencl_test/tm_seq.cl tm_seq.cl

classes_test.o:
	$(CC) $(CFLAGS) -c test_classes.cpp

tm_avx_r128_8.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_r128_8.cpp

tm_avx_r128s_8.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_r128s_8.cpp

tm_avx_r128s_map_8.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_r128s_map_8.cpp

tm_avx_r128_map_8.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_r128_map_8.cpp

tm_avx_r256_map_8.o:
	$(CC) $(CFLAGS) $(AVXFLAGS) -c cpu/tm_avx_r256_map_8.cpp

classes_test: key_schedule.o rng_obj.o alignment2.o tm_base.o tm_avx_r128_8.o tm_avx_r128s_8.o tm_avx_r128s_map_8.o tm_avx_r128_map_8.o tm_avx_r256_map_8.o tm_8.o classes_test.o
	$(CC) $(CFLAGS) -lgtest -lgtest_main -o classes_test tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_avx_r128_8.o tm_avx_r128s_8.o tm_avx_r128s_map_8.o tm_avx_r128_map_8.o tm_avx_r256_map_8.o tm_8.o classes_test.o

clean:
	rm -f tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o tm_avx512_8_in_cpu_shuffled.o test_cpu.o opencl.o tm_opencl_seq.o test_gpu.o classes_test.o
