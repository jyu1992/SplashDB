CC = gcc
CXX = g++
CXXFLAGS = -Wall -std=c++11 -msse4.2 -g
LDFLAGS = -g
LDLIBS = -lstdc++ -lm

.PHONY: default
default: all

splash: splash_table.o

splash.o: splash_table.hpp

splash_table.o: splash_table.hpp

randomizer:

probe: splash_table.o

probe.o: splash_table.hpp

.PHONY: clean
clean:
	rm -f splash randomizer probe *.o

.PHONY: all
all: splash randomizer probe
