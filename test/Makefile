

CC=clang
CXX=clang++

CFLAGS=-g -O0 -Wall -Wextra -Wno-unused -Wno-unused-parameter -Werror
CXXFLAGS=$(CFLAGS) -gstabs
LDFLAGS=check_heap.o

DEPS=danglingptr.cpp disaster.cpp doublefree.cpp illegalread.cpp malloc-verifier.cpp test1.cpp test2.cpp test3.cpp test4.cpp testsocket.cpp 

all: check_heap_test danglingptr disaster doublefree illegalread malloc-verifier test1 test2 test3 test4 testsocket

check_heap.o:
	$(CC) -c check_heap.c -o $@ $< $(CFLAGS)

check_heap_test: check_heap_test.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -I../include

danglingptr: danglingptr.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

disaster: disaster.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

doublefree: doublefree.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

illegalread: illegalread.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

malloc-verifier: malloc-verifier.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test1: test1.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test2: test2.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test3: test3.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test4: test4.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

testsocket: testsocket.cpp check_heap.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -I../diehard/src/replicated # for socket.h

clean:
	rm -f check_heap_test danglingptr disaster doublefree illegalread malloc-verifier test1 test2 test3 test4 testsocket check_heap.o
