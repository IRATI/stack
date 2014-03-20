#!@PYTHON@

#
# rina-mem-stats
#
# Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
#

import re
import sys
import argparse

def extract_points(records, idx):
    times = []
    values = []
    for r in records:
        try:
            t = r['t']
            y = r[idx]
            times.append(t)
            values.append(y)
        except KeyError:
            pass

    return times, values


# command line arguments definition and processing
parser = argparse.ArgumentParser(description = 'Show RINA memory usage statistics',
                                 epilog = 'Report bugs to <@PACKAGE_BUGREPORT@>')
parser.add_argument('-i', '--input', help = 'The log file from which you want to extract the memory statistics', required = True, metavar = 'INPUT_LOG')
parser.add_argument('-l', '--list', help = 'Show list of the collected memory stats records in a pretty way, instead of raw output', action = 'store_true')
args = parser.parse_args()


# extract the stats from the input log
fin = open(args.input, 'r')

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

# pretty print the collected data, if the user wants it,
# otherwise go for raw output, which is going to be
# fed to another script
if args.list:
    lr = None
    for r in records:
        try:
            t = r['t']
            print('Time %f (seconds)' % (t, ))
            for k in r.keys():
                if k != 't':
                    if lr:
                        diff = str(r[k] - lr[k])
                        if r[k] >= lr[k]:
                            diff = '+' + diff
                    else:
                        diff = '?'
                    print('\t(%d bytes) ==> %d items [%s]' % (pow(2, k), r[k], diff))
            print('')
            lr = r
        except KeyError:
            pass
else:
    idx = 0
    while 1:
        times, values = extract_points(records, idx)
        if len(times) == 0:
            break
        print str(pow(2, idx)) + '-bin'
        for i in range(len(times)):
            print times[i], values[i]
        idx += 1
