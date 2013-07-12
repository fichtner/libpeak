#!/usr/bin/env python

import sys

def peek_to_csv(line, header):
	select = 1
	csv = []

	if header:
		# show keywords header
		select = 0

	try:
		raw = line.rstrip().split(", ")
		for value in raw:
			csv.append(value.split(": ")[select])
	except:
		pass
	else:
		print ",".join(csv)

line = sys.stdin.readline()
peek_to_csv(line, 1)
peek_to_csv(line, 0)

while True:
	line = sys.stdin.readline()
	if not line:
		break
	peek_to_csv(line, 0)
