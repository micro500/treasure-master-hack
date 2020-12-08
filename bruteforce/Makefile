CC = g++
CFLAGS = -Icommon -Icpu -std=c++11 -O2
AVXFLAGS = -mavx
AVX2FLAGS = -mavx2
all: cpu_test

tm_tester.o:
	$(CC) $(CFLAGS) -c common/tm_tester.cpp

key_schedule.o:
	$(CC) $(CFLAGS) -c common/key_schedule.cpp

rng_obj.o:
	$(CC) $(CFLAGS) -c common/rng_obj.cpp

alignment2.o:
	$(CC) $(CFLAGS) -c common/alignment2.cpp

tm_base.o:
	$(CC) $(CFLAGS) -c common/tm_base.cpp

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

tm_sse2_8_shuffled.o:
	$(CC) $(CFLAGS) -c cpu/tm_sse2_8_shuffled.cpp

test_cpu.o:
	$(CC) $(CFLAGS) -c test_cpu.cpp

cpu_test: tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o test_cpu.o
	$(CC) $(CFLAGS) -o cpu_test tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o test_cpu.o
  
clean:
	rm tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o test_cpu.o