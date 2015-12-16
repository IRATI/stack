#
# Makefile for IRATI Flow Allocator Rate Enforcement plugin.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#
# Root folder.
root=/pristine
# Userspace compiled folder.
rootu=/pristine/userspace 
# Final directory where to put the policy.
out-dir=$(rootu)/lib/rinad/ipcp/
# Librina headers.
librina-dir=$(root)/librina/include
# Rinad elements.
rinad-dir=$(root)/rinad/src
# Rinad compiled elements.
rinad-out-dir=$(root)/build/rinad/src/
# Compiled libraries.
lib-dir=$(rootu)/lib

all:
	cp fare.manifest $(out-dir)
	g++ -I$(rinad-dir) -I$(rinad-dir)/common -I$(rinad-out-dir) -I$(librina-dir) -L$(lib-dir) -lrina -fPIC -shared fa-ps-fare.cpp -o $(out-dir)/fare.so