#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS = tclap encoders

#
#Libs
#
librinad_ladir = $(pkgincludedir)/ipcm
lib_LTLIBRARIES = librinad.la

#Distributable headers
librinad_la_HEADERS = 
	encoder.h \
	configuration.h

librinad_la_CFLAGS   =
librinad_la_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			\
	-I$(srcdir)/tclap			
librinad_la_LIBADD   =				\
	$(LIBRINA_LIBS)				\
	$(LIBPROTOBUF_LIBS)			\
	-Lencoders -lencoders

librinad_la_SOURCES  =					\
	debug.cc              debug.h                   \
	rina-configuration.cc rina-configuration.h	\
	configuration.h    configuration.cc   \
	encoder.cc	      encoder.h			\
			      concurrency.h

commonheadersdir = $(includedir)/rinad/common
commonheaders_HEADERS = \
	encoder.h	\
	configuration.h \
	rina-configuration.h

encodersheadersdir = $(includedir)/rinad/common/encoders
# Here 'make' has entered the build/rinad/src/common directory,
# so we can use relative paths to collect headers
encodersheaders_HEADERS =					\
	encoders/ApplicationProcessNamingInfoMessage.pb.h	\
	encoders/CommonMessages.pb.h				\
	encoders/ConnectionPoliciesMessage.pb.h			\
	encoders/DataTransferConstantsMessage.pb.h		\
	encoders/DirectoryForwardingTableEntryArrayMessage.pb.h	\
	encoders/DirectoryForwardingTableEntryMessage.pb.h	\
	encoders/DIFAllocatorMessages.pb.h	\
	encoders/EnrollmentInformationMessage.pb.h		\
	encoders/FlowMessage.pb.h				\
	encoders/FlowStateGroupMessage.pb.h			\
	encoders/FlowStateMessage.pb.h				\
	encoders/MA-IPCP.pb.h					\
	encoders/NeighborArrayMessage.pb.h			\
	encoders/NeighborMessage.pb.h				\
	encoders/PolicyDescriptorMessage.pb.h			\
	encoders/QoSCubeArrayMessage.pb.h			\
	encoders/QoSCubeMessage.pb.h				\
	encoders/QoSSpecification.pb.h				\
	encoders/WhatevercastNameArrayMessage.pb.h		\
	encoders/WhatevercastNameMessage.pb.h		\
	encoders/RIBObjectData.pb.h
	
EXTRA_DIST =					\
	librinad.pc.in
        
edit = $(SED)								\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'			\
        -e 's|@LIBPROTOBUF_MIN_VERSION[@]|$(LIBPROTOBUF_MIN_VERSION)|g'	\
		-e 's|@LIBPROTOBUF_CFLAGS[@]|$(LIBPROTOBUF_CFLAGS)|g'		\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'				\
        -e 's|@prefix[@]|$(prefix)|g'					\
        -e 's|@includedir[@]|$(includedir)|g'				\
        -e 's|@libdir[@]|$(libdir)|g'

librinad.pc: Makefile librinad.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = librinad.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = librinad.pc
