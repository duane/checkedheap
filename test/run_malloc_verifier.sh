#!/bin/bash

PROCS=4
ITERS=50
RELTESTDIR=`dirname $0`
TESTDIR=`cd $RELTESTDIR; pwd`

$TESTDIR/malloc-verifier $PROCS $ITERS
exit

