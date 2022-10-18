# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
# CFLAGS  = -g -O2 -H  -Wall -fsanitize=leak -std=c++11 -Wno-unknown-pragmas -I goldilocks/src/

LIBOMP := $(shell find /usr/lib/llvm-* -name "libomp.so" | sed 's/libomp.so//')
ifndef LIBOMP
$(error LIBOMP is not set, you need to install libomp-dev)
endif

#CXX := mpiCC
#LDFLAGS := -lpthread -lgmp -lstdc++ -lomp -lgmpxx -lbenchmark -L$(LIBOMP) -Wno-unknown-pragmas

CFLAGS  = -ggdb3 -O3 -Wall -std=c++17 -I goldilocks/src -pthread -fopenmp -D__OVERLOAD_POLS__
LDFLAGS  = -lpthread -lgmp -lgmpxx -lomp -L$(LIBOMP)

# The build target
TARGET = ../build/main

all: $(TARGET)

#	$(CC) $(CFLAGS) -o $(TARGET) references.cpp reference.cpp main.cpp  goldilocks/src/goldilocks_base_field.cpp $(LDFLAGS)

$(TARGET): *.cpp *.hpp token/*.cpp token/*.hpp
#	$(CC) $(CFLAGS) -o $(TARGET) engine.cpp expression.cpp dependencies.cpp operation_value.cpp operation.cpp public.cpp reference.cpp references.cpp goldilocks/src/goldilocks_base_field.cpp $(LDFLAGS)
	$(CC) $(CFLAGS) -o $(TARGET) *.cpp token/*.cpp goldilocks/src/goldilocks_base_field.cpp $(LDFLAGS)

clean:
	$(RM) $(TARGET)