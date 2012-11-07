INCLUDES=-Iinclude -IHeap-Layers
DEBUG ?= -g
OPT ?= -O0 $(DEBUG)
CFLAGS=-Wall -Wextra -Wno-unused -Wno-unused-parameter -Werror $(OPT) -finline-functions -ffast-math -fomit-frame-pointer -D_REENTRANT=1 -pipe $(INCLUDES) -fPIC
CXXFLAGS=$(CFLAGS) -fno-rtti
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
STUBLIBPATH = libcheckedheapstub.$(LIBEXT)

all: libcheckedheap libcheckheapstub

libcheckedheap:
	$(CXX) -o $(LIBPATH) $(SRCS)  $(CXXFLAGS) $(LDFLAGS)

libcheckheapstub:
	$(CC) -o $(STUBLIBPATH) $(STUBSRCS) $(CFLAGS) $(LDFLAGS)

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
