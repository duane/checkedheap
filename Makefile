INCLUDES=-Iinclude -IHeap-Layers
DEBUG ?= -g
UNOPT = -O0 $(DEBUG)
OPT = -O3 $(DEBUG) -DNDEBUG=1
CFLAGS=-Wall -Wextra -Wno-unused -Wno-unused-parameter -Werror -finline-functions -ffast-math -fomit-frame-pointer -D_REENTRANT=1 -pipe $(INCLUDES) -fPIC
CTHREADS=-DCHECK_HEAP_THREADED=1
LDTHREADS=-lpthread
CXXFLAGS=$(CFLAGS) -fno-rtti $(UNOPT)
OPTCXXFLAGS=$(CFLAGS) -fno-rtti $(OPT)
LDFLAGS=

SRCS = src/libcheckedheap.cpp
STUBSRCS = src/check_heap_stub.c

OS := $(shell uname -s)

ifeq ($(OS), Darwin)
CC=clang
CXX=clang++
LIBEXT=dylib
CXXFLAGS += -msse2 -arch i386 -arch x86_64 -D'CUSTOM_PREFIX(X)=xx\#\#x'
LDFLAGS += -dynamiclib
SRCS += Heap-Layers/wrappers/macwrapper.cpp
else ifeq ($(OS), Linux)
CC=clang
CXX=clang++
CXXFLAGS += -march=core2 -malign-double
CXXFLAGS += -Wno-deprecated-declarations # for __malloc_hook and friends
LDFLAGS += -shared -Bsymbolic -ldl
LIBEXT=so
SRCS += Heap-Layers/wrappers/gnuwrapper.cpp
endif

LIBPATH = libcheckedheap.$(LIBEXT)
LIBPATH_THREAD = libcheckedheap_thread.$(LIBEXT)
STUBLIBPATH = libcheckedheapstub.$(LIBEXT)

all: libcheckedheap libcheckedheap_thread libcheckheapstub

opt: libcheckedheapopt libcheckheapstub libcheckedheapopt_thread

libcheckedheapopt:
	$(CXX) -o $(LIBPATH) $(SRCS) $(OPTCXXFLAGS) $(LDFLAGS)

libcheckedheapopt_thread:
	$(CXX) -o $(LIBPATH_THREAD) $(SRCS) $(OPTCXXFLAGS) $(CTHREADS) $(LDFLAGS) $(LDTHREADS)

libcheckedheap:
	$(CXX) -o $(LIBPATH) $(SRCS)  $(CXXFLAGS) $(LDFLAGS)

libcheckedheap_thread:
	$(CXX) -o $(LIBPATH_THREAD) $(SRCS)  $(CXXFLAGS) $(CTHREADS) $(LDFLAGS)  $(LDTHREADS)

libcheckheapstub:
	$(CC) -o $(STUBLIBPATH) $(STUBSRCS) $(CFLAGS) $(LDFLAGS) $(UNOPT)

test: 
	make -C test;
	test/run_all.sh

clean:
	rm -f $(LIBPATH) $(STUBLIBPATH)
ifeq ($(OS), Darwin)
	rm -rf $(LIBPATH).dSYM/ $(STUBLIBPATH).dSYM/
endif
	make clean -C test

.PHONY: test
