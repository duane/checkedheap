CC=gcc
CXX=g++

CFLAGS=-g -O0 -Wall -Wextra -Wno-unused -Wno-unused-parameter -Werror -I../DieHard/src/replicated
CXXFLAGS=$(CFLAGS)
LDFLAGS=-L.. -lcheckedheapstub -lpthread

PWD := $(shell pwd)
LIBDIR := $(shell cd ..; pwd)
GTEST=$(PWD)/gtest-1.6.0

OS := $(shell uname -s)
ifeq ($(OS), Linux)
LDFLAGS += -Wl,-rpath=$(LIBDIR)
endif

DEPS=checkedheap.cpp check_heap.cpp danglingptr.cpp disaster.cpp doublefree.cpp illegalread.cpp malloc-test.cpp malloc-verifier.cpp nheap.cpp test1.cpp test2.cpp test3.cpp test4.cpp testsocket.cpp 

GTEST_SRCS=$(GTEST)/src/gtest_main.cc $(GTEST)/src/gtest-all.cc

all:checkedheap.cpp check_heap danglingptr disaster doublefree illegalread malloc-verifier test1 test2 test3 test4 testsocket

checkedheap: checkedheap.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -I../include -I../Heap-Layers -m32

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
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) -lpthread -I../include

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

gtest: malloc-test.cpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(GTEST_SRCS) -I$(GTEST) -I$(GTEST)/include -UGTEST_HAS_PTHREAD -fno-inline

clean:
	rm -f checkedheap check_heap danglingptr disaster doublefree illegalread malloc-verifier nheap test1 test2 test3 test4 testsocket
	rm -rf checkedheap.dSYM/ check_heap.dSYM/ danglingptr.dSYM/ disaster.dSYM/ doublefree.dSYM/ illegalread.dSYM/ malloc-verifier.dSYM/ nheap.dSYM/ test1.dSYM/ test2.dSYM/ test3.dSYM/ test4.dSYM/ testsocket.dSYM/ 
	rm -f test.log

.PHONY: checkedheap
