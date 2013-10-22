#! /usr/bin/env python
from scapy.all import *
import fcntl, socket, struct, getopt,sys, struct

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
try:
    src_mac
except NameError:
    print 'Failed to get the source mac of your interface :('

dst_mac = "ff:ff:ff:ff:ff:ff"

len_a = len(paddr_a)
len_b = len(paddr_b)
s='\x00'
if len_a >= len_b:
    max_len = len_a
    add_len = max_len - len_b
    for i in range(add_len):
        paddr_b+=s
else:
    max_len = len_b
    add_len = max_len - len_a
    for i in range(add_len):
        paddr_a+=s

# print 'len_a:', len_a
# print 'len_b:',len_b
# print 'add_len:',add_len
# print 'max_len:',max_len
# print 'len_a, len_b after:',len(paddr_a)," ",len(paddr_b)

# hw type/protocol type/hw length
arp_packet='\x00\x01\xd1\xf0\x06'

# pa Length
hex_len=bytes(chr(max_len))
arp_packet+=hex_len

# opcode
arp_packet+='\x00\x01'

# mac addr src
macstr = src_mac.replace(':', '').decode('hex')
arp_packet+=macstr

# paddr a
b = str.encode(paddr_a)
arp_packet+=b

#broadcast mac addr
arp_packet+='\x00\x00\x00\x00\x00\x00'

# paddr b
c=str.encode(paddr_b)
arp_packet+=c
print 'Kazakh pride launching...'

print 'Arp packet:'
print(list(arp_packet))

if sendp(Ether(src=src_mac,dst=dst_mac,type=0x0806)/arp_packet)==0:
    print "Failed to send packet"

