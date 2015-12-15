#
# Makefile for IRATI Flow Allocator Rate Enforcement plugin.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#
# Final directory where to put the policy.
out-dir=/pristine/userspace/lib/rinad/ipcp/
# Librina headers.
librina-dir=/pristine/librina/include
# Rinad elements.
rinad-dir=/pristine/rinad/src
# Rinad compiled elements.
rinad-out-dir=/pristine/build/rinad/src/
# Compiled libraries.
lib-dir=/pristine/userspace/lib

all:
	cp fare.manifest $(out-dir)
	g++ -I$(rinad-dir) -I$(rinad-dir)/common -I$(rinad-out-dir) -I$(librina-dir) -L$(lib-dir) -lrina -fPIC -shared fa-ps-fare.cpp -o $(out-dir)/fare.so