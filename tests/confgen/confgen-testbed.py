#!/usr/bin/python

#
# Configuration generator
#
#    Francesco Salvestrini <>f.salvestrini@nextworks.it
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301  USA

import sys

import xml.dom.minidom
import argparse

tag = "confgen"

def debug(message):
    print(tag + ": " + message)

def error(message):
    print(tag + ": " + message)

#
# Main
#

topology_filename = "topology.xml"
ipcps_filename    = "ipcps.xml"

try:
    parser = argparse.ArgumentParser(description='A configuration builder.',
                                     epilog='Bugreport here')
    parser.add_argument('--topology',
                        type=str,
                        metavar="FILE",
                        default=[ topology_filename ],
                        nargs=1,
                        help='the topology XML filename ' + \
                             '(default: %(default)s)')

    parser.add_argument('--ipcps',
                        type=str,
                        metavar="FILE",
                        default=[ ipcps_filename ],
                        nargs=1,
                        help='the IPC Process XML filename ' + \
                             '(default: %(default)s)')

    args = parser.parse_args()

    topology_filename = args.topology[0]
    ipcps_filename    = args.ipcps[0]
except Exception as e:
    error("Cannot parse options: " + str(e))
    sys.exit(1)

debug("Input files:")
debug("  Topology = " + str(topology_filename))
debug("  IPCPs    = " + str(ipcps_filename))

try:
    DOM_topology = xml.dom.minidom.parse(topology_filename)

    debug("Parsing topology")

    topology = DOM_topology.documentElement
    assert topology.tagName == "topology"

    nodes = DOM_topology.getElementsByTagName("node")

    debug("File has " + str(len(nodes)) + " node(s), walking all of them ...")
    for node in nodes:
        debug("Node: " + node.getAttribute("id"))

        interfaces = node.getElementsByTagName("interface")
        for interface in interfaces:
            debug("  Interface: " + \
                  interface.getAttribute("id") + \
                  " (" + interface.getAttribute("ip") + ")")
    
    links = DOM_topology.getElementsByTagName("link")

    debug("File has " + str(len(links)) + " link(s), walking all of them ...")
    for link in links:
        debug("Link: " + link.getAttribute("id"))
        a = link.getElementsByTagName("from")[0]
        b = link.getElementsByTagName("to")[0]
        debug("  Connects: " + \
              a.getAttribute("node") + ":" + a.getAttribute("interface") + \
              " <-> " + \
              b.getAttribute("node") + ":" + b.getAttribute("interface"))
    debug("Topology parsed successfully")

    DOM_ipcps = xml.dom.minidom.parse(ipcps_filename)

    debug("Parsing ipcps")

    ipcps = DOM_ipcps.documentElement
    assert ipcps.tagName == "ipcps"

    nodes = DOM_ipcps.getElementsByTagName("node")
    for node in nodes:
        debug("Node:" + node.getAttribute("id"))
        ipcps = node.getElementsByTagName("ipcp")
        for ipcp in ipcps:
            debug("  IPCP = " + \
                  ipcp.getAttribute("ap-name") + \
                  " " + \
                  ipcp.getAttribute("ap-instance"))

            dif_assign    = ipcp.getAttribute("dif")

            registrations = ipcp.getElementsByTagName("register-dif")
            dif_register  = []
            for dif in registrations:
                dif_register.append(str(dif.getAttribute("name")))

            debug("  IPCP assigned to DIF:   " + dif_assign)
            debug("  IPCP registers to DIFs: " + str(dif_register))

    debug("IPCPs parsed successfully")

except Exception as e:
    error("Got a problem: " + str(e))
    sys.exit(1)

sys.exit(0)
