
proc_test: process.o test.o
	g++ -o proc_test process.o test.o

test.o: src/test.cpp
	g++ -c -g -std=c++11 -I./include src/test.cpp

process.o: src/process.cpp include/process.h
	g++ -c -g -std=c++11 -I./include src/process.cpp
