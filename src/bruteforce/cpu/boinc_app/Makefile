CC     = g++
CC_C   = gcc
CFLAGS = -I../../common -I../cpu -I. -std=c++11 -O2 -m64 -mavx
CFLAGS_C = -I. -O2 -m64

all: tm_bruteforce

main.o:
	$(CC) $(CFLAGS) -c main.cpp

sha256.o:
	$(CC_C) $(CFLAGS_C) -c sha256.c

rng_obj.o:
	$(CC) $(CFLAGS) -c ../../common/rng_obj.cpp

key_schedule.o:
	$(CC) $(CFLAGS) -c ../../common/key_schedule.cpp

tm_base.o:
	$(CC) $(CFLAGS) -c ../../common/tm_base.cpp

alignment2.o:
	$(CC) $(CFLAGS) -c ../../common/alignment2.cpp

tm_avx_r128s_8.o:
	$(CC) $(CFLAGS) -c ../cpu/tm_avx_r128s_8.cpp

OBJS = main.o sha256.o rng_obj.o key_schedule.o tm_base.o alignment2.o tm_avx_r128s_8.o

tm_bruteforce: $(OBJS)
	$(CC) $(CFLAGS) -o tm_bruteforce $(OBJS)

clean:
	rm -f $(OBJS) tm_bruteforce
