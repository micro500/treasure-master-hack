CXX      := g++
CC       := gcc
CXXFLAGS := -O2 -std=c++11 -I../../common -I. -I../boinc_app
CFLAGS   := -O2 -I../boinc_app
LDFLAGS  := -lOpenCL

TARGET := tm_opencl_32_8_test

SRCS_CXX := main.cpp key_schedule.cpp ../../common/rng.cpp
SRCS_C   := ../boinc_app/sha256.c

OBJS := $(SRCS_CXX:.cpp=.o) $(SRCS_C:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
