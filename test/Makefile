
CC=clang
CXX=clang++

CFLAGS=-g -O0 -Wall -Wextra -Wno-unused -Wno-unused-parameter -Werror -I../DieHard/src/replicated
CXXFLAGS=$(CFLAGS)
LDFLAGS=-L.. -lcheckedheapstub -lpthread

PWD := $(shell pwd)
LIBDIR := $(shell cd ..; pwd)

OS := $(shell uname -s)
ifeq ($(OS), Linux)
LDFLAGS += -Wl,-rpath=$(LIBDIR)
endif

DEPS=check_heap.cpp danglingptr.cpp disaster.cpp doublefree.cpp illegalread.cpp malloc-verifier.cpp nheap.cpp test1.cpp test2.cpp test3.cpp test4.cpp testsocket.cpp 

all: check_heap danglingptr disaster doublefree illegalread malloc-verifier test1 test2 test3 test4 testsocket

check_heap: check_heap.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -I../include

danglingptr: danglingptr.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

disaster: disaster.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

doublefree: doublefree.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

illegalread: illegalread.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

malloc-verifier: malloc-verifier.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -lpthread

nheap: nheap.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -I../include -I../Heap-Layers

test1: test1.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test2: test2.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test3: test3.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

test4: test4.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

testsocket: testsocket.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -I../DieHard/src/replicated -lpthread

clean:
	rm -f check_heap danglingptr disaster doublefree illegalread malloc-verifier nheap test1 test2 test3 test4 testsocket
	rm -rf check_heap.dSYM/ danglingptr.dSYM/ disaster.dSYM/ doublefree.dSYM/ illegalread.dSYM/ malloc-verifier.dSYM/ nheap.dSYM/ test1.dSYM/ test2.dSYM/ test3.dSYM/ test4.dSYM/ testsocket.dSYM/ 
	rm -f test.log
