# written for python 2.x versions

import re
import sys
import matplotlib.pyplot as plt
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
parser = argparse.ArgumentParser(description = 'Show RINA memory usage statistics')
parser.add_argument('-i', '--input', help = 'The log file from which you want to extract the memory statistics', required = True, metavar = 'INPUT_LOG')
parser.add_argument('-p', '--plot', help = 'Show a plot of the memory stats', action = 'store_true')
parser.add_argument('-l', '--list', help = 'Show a list of the collected memory stats records', action = 'store_true')
args = parser.parse_args()


# extract the stats from the input log
fin = open(args.input, 'r')

memstat_banner = 'MEMSTAT '
records = []

while 1:
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
                    cur['t'] = float(m.group(0))/1000
                except ValueError:
                    pass
        elif msg.startswith('END'):
            pass
        else:
            m = re.match(r'([0-9]+) ([0-9]+)', msg)
            if m:
                try:
                    idx = int(m.group(1))
                    val = int(m.group(2))
                    cur[idx] = val
                except ValueError:
                    pass

fin.close();

# show a list, if the user wants it
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


# show a plot, if the user wants it
if args.plot:
    labels = []
    idx = 0
    while 1:
        times, values = extract_points(records, idx)
        if len(times) == 0:
            break
        plt.plot(times, values)
        labels.append(str(pow(2, idx)) + '-bin')
        idx += 1

    plt.legend(labels)
    plt.show()
