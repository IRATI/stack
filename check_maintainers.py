#!/usr/bin/env python

import os
import sys
import getopt
import re
import fnmatch

"""

@brief Check sanity of MAINTAINERS file


@author Marc Sune <marc.sune (at) bisdn.de>

"""

#Paths containing sources (must end with '/')
PATHS_TO_CHECK={"linux/net/rina/", "librina/src/", "librina/include/", "librina/test/",  "rinad/src/", "rina-tools/src/"}

#Only check files which we are interested in
#(exclude binaries and autotools scripts)
CHECK_EXTENSIONS={".h", ".c", ".cc", ".hpp", ".cpp", ".pc", ".py", ".proto", ".manifest"}

#File containing the relations between maintainers
MAINTAINERS_FILE="MAINTAINERS"

#Counter of errors
errors=0

def errormsg(msg):
	print("ERROR: "+msg)

def sanitize_path(p):
	if p[-1] == '/':
		return p+"*"
	return p

#Parse maintainers and paths
def parse_maintainers(f):
	parsed = dict()
	while True:
		l = f.readline()
		if not l: return parsed
		r = re.compile(r"^M: ([^\n]+)\n")
		m = r.search(l)
		if not m:
			continue
		#Add authors
		if m.group(1) not in parsed:
			parsed[m.group(1)] = {"f":list(), "x":list() }

		#Parse file and exclusions
		while True:
			l = f.readline()
			if not l: return parsed
			r = re.compile(r"^(F|X): ([^\n]+)\n")
			m2 = r.search(l)
			if not m2: break
			path = sanitize_path(m2.group(2))
			parsed[m.group(1)][m2.group(1).lower()].append(path)
	return parsed

def get_directory(p):
	return p.rsplit('/', 1)[0]

def check_extension(f):
	return os.path.splitext(f)[1] in CHECK_EXTENSIONS

def validate_maintainers(maintainers, fn):
	global errors
	num_maintainers=0
	c=list()
	for m, val in maintainers.items():
		for p in val["f"]:
			if fnmatch.fnmatch(fn, p):
				num_maintainers +=1
				c.append(m)
		for p in val["x"]:
			if fnmatch.fnmatch(fn, p):
				num_maintainers -=1

	if num_maintainers == 0:
		errormsg("unclaimed file: '%s'" % (fn))
		errors +=1
	elif num_maintainers > 1:
		msg="duplicated claim for file '%s' by\n" % (fn)
		for m in c:
			msg+="  "+str(m)+"\n"
		errormsg(msg)
		errors +=1
	#else:
	#	print "file '%s' claimed by '%s'" % (fn, c)

def scan_paths(maintainers, paths):
	for p in paths:
		for root, directories, filenames in os.walk(get_directory(p)):
			for f in filenames:
				name=os.path.join(root,f)
				if not check_extension(name):
					continue

				validate_maintainers(maintainers, name)
#Main routine
def main():
	try:
		f = open(MAINTAINERS_FILE)
	except:
		errormsg("Unable to open maintainers file '"+MAINTAINERS_FILE+"'")
	maintainers = parse_maintainers(f)
	scan_paths(maintainers, PATHS_TO_CHECK)
	if errors > 0:
		print("Number of errors: %s" % errors)
	sys.exit(errors)

if __name__ == "__main__":
    main()
