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

CFLAGS  = -ggdb3 -O3 -Wall -std=c++17 -I goldilocks/src -pthread -fopenmp -D__OVERLOAD_POLS__

# sudo apt install libeditreadline-dev
LDFLAGS = -lpthread -lgmp -lgmpxx -lomp -lreadline -L$(LIBOMP)
GOLDI_CPP = goldilocks/src/goldilocks_base_field.cpp

# The build target
PIL_SERVER = ../build/pilserver
PIL_VERIFY = ../build/pilverify

COMMON_SOURCES = block.cpp connection_map.cpp dependencies.cpp engine.cpp expression.cpp expressions.cpp operation.cpp operation_value.cpp public.cpp reference.cpp references.cpp tools.cpp
COMMON_INCLUDES = block.hpp connection_map.hpp cyclic.hpp debug.hpp dependencies.hpp engine.hpp expression.hpp expressions.hpp fr_element.hpp operation.hpp operation_value.hpp public.hpp reference.hpp references.hpp tools.hpp types.hpp

PIL_SERVER_SOURCES = $(COMMON_SOURCES) server.cpp service.cpp
PIL_SERVER_INCLUDES = $(COMMON_INCLUDES) service.hpp

PIL_VERIFY_SOURCES = $(COMMON_SOURCES) verify.cpp interactive.cpp
PIL_VERIFY_INCLUDES = $(COMMON_INCLUDES) interactive.hpp

all: $(PIL_VERIFY) $(PIL_SERVER)

$(PIL_VERIFY): $(PIL_VERIFY_SOURCES) $(PIL_VERIFY_INCLUDES)
	mkdir -p ../build
	$(CC) $(CFLAGS) -o $(PIL_VERIFY) -DPIL_VERIFY $(PIL_VERIFY_SOURCES) $(GOLDI_CPP) $(LDFLAGS) -lreadline

$(PIL_SERVER): $(PIL_SERVER_SOURCES) $(PIL_SERVER_INCLUDES)
	mkdir -p ../build
	$(CC) $(CFLAGS) -o $(PIL_SERVER) -I /usr/local/include/hv -DPIL_SERVER $(PIL_SERVER_SOURCES) $(GOLDI_CPP) $(LDFLAGS) -lhv

clean:
	$(RM) $(PIL_SERVER) $(PIL_VERIFY)