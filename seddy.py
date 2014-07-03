#!/bin/python2

import re
import sys
import os


acronyms = ['RIB', 'DIF', 'SDU', 'PDU']
#acronyms = []

def compute_name(sg):
        global acronyms

        if sg.count('_') > 0:
            # C-ish
            ret = sg[4:] + '_'
        else:
            # javish
            ret = sg[3].lower() + sg[4:]

        for a in acronyms:
            if not ret.islower():
                idx = ret.upper().find(a)
                if idx != -1:
                    ret = ret.replace(ret[idx:idx+len(a)], a)

        return ret

def setter_repl(m):
    global name
    return m.group(1) + name + ' = ' + m.group(2)

def getter_repl(m):
    global name
    return m.group(1) + name

showfp = False

# collects all headers
headers_dirs = ['librina/include']
headers_paths = []
for headers_dir in headers_dirs:
    for dirname, dirnames, filenames in os.walk(headers_dir):
        for filename in filenames:
            m = re.search(r'.+\.h$', filename)
            if m:
                headers_paths.append(os.path.join(dirname, filename))

# search for possible getters and getters in the collected files
print "Crawling headers..."
getters = set()
setters = set()
for path in headers_paths:
    f = open(path, 'r')
    content = f.read()

    candidates = re.findall(r'(get[^ \t\n\r(]+)\(', content)
    for candidate in candidates:
        name = compute_name(candidate)
        m = re.search(r'[ \n\t]+' + name + r'[ \n\t]*;', content)
        if m:
            getters.add(candidate)
        else:
            if showfp:
                print "False positive", name

    candidates = re.findall(r'(set[^ \t\n\r(]+)\(', content)
    for candidate in candidates:
        name = compute_name(candidate)
        m = re.search(r'[ \n\t]+' + name + r'[ \n\t]*;', content)
        if m:
            setters.add(candidate)
        else:
            if showfp:
                print "False positive", name

    f.close()
print "Header crawling completed"

for c in getters:
    print c, compute_name(c)
for c in setters:
    print c, compute_name(c)

# collects all the C, C++ and Java source files
target_dirs = ['librina/src', 'rinad/src', 'rina-tools/src']
target_paths = []
for target_dir in target_dirs:
    for dirname, dirnames, filenames in os.walk(target_dir):
        for filename in filenames:
            m = re.search(r'[^.]+\.((cc)|(h)|(c)|(java))$', filename)
            if m:
                target_paths.append(os.path.join(dirname, filename))

print "Replacing..."
for path in target_paths:
    f = open(path, 'r')
    content = f.read();
    f.close()

    for g in getters:
        name = compute_name(g)
        getter_pattern = r'(\.|>[ \t\n]*)' + g + '\(\)'
        content = re.sub(getter_pattern, getter_repl, content)

    for s in setters:
        name = compute_name(s)
        setter_pattern = r'(\.|>[ \t\n]*)' + s + '\(([^)]+)\)'
        content = re.sub(setter_pattern, setter_repl, content)

    f = open(path, 'w')
    f.write(content)
    f.close()
print "Replacing completed"
