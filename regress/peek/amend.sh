#!/bin/sh

if [ $# -eq 0 ]; then
	echo "usage: $0 test ..."
	exit
fi

for TEST in "$@"; do
	[ "$TEST" = "amend.sh" -o "$TEST" = "Makefile" ] && continue

	LINES=`grep "$TEST" Makefile`

	[ -z "$LINES" ] && echo "unknown $TEST" && continue

	set `echo "$LINES"`
	while : ; do
		case $# in
		[012]) echo "ambigous $TEST" && break;;
		esac;
		if [ ! "$1" = "$TEST" -o ! "$3" = "\\" ]; then
			shift; shift; shift
			continue
		fi
		../../bin/peek/peek ../../sample/$2 \
		    > $1 && echo "rewrote $1"
		break;
	done
done
