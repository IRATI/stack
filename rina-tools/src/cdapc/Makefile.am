#
# Makefile.am
#
# Written by: Micheal Crotty <mcrotty AT tssg DOT org>
#
SUBDIRS      = encoders
EXTRA_DIST   =
CLEANFILES   =

bin_PROGRAMS = cdapc	unittests

cdapc_CPPFLAGS =					 \
	-std=c++11           \
	$(CPPFLAGS_EXTRA)		 \
	$(LIBRINA_CFLAGS)		 \
	-I$(srcdir)/../common	
	
cdapc_LDADD    = 	$(LIBRINA_LIBS) -ldl -lrinad \
	-Lencoders -lfullencoders \
	../common/librinaapp.la	
	
cdapc_SOURCES  =						\
	main.cc	\
	connector.cc			connector.h	\
	easywsclient.cc	  easywsclient.h \
	dmsworker.cc 			maworker.cc \
	eventfilters.cc   eventfilters.h \
	json_format.cc 		json_format.h \
	strutil.h 		    stl_util.h \
	map_util.h        eventsystem.h	    


unittests_SOURCES  =				\
	unittests.cc \
	cdapserialisertest.cc  eventfilterstest.cc \
	dmsworkertest.cc       dmsworker.cc \
	easywsclient.cc	  		 easywsclient.h \
	json_format.cc         json_format.h \
	eventfilters.cc   		 eventfilters.h \
	eventsystem.h					 catch.h
	
unittests_CPPFLAGS =		\
	-std=c++11            \
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
	
unittests_LDADD    = 	$(LIBRINA_LIBS) -ldl \
			-Lencoders -lfullencoders \
			../common/librinaapp.la	

#			$(builddir)/../common/librinad.la

check_PROGRAMS =				\
	unittests

XFAIL_TESTS =
PASS_TESTS  = unittests

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)
