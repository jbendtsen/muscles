#!/bin/python

import os
import sys

libs = ["SDL2", "freetype", "pthread"]
includes = ["/usr/include/SDL2", "/usr/include/freetype2", "./Muscles"]
dirs = ["./Muscles", "./Muscles/dialog"]
excludes = ["io-win32.cpp"]

cpp_list = []
for d in dirs:
	l = os.listdir(d)
	for f in l:
		if f[-4:] != ".cpp":
			continue

		skip = False
		for e in excludes:
			if e == f:
				skip = True
				break

		if skip:
			continue

		cpp_list.append(d + "/" + f)

command = "g++ -g"
for l in libs:
	command += " -l" + l
for i in includes:
	command += " -I" + i
for c in cpp_list:
	command += " " + c

command += " -o muscles-linux"

print(command)
res = os.system(command)
sys.exit(0 if res == 0 else 1)
