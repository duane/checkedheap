CXX=clang++ -Qunused-arguments
LIBNAME=libcheckedheap.dylib
INCLUDES=-Iinclude -IHeap-Layers
CXXFLAGS=-Wall -Wextra -Wno-unused -Wno-unused-parameter -Werror -g -O0  -mpreferred-stack-boundary=4 -msse2 -arch i386 -arch x86_64 $(INCLUDES) -D'CUSTOM_PREFIX(X)=xx\#\#x' -dynamiclib -D_REENTRANT=1 
LDFLAGS=-gstubs

all: libcheckedheap

libcheckedheap:
	$(CXX) -o $(LIBNAME) src/libcheckedheap.cpp Heap-Layers/wrappers/macwrapper.cpp $(CXXFLAGS) $(LDFLAGS)

test: 
	make -C test;
	test/run_all.sh

clean:
	rm -f $(LIBNAME)
	make clean -C test


.PHONY: test
