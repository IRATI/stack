#! /usr/bin/env python
from scapy.all import *

class RINARP(Packet):
    name = "RINA ARP Packet"
    fields_desc=[ XShortField("hwtype", 0x0001),
                  XShortField("ptype"),
                  ByteField("hwlen", 6),
                  FieldLenField("lenfield", None, length_of="psrc", length_of="pdst"),
                  ShortEnumField("op", 1, {"req":1, "rep":2}),
                  ARPSourceMACField("hwsrc"),
                  StrLenField("psrc", "", length_from = lambda pkt: pkt.lenfield),
                  MACField("hwdst", ETHER_ANY),
                  StrLenField("pdst", "", length_from = lambda pkt: pkt.lenfield)]

bind_layers(Ether, RINARP, type="0x0D1F")
