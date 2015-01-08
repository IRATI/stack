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

tag = "confgen"

def debug(message):
    print(tag + ": " + message)

def error(message):
    print(tag + ": " + message)

try:
    DOM_topology = xml.dom.minidom.parse("topology.xml")

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

except Exception as e:
    error("Got a problem: " + str(e))
    sys.exit(1)

sys.exit(0)
