#!/bin/sh

if [ $# -ne 1 ]
then
	echo "usage: $0 test_trace"
	exit
fi

../../bin/peek/peek $1.in > $1.out
