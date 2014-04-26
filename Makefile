CC = gcc
CXX = g++
CFLAGS = -Wall -std=c99 -msse4.2 -g
CXXFLAGS = -Wall -std=c++0x -msse4.2 -g
LDFLAGS = -g
LDLIBS = -lstdc++ -lm

.PHONY: default
default: all

splash: splash_table.o

splash.o: splash_table.hpp

splash_table.o: splash_table.hpp

randomizer:

probe:

.PHONY: clean
clean:
	rm -f splash randomizer probe *.o

.PHONY: all
all: splash randomizer probe
