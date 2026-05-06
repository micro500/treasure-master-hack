CC = g++
CFLAGS = -I../../common -I. -Ismoke_test -Ibench -std=c++17 -O3 -m64 -Wall -flto
SSSE3FLAGS = -mssse3
AVXFLAGS = -mavx
AVX2FLAGS = -mavx2
AVX512FFLAGS = -mavx512f
AVX512BWFLAGS = -mavx512bw
AVX512VLFLAGS = -mavx512vl
all: cpu_test smoke_test bench

tm_tester.o: ../../common/tm_tester.cpp ../../common/tm_tester.h
	$(CC) $(CFLAGS) -c ../../common/tm_tester.cpp

key_schedule.o: ../../common/key_schedule.cpp ../../common/key_schedule.h
	$(CC) $(CFLAGS) -c ../../common/key_schedule.cpp

rng_obj.o: ../../common/rng_obj.cpp ../../common/rng_obj.h
	$(CC) $(CFLAGS) -c ../../common/rng_obj.cpp

alignment2.o: ../../common/alignment2.cpp ../../common/alignment2.h
	$(CC) $(CFLAGS) -c ../../common/alignment2.cpp

tm_base.o: ../../common/tm_base.cpp ../../common/tm_base.h
	$(CC) $(CFLAGS) -c ../../common/tm_base.cpp

tm_8.o: tm_8.cpp tm_8.h
	$(CC) $(CFLAGS) -c tm_8.cpp

tm_32_8.o: tm_32_8.cpp tm_32_8.h
	$(CC) $(CFLAGS) -c tm_32_8.cpp

tm_32_16.o: tm_32_16.cpp tm_32_16.h
	$(CC) $(CFLAGS) -c tm_32_16.cpp

tm_64_8.o: tm_64_8.cpp tm_64_8.h
	$(CC) $(CFLAGS) -c tm_64_8.cpp

tm_64_16.o: tm_64_16.cpp tm_64_16.h
	$(CC) $(CFLAGS) -c tm_64_16.cpp

tm_128_8.o: tm_128_8.cpp tm_128_8.h
	$(CC) $(CFLAGS) -c tm_128_8.cpp

tm_128_16.o: tm_128_16.cpp tm_128_16.h
	$(CC) $(CFLAGS) -c tm_128_16.cpp

tm_avx_8.o: tm_avx_8.cpp tm_avx_8.h
	$(CC) $(CFLAGS) $(AVXFLAGS) -c tm_avx_8.cpp

tm_avx_8_in_cpu.o: tm_avx_8_in_cpu.cpp tm_avx_8_in_cpu.h
	$(CC) $(CFLAGS) $(AVXFLAGS) -c tm_avx_8_in_cpu.cpp

tm_avx_8_in_cpu_shuffled.o: tm_avx_8_in_cpu_shuffled.cpp tm_avx_8_in_cpu_shuffled.h
	$(CC) $(CFLAGS) $(AVXFLAGS) -c tm_avx_8_in_cpu_shuffled.cpp

tm_avx_16.o: tm_avx_16.cpp tm_avx_16.h
	$(CC) $(CFLAGS) $(AVXFLAGS) -c tm_avx_16.cpp

tm_avx_in_cpu.o: tm_avx_in_cpu.cpp tm_avx_in_cpu.h
	$(CC) $(CFLAGS) $(AVXFLAGS) -c tm_avx_in_cpu.cpp

tm_avx2_8_in_cpu_shuffled.o: tm_avx2_8_in_cpu_shuffled.cpp tm_avx2_8_in_cpu_shuffled.h
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_8_in_cpu_shuffled.cpp

tm_avx512_8_in_cpu_shuffled.o: tm_avx512_8_in_cpu_shuffled.cpp tm_avx512_8_in_cpu_shuffled.h
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -c tm_avx512_8_in_cpu_shuffled.cpp

tm_sse2_8_shuffled.o: tm_sse2_8_shuffled.cpp tm_sse2_8_shuffled.h
	$(CC) $(CFLAGS) -c tm_sse2_8_shuffled.cpp

test_cpu.o: test_cpu.cpp
	$(CC) $(CFLAGS) -c test_cpu.cpp

cpu_test: tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o tm_avx512_8_in_cpu_shuffled.o test_cpu.o
	$(CC) $(CFLAGS) -o cpu_test tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o tm_avx512_8_in_cpu_shuffled.o test_cpu.o

smoke_test.o: smoke_test/smoke_test.cpp
	$(CC) $(CFLAGS) -c smoke_test/smoke_test.cpp

tm_ssse3_r128_8.o: tm_ssse3_r128_8.h tm_ssse3_r128_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_r128_8.cpp

tm_ssse3_r128s_8.o: tm_ssse3_r128s_8.h tm_ssse3_r128s_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_r128s_8.cpp

tm_ssse3_r128_map_8.o: tm_ssse3_r128_map_8.h tm_ssse3_r128_map_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_r128_map_8.cpp

tm_ssse3_r128s_map_8.o: tm_ssse3_r128s_map_8.h tm_ssse3_r128s_map_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_r128s_map_8.cpp

tm_ssse3_m128_8.o: tm_ssse3_m128_8.h tm_ssse3_m128_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_m128_8.cpp

tm_ssse3_m128s_8.o: tm_ssse3_m128s_8.h tm_ssse3_m128s_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_m128s_8.cpp

tm_ssse3_m128_map_8.o: tm_ssse3_m128_map_8.h tm_ssse3_m128_map_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_m128_map_8.cpp

tm_ssse3_m128s_map_8.o: tm_ssse3_m128s_map_8.h tm_ssse3_m128s_map_8.cpp
	$(CC) $(CFLAGS) $(SSSE3FLAGS) -mno-avx -c tm_ssse3_m128s_map_8.cpp

tm_avx_r128_8.o: tm_avx_r128_8.h tm_avx_r128_8.cpp
	$(CC) $(CFLAGS) $(AVXFLAGS) -mno-avx512f -c tm_avx_r128_8.cpp

tm_avx_r128s_8.o: tm_avx_r128s_8.h tm_avx_r128s_8.cpp
	$(CC) $(CFLAGS) $(AVXFLAGS) -mno-avx512f -c tm_avx_r128s_8.cpp

tm_avx_r128s_map_8.o: tm_avx_r128s_map_8.h tm_avx_r128s_map_8.cpp
	$(CC) $(CFLAGS) $(AVXFLAGS) -mno-avx512f -c tm_avx_r128s_map_8.cpp

tm_avx_r128_map_8.o: tm_avx_r128_map_8.h tm_avx_r128_map_8.cpp
	$(CC) $(CFLAGS) $(AVXFLAGS) -mno-avx512f -c tm_avx_r128_map_8.cpp

tm_avx_r256_map_8.o: tm_avx_r256_map_8.h tm_avx_r256_map_8.cpp
	$(CC) $(CFLAGS) $(AVXFLAGS) -c tm_avx_r256_map_8.cpp

tm_avx_m256_8.o: tm_avx_m256_8.h tm_avx_m256_8.cpp
	$(CC) $(CFLAGS) $(AVXFLAGS) -c tm_avx_m256_8.cpp

tm_avx2_r256_8.o: tm_avx2_r256_8.h tm_avx2_r256_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_r256_8.cpp

tm_avx2_r256s_8.o: tm_avx2_r256s_8.h tm_avx2_r256s_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_r256s_8.cpp

tm_avx2_r256_map_8.o: tm_avx2_r256_map_8.h tm_avx2_r256_map_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_r256_map_8.cpp

tm_avx2_r256s_map_8.o: tm_avx2_r256s_map_8.h tm_avx2_r256s_map_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_r256s_map_8.cpp

tm_avx2_m256_8.o: tm_avx2_m256_8.h tm_avx2_m256_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_m256_8.cpp

tm_avx2_m256s_8.o: tm_avx2_m256s_8.h tm_avx2_m256s_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_m256s_8.cpp

tm_avx2_m256_map_8.o: tm_avx2_m256_map_8.h tm_avx2_m256_map_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_m256_map_8.cpp

tm_avx2_m256s_map_8.o: tm_avx2_m256s_map_8.h tm_avx2_m256s_map_8.cpp
	$(CC) $(CFLAGS) $(AVX2FLAGS) -c tm_avx2_m256s_map_8.cpp

tm_avx512bw_r512_8.o: tm_avx512bw_r512_8.h tm_avx512bw_r512_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -c tm_avx512bw_r512_8.cpp

tm_avx512bw_r512s_8.o: tm_avx512bw_r512s_8.h tm_avx512bw_r512s_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -c tm_avx512bw_r512s_8.cpp

tm_avx512bw_r512_map_8.o: tm_avx512bw_r512_map_8.h tm_avx512bw_r512_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -c tm_avx512bw_r512_map_8.cpp

tm_avx512bw_r512s_map_8.o: tm_avx512bw_r512s_map_8.h tm_avx512bw_r512s_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -c tm_avx512bw_r512s_map_8.cpp

tm_avx512bwvl_r512_8.o: tm_avx512bwvl_r512_8.h tm_avx512bwvl_r512_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -mavx512vl -c tm_avx512bwvl_r512_8.cpp

tm_avx512bwvl_r512s_8.o: tm_avx512bwvl_r512s_8.h tm_avx512bwvl_r512s_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -mavx512vl -c tm_avx512bwvl_r512s_8.cpp

tm_avx512bwvl_r512_map_8.o: tm_avx512bwvl_r512_map_8.h tm_avx512bwvl_r512_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -mavx512vl -c tm_avx512bwvl_r512_map_8.cpp

tm_avx512bwvl_r512s_map_8.o: tm_avx512bwvl_r512s_map_8.h tm_avx512bwvl_r512s_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512BWFLAGS) -mavx512vl -c tm_avx512bwvl_r512s_map_8.cpp

tm_avx512vl_r128_8.o: tm_avx512vl_r128_8.h tm_avx512vl_r128_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r128_8.cpp

tm_avx512vl_r128s_8.o: tm_avx512vl_r128s_8.h tm_avx512vl_r128s_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r128s_8.cpp

tm_avx512vl_r128_map_8.o: tm_avx512vl_r128_map_8.h tm_avx512vl_r128_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r128_map_8.cpp

tm_avx512vl_r128s_map_8.o: tm_avx512vl_r128s_map_8.h tm_avx512vl_r128s_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r128s_map_8.cpp

tm_avx512vl_r256_8.o: tm_avx512vl_r256_8.h tm_avx512vl_r256_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r256_8.cpp

tm_avx512vl_r256s_8.o: tm_avx512vl_r256s_8.h tm_avx512vl_r256s_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r256s_8.cpp

tm_avx512vl_r256_map_8.o: tm_avx512vl_r256_map_8.h tm_avx512vl_r256_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r256_map_8.cpp

tm_avx512vl_r256s_map_8.o: tm_avx512vl_r256s_map_8.h tm_avx512vl_r256s_map_8.cpp
	$(CC) $(CFLAGS) $(AVX512FFLAGS) $(AVX512VLFLAGS) -c tm_avx512vl_r256s_map_8.cpp

smoke_test: key_schedule.o rng_obj.o alignment2.o tm_base.o tm_avx_r128_8.o tm_avx_r128s_8.o tm_avx_r128s_map_8.o tm_avx_r128_map_8.o tm_avx_r256_map_8.o tm_avx_m256_8.o tm_avx2_r256_8.o tm_avx2_r256s_8.o tm_avx2_r256_map_8.o tm_avx2_r256s_map_8.o tm_avx2_m256_8.o tm_avx2_m256s_8.o tm_avx2_m256_map_8.o tm_avx2_m256s_map_8.o tm_8.o tm_32_8.o tm_64_8.o smoke_test.o tm_avx512bw_r512_8.o tm_avx512bw_r512s_8.o tm_avx512bw_r512_map_8.o tm_avx512bw_r512s_map_8.o tm_avx512vl_r128_8.o tm_avx512vl_r128s_8.o tm_avx512vl_r128_map_8.o tm_avx512vl_r128s_map_8.o tm_avx512vl_r256_8.o tm_avx512vl_r256s_8.o tm_avx512vl_r256_map_8.o tm_avx512vl_r256s_map_8.o tm_avx512bwvl_r512_8.o tm_avx512bwvl_r512s_8.o tm_avx512bwvl_r512_map_8.o tm_avx512bwvl_r512s_map_8.o tm_ssse3_r128_8.o tm_ssse3_r128s_8.o tm_ssse3_r128_map_8.o tm_ssse3_r128s_map_8.o tm_ssse3_m128_8.o tm_ssse3_m128s_8.o tm_ssse3_m128_map_8.o tm_ssse3_m128s_map_8.o smoke_test/smoke_test.cpp
	$(CC) $(CFLAGS) -o smoke_test_out key_schedule.o rng_obj.o alignment2.o tm_base.o tm_avx_r128_8.o tm_avx_r128s_8.o tm_avx_r128s_map_8.o tm_avx_r128_map_8.o tm_avx_r256_map_8.o tm_avx_m256_8.o tm_avx2_r256_8.o tm_avx2_r256s_8.o tm_avx2_r256_map_8.o tm_avx2_r256s_map_8.o tm_avx2_m256_8.o tm_avx2_m256s_8.o tm_avx2_m256_map_8.o tm_avx2_m256s_map_8.o tm_8.o tm_32_8.o tm_64_8.o tm_avx512bw_r512_8.o tm_avx512bw_r512s_8.o tm_avx512bw_r512_map_8.o tm_avx512bw_r512s_map_8.o tm_avx512vl_r128_8.o tm_avx512vl_r128s_8.o tm_avx512vl_r128_map_8.o tm_avx512vl_r128s_map_8.o tm_avx512vl_r256_8.o tm_avx512vl_r256s_8.o tm_avx512vl_r256_map_8.o tm_avx512vl_r256s_map_8.o tm_avx512bwvl_r512_8.o tm_avx512bwvl_r512s_8.o tm_avx512bwvl_r512_map_8.o tm_avx512bwvl_r512s_map_8.o tm_ssse3_r128_8.o tm_ssse3_r128s_8.o tm_ssse3_r128_map_8.o tm_ssse3_r128s_map_8.o tm_ssse3_m128_8.o tm_ssse3_m128s_8.o tm_ssse3_m128_map_8.o tm_ssse3_m128s_map_8.o smoke_test.o -lgtest -lgtest_main -pthread

bench.o: bench/bench.cpp
	$(CC) $(CFLAGS) -c bench/bench.cpp

tm_cpu_factory.o: tm_cpu_factory.cpp tm_cpu_factory.h
	$(CC) $(CFLAGS) -c tm_cpu_factory.cpp

BENCH_OBJS = key_schedule.o rng_obj.o alignment2.o tm_base.o \
	tm_8.o tm_32_8.o tm_64_8.o \
	tm_ssse3_r128_8.o tm_ssse3_r128s_8.o tm_ssse3_r128_map_8.o tm_ssse3_r128s_map_8.o \
	tm_ssse3_m128_8.o tm_ssse3_m128s_8.o tm_ssse3_m128_map_8.o tm_ssse3_m128s_map_8.o \
	tm_avx_r128_8.o tm_avx_r128s_8.o tm_avx_r128_map_8.o tm_avx_r128s_map_8.o tm_avx_r256_map_8.o \
	tm_avx2_r256_8.o tm_avx2_r256s_8.o tm_avx2_r256_map_8.o tm_avx2_r256s_map_8.o \
	tm_avx2_m256_8.o tm_avx2_m256s_8.o tm_avx2_m256_map_8.o tm_avx2_m256s_map_8.o \
	tm_avx512bw_r512_8.o tm_avx512bw_r512s_8.o tm_avx512bw_r512_map_8.o tm_avx512bw_r512s_map_8.o \
	tm_avx512vl_r128_8.o tm_avx512vl_r128s_8.o tm_avx512vl_r128_map_8.o tm_avx512vl_r128s_map_8.o \
	tm_avx512vl_r256_8.o tm_avx512vl_r256s_8.o tm_avx512vl_r256_map_8.o tm_avx512vl_r256s_map_8.o \
	tm_avx512bwvl_r512_8.o tm_avx512bwvl_r512s_8.o tm_avx512bwvl_r512_map_8.o tm_avx512bwvl_r512s_map_8.o \
	tm_cpu_factory.o bench.o

bench: $(BENCH_OBJS)
	$(CC) $(CFLAGS) -o bench_out $(BENCH_OBJS) -lpthread

clean:
	rm -f tm_tester.o key_schedule.o rng_obj.o alignment2.o tm_base.o tm_8.o tm_32_8.o tm_32_16.o tm_64_8.o tm_64_16.o tm_128_8.o tm_128_16.o tm_avx_8.o tm_avx_8_in_cpu.o tm_avx_8_in_cpu_shuffled.o tm_avx_16.o tm_avx_in_cpu.o tm_sse2_8_shuffled.o tm_avx2_8_in_cpu_shuffled.o tm_avx512_8_in_cpu_shuffled.o tm_avx_r128_8.o tm_avx_r128s_8.o tm_avx_r128s_map_8.o tm_avx_r128_map_8.o tm_avx_r256_map_8.o tm_avx_m256_8.o tm_avx512bw_r512_8.o tm_avx512bw_r512s_8.o tm_avx512bw_r512_map_8.o tm_avx512bw_r512s_map_8.o tm_avx512bwvl_r512_8.o tm_avx512bwvl_r512s_8.o tm_avx512bwvl_r512_map_8.o tm_avx512bwvl_r512s_map_8.o tm_avx512_r128s_8.o tm_avx512vl_r128_8.o tm_avx512vl_r128s_8.o tm_avx512vl_r128_map_8.o tm_avx512vl_r128s_map_8.o tm_avx512vl_r256_8.o tm_avx512vl_r256s_8.o tm_avx512vl_r256_map_8.o tm_avx512vl_r256s_map_8.o test_cpu.o smoke_test.o bench.o bench_out smoke_test_out
