#!/bin/bash

OS=`uname -s`
SCRIPT=$0
PROG=$*

if [ "$PROG" = "" ]; then 
  echo "USAGE: $SCRIPT PROGRAM [ARG1 ARG2 ...]"
  echo "       executes \`PROGRAM ARG1 ARG2 ...\` in the diehard environment"
  echo ""
  exit 1
fi

RELTESTDIR=`dirname $0`
TESTDIR=`cd $RELTESTDIR; pwd`
LIBDIR=`cd $TESTDIR/..; pwd`

case $OS in
  "Darwin" )
    : ${LIBNAME:="libcheckedheap.dylib"}
    : ${LIB_PATH:="`dirname $TESTDIR`/$LIBNAME"}
    export DYLD_INSERT_LIBRARIES=$LIB_PATH;
    export DYLD_FORCE_FLAT_NAMESPACE=""
    ;;
  "Linux" )
    : ${LIBNAME:="libcheckedheap.so"}
    : ${LIB_PATH:="`dirname $TESTDIR`/$LIBNAME"}
    export LD_PRELOAD=$LIB_PATH
    ;;
esac

$PROG
exit
