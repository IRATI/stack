#! /usr/bin/env python
from scapy.all import *
import fcntl, socket, struct, getopt,sys

def getHwAddr(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', ifname[:15]))
    return ''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1]

options, remainder = getopt.getopt(sys.argv[1:], 'i:a:b:', ['interface=', 
                                                            'paddr_a=',
                                                            'paddr_b=',
                                                            ])
for opt, arg in options:
    if opt in ('-i', '--interface'):
        interface_name = arg
    elif opt in ('-a', '--paddr_a'):
        paddr_a = arg
    elif opt in ('-b', '-paddr_b'):
        paddr_b = arg

try:
    interface_name
    paddr_a
    paddr_b
except NameError:
    print 'Usage: ./rinarp.py -i interface_name -a paddr_a -b paddr_b'
    sys.exit()


src_mac = getHwAddr(interface_name)
dst_mac = "ff:ff:ff:ff:ff:ff"

len_a = len(paddr_a)
len_b = len(paddr_b)
if len_a >= len_b:
    max_len = len_a
    Grow b
else:
    max_len = len_b
    Grow a

Convert src_mac to hexstring

paddr_a to hexstring

paddr_b to hexstring

sendp(Ether(src=src_mac,dst=dst_mac)/'\x00\x01\x0D\x1F\x06\x02\x00\x01\x00\x25\x64\x5a\x89\xbb\x4e\x4f\x00\x00\x00\x00\x00\x00\x54\x49')



