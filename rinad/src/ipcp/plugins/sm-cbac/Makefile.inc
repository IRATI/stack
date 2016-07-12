EXTRA_DIST   =
CLEANFILES   =
DISTCLEANFILES = 
protoSOURCES = 
MOSTLYCLEANFILES =
othersrcdir = $(srcdir)/../../../common/encoders

sm-cbac.stamp: sm-cbac.proto
	rm -f sm-cbac.tmp
	touch sm-cbac.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)  -I$(othersrcdir) \
                --cpp_out=$(builddir)           \
                $(srcdir)/sm-cbac.proto $(othersrcdir)/ApplicationProcessNamingInfoMessage.proto
	mv -f sm-cbac.tmp $@

sm-cbac.pb.h sm-cbac.pb.cc: sm-cbac.stamp
	if test -f $@; then :; else \
                trap 'rm -rf sm-cbac.lock sm-cbac.stamp' 1 2 13 15; \
                if mkdir sm-cbac.lock 2>/dev/null; then \
                        rm -f sm-cbac.stamp; \
                        $(MAKE) $(AM_MAKEFLAGS) sm-cbac.stamp; \
                        result=$$?; rm -rf sm-cbac.lock; exit $$result; \
                else \
                        while test -d sm-cbac.lock; do sleep 1; done; \
                        test -f sm-cbac.stamp; \
                fi; \
	fi
DISTCLEANFILES +=                               \
        sm-cbac.pb.h sm-cbac.pb.cc sm-cbac.stamp

MOSTLYCLEANFILES +=                             \
        sm-cbac.tmp

EXTRA_DIST +=                                   \
        sm-cbac.proto

protoSOURCES += sm-cbac.pb.cc sm-cbac.pb.h
