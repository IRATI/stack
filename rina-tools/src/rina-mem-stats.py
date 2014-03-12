#!@PYTHON@

#
# rina-mem-stats
#
# Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
#

import re
import sys

if len(sys.argv) < 2:
    print("Usage: mem-stats.py LOG_FILE")
    quit()

file_name      = sys.argv[1]
fin            = open(file_name, 'r')
memstat_banner = 'MEMSTAT '
records        = []

while True:
    line = fin.readline()
    if line == "":
        break;

    m = re.search(r'' + memstat_banner + '(.*)', line)
    if m:
        msg = m.group(1)
        if msg.startswith('BEG'):
            cur = dict()
            records.append(cur)
            m = re.search('[0-9]+', msg)
            if m:
                try:
                    cur['t'] = int(m.group(0))
                except ValueError:
                    pass
        elif msg.startswith('END'):
            pass
        else:
            m = re.match(r'([0-9]+) ([0-9]+)', msg)
            if m:
                try:
                    idx      = int(m.group(1))
                    val      = int(m.group(2))
                    cur[idx] = val
                except ValueError:
                    pass

fin.close();

lr = None
for r in records:
    try:
        t = r['t']
        print('Time %f (seconds)' % (t/1000, ))
        for k in r.keys():
            if k != 't':
                if lr:
                    diff = str(r[k] - lr[k])
                    if r[k] >= lr[k]:
                        diff = '+' + diff
                else:
                    diff = '?'
                print('\t(%d bytes) ==> %d items [%s]' %
                      (pow(2, k), r[k], diff))
        print('')
        lr = r
    except KeyError:
        pass

# more elaborations here
