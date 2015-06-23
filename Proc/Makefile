
proc_test: process.o test.o
	g++ -Wall -o proc_test process.o test.o -lboost_filesystem -lboost_system

test.o: src/test.cpp
	g++ -c -g -std=c++11 -Wall -I./include src/test.cpp

process.o: src/process.cpp include/process.h
	g++ -c -g -std=c++11 -Wall -I./include src/process.cpp

clean:
	rm -f *.o proc_test
