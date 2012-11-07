#!/bin/bash
RELTESTDIR=`dirname $0`
TESTDIR=`cd $RELTESTDIR; pwd`
: ${TESTLOG:=${TESTDIR}/test.log}

GOOD="\033[32m"
BAD="\033[31m"
NORMAL="\033[0m"

for test in danglingptr disaster doublefree illegalread test1 test2 test4 testsocket
do
  echo -n "Running test $test... "
  $TESTDIR/run.sh $TESTDIR/$test &>"$TESTLOG"
  STATUS=$?
  if [ "$STATUS" = "0" ]; then
    echo -e "${GOOD}OK${NORMAL}"
  else
    echo -e "${BAD}FAIL:${NORMAL}"
    echo    "Exited with return code $STATUS."
    echo    "See log at \"$TESTLOG\" for detaits."
    break
  fi
done

