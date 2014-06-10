#!@PYTHON@

#
# show-plots
#
#    Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
