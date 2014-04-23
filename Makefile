CC = gcc
CXX = g++
CXXFLAGS = -Wall -std=c++11 -g
LDFLAGS = -g
LDLIBS = -lstdc++

.PHONY: default
default: splash

splash: splash_table.o

splash.o: splash_table.hpp

splash_table.o: splash_table.hpp

.PHONY: clean
clean:
	rm -f splash *.o

.PHONY: all
all: clean default
