#!/usr/bin/env python

import os
import sys
import getopt
import re
import subprocess
import fnmatch
from argparse import ArgumentParser, RawTextHelpFormatter

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
	if f == MAINTAINERS_FILE:
		return True
	return os.path.splitext(f)[1] in CHECK_EXTENSIONS

def get_maintainers(maintainers, fn):
	c=list()
	num_maintainers=0
	for m, val in maintainers.items():
		for p in val["f"]:
			if fnmatch.fnmatch(fn, p):
				num_maintainers +=1
				c.append(m)
		for p in val["x"]:
			if fnmatch.fnmatch(fn, p):
				num_maintainers -=1
	return c, num_maintainers

def validate_maintainers(maintainers, fn):
	global errors
	c, num_maintainers = get_maintainers(maintainers, fn)

	if num_maintainers == 0:
		errormsg("unclaimed file '%s'" % (fn))
		errors +=1
	elif num_maintainers > 1:
		msg="duplicated claim for file '%s' by\n" % (fn)
		for m in c:
			msg+="  "+str(m)+"\n"
		errormsg(msg)
		errors +=1
	#else:
	#	print("file '%s' claimed by '%s'" % (fn, c))

def scan_paths(maintainers, paths):
	for p in paths:
		for root, directories, filenames in os.walk(get_directory(p)):
			for f in filenames:
				name=os.path.join(root,f)
				if not check_extension(name):
					continue

				validate_maintainers(maintainers, name)

def list_maintainers(maintainers, opt, grouped):

	mfiles = dict()
	try:
		int(opt)
		is_int = True
	except:
		is_int = False

	if os.path.isfile(opt):
		cmd = "git apply --index --numstat "+opt+" | cut -f 3-"
		try:
			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
		except Exception as e:
			errormsg("Unable to parse patch file '"+opt+"'. Error"+str(e))
			sys.exit(1)
	elif is_int:
		cmd = "git diff HEAD~"+opt+" --name-only"
		try:
			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
		except Exception as e:
			errormsg("Unable to retrieve files for the last '"+opt+"' commits. Error"+str(e))
			sys.exit(1)

	else:
		cmd = "git diff "+opt+" --name-only"
		try:
			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
		except Exception as e:
			errormsg("Unable to retrieve files for the last '"+opt+"' commits. Error"+str(e))
			sys.exit(1)

	for f_ in p.stdout.read().split():
		f_ = f_.decode('utf-8')
		ms, n = get_maintainers(maintainers, f_)
		if not check_extension(f_):
			continue
		if n == 0:
			continue

		if not grouped:
			print("F: "+f_)
			for m in ms:
				print("\t"+m)
		else:
			for m in ms:
				if m not in mfiles:
					mfiles[m] = list()
				mfiles[m].append(f_)
	if grouped:
		for m in mfiles:
			print("M: "+m)
			for f in mfiles[m]:
				print ("\t"+f)


#Main routine
def main():
	#Parse arguments
	parser = ArgumentParser(description=" %s - check maintainers script.\n\n"
				"Check sanity of the MAINTAINERS file and list maintainers for a patch/series." % (__file__),
				epilog="Examples:\n"
				" %s \t\t\t\t-- check MAINTAINERS file sanity and against the current tree\n"
				" %s -n 0001-feature.patch\t-- list *only* the maintainers of the files affected by the patch\n"
				" %s -n -l 20\t\t\t-- list *only* the maintainers of the files affected by the last 20 patches (git diff HEAD~20)\n"
				" %s -n -l 20 -m\t\t-- list *only* the maintainers of the files affected by the last 20 patches (git diff HEAD~20) grouped by maintaner\n"
				" %s -n -l v0.9.0\t\t-- list *only* the maintainers of the files affected by the last 20 patches (git diff v0.9.0)" % (__file__,	__file__,__file__,__file__,__file__),
                                add_help=True,
				formatter_class=RawTextHelpFormatter)

	parser.add_argument("-n", "--no-maintainers-check",
			dest="no_check", action="store_true", default=False,
			help="Do not check the MAINTAINERS file against the tree")

	parser.add_argument("-l", "--list-maintainers",
			dest="patchset", default=False,
			help="List maintainers for a patch/series of patches.\n"
				"PATCHSET can be: a patch file, or the number of commits \nfrom HEAD or commit SHA")

	parser.add_argument("--dont-group-by-maintainer",
			dest="grouped", action="store_false", default=True,
			help="If \"-l\" selected, don't group modified files by maintainer.\n")
	args = parser.parse_args()

	try:
		f = open(MAINTAINERS_FILE)
	except:
		errormsg("Unable to open maintainers file '"+MAINTAINERS_FILE+"'")

	#Parse maintainers file
	maintainers = parse_maintainers(f)
	#Check it
	if not args.no_check:
		scan_paths(maintainers, PATHS_TO_CHECK)
	#List maintainers
	if args.patchset:
		list_maintainers(maintainers, args.patchset, args.grouped)

	if errors > 0:
		print("Number of errors: %s" % errors)
	sys.exit(errors)

if __name__ == "__main__":
    main()
