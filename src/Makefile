# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
# CFLAGS  = -g -O2 -Wall -fsanitize=leak -std=c++11 -Wno-unknown-pragmas -I goldilocks/src/
CFLAGS  = -g -O3 -Wall -std=c++11 -Wno-unknown-pragmas -I goldilocks/src/
LDFLAGS  = -lgmp

# The build target
TARGET = ../build/main

all: $(TARGET)

#	$(CC) $(CFLAGS) -o $(TARGET) references.cpp reference.cpp main.cpp  goldilocks/src/goldilocks_base_field.cpp $(LDFLAGS)

$(TARGET): *.cpp *.hpp
	$(CC) $(CFLAGS) -o $(TARGET) *.cpp  goldilocks/src/goldilocks_base_field.cpp $(LDFLAGS)

clean:
	$(RM) $(TARGET)