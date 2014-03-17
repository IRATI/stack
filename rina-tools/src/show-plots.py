#!@PYTHON@

#
# show-plots
#
# Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
#

import re
import sys
import matplotlib.pyplot as plt


# input
labels = []
pairs_set = []
pairs = []
while 1:
    line = sys.stdin.readline()
    if line == '':
        break
    m = re.match(r'^[^ ]+$', line)
    if m:
        # new plot
        if len(pairs):
            pairs_set.append(pairs)
        labels.append(m.group(0))
        pairs = dict()
        pairs['times'] = []
        pairs['values'] = []
    else:
        m = re.match(r'^([0-9]+) ([0-9]+)', line)
        if m:
            try:
                pairs['times'].append(int(m.group(1)))
                pairs['values'].append(int(m.group(2)))
            except ValueError:
                pass
if len(pairs):
    pairs_set.append(pairs)


# generate plots
for pairs in pairs_set:
    plt.plot(pairs['times'], pairs['values'])
    
plt.legend(labels)
plt.show()
