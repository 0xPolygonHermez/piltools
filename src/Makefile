# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -g -Wall -Wno-unknown-pragmas -I goldilocks/src/
LDFLAGS  = -lgmp

# The build target
TARGET = main

all: $(TARGET)

#	$(CC) $(CFLAGS) -o $(TARGET) references.cpp reference.cpp main.cpp  goldilocks/src/goldilocks_base_field.cpp $(LDFLAGS)

$(TARGET): *.cpp *.hpp
	$(CC) $(CFLAGS) -o $(TARGET) *.cpp  goldilocks/src/goldilocks_base_field.cpp $(LDFLAGS)

clean:
	$(RM) $(TARGET)