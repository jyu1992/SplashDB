CC = gcc
CXX = g++
CXXFLAGS = -Wall -std=c++11 -g
LDFLAGS = -g
LDLIBS = -lstdc++ -lm

.PHONY: default
default: splash

splash: splash_table.o

splash.o: splash_table.hpp

splash_table.o: splash_table.hpp

randomizer:

.PHONY: clean
clean:
	rm -f splash randomizer *.o

.PHONY: all
all: splash randomizer
