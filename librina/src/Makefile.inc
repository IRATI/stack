protoSOURCES =
DISTCLEANFILES =

CDAP.stamp: CDAP.proto
	rm -f CDAP.tmp
	touch CDAP.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/CDAP.proto
	mv -f CDAP.tmp $@

CDAP.pb.h CDAP.pb.cc: CDAP.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf CDAP.lock CDAP.stamp' 1 2 13 15; \
	  if mkdir CDAP.lock 2>/dev/null; then \
	    rm -f CDAP.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) CDAP.stamp; \
	    result=$$?; rm -rf CDAP.lock; exit $$result; \
	  else \
	    while test -d CDAP.lock; do sleep 1; done; \
	    test -f CDAP.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        CDAP.pb.h CDAP.pb.cc CDAP.stamp

MOSTLYCLEANFILES +=                             \
        CDAP.tmp

EXTRA_DIST +=                                   \
        CDAP.proto

protoSOURCES += CDAP.pb.cc CDAP.pb.h


