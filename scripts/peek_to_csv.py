#!/usr/bin/env python

# Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

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
