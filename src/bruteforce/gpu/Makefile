CC = g++
CFLAGS = -I../../common -I. -std=c++17 -O3 -m64 -Wall -flto
OPENCL_CFLAGS = -I/usr/local/cuda/include
OPENCL_LDFLAGS = -L/usr/local/cuda/lib64 -lOpenCL

all: test_gpu

key_schedule.o: ../../common/key_schedule.cpp ../../common/key_schedule.h
	$(CC) $(CFLAGS) -c ../../common/key_schedule.cpp

rng_obj.o: ../../common/rng_obj.cpp ../../common/rng_obj.h
	$(CC) $(CFLAGS) -c ../../common/rng_obj.cpp

alignment2.o: ../../common/alignment2.cpp ../../common/alignment2.h
	$(CC) $(CFLAGS) -c ../../common/alignment2.cpp

tm_base.o: ../../common/tm_base.cpp ../../common/tm_base.h
	$(CC) $(CFLAGS) -c ../../common/tm_base.cpp

opencl.o: ../../common/opencl.cpp ../../common/opencl.h
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c ../../common/opencl.cpp

tm_opencl_seq.o: tm_opencl_seq.cpp tm_opencl_seq.h
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c tm_opencl_seq.cpp

tm_opencl_32x.o: tm_opencl_32x.cpp tm_opencl_32x.h
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c tm_opencl_32x.cpp

test_gpu.o: test_gpu/test_gpu.cpp
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c test_gpu/test_gpu.cpp

test_gpu: key_schedule.o rng_obj.o alignment2.o tm_base.o opencl.o tm_opencl_seq.o tm_opencl_32x.o test_gpu.o
	$(CC) $(CFLAGS) -o test_gpu key_schedule.o rng_obj.o alignment2.o tm_base.o opencl.o tm_opencl_seq.o tm_opencl_32x.o test_gpu.o $(OPENCL_LDFLAGS)
	cp opencl_test/tm_seq.cl tm_seq.cl

clean:
	rm -f key_schedule.o rng_obj.o alignment2.o tm_base.o opencl.o tm_opencl_seq.o tm_opencl_32x.o test_gpu.o test_gpu tm_seq.cl
