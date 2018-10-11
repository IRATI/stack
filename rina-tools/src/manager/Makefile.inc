protoSOURCES =
DISTCLEANFILES =

nm.stamp: nm.proto
	rm -f nm.tmp
	touch nm.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/nm.proto
	mv -f nm.tmp $@

nm.pb.h nm.pb.cc: nm.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf nm.lock nm.stamp' 1 2 13 15; \
	  if mkdir nm.lock 2>/dev/null; then \
	    rm -f nm.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) nm.stamp; \
	    result=$$?; rm -rf nm.lock; exit $$result; \
	  else \
	    while test -d nm.lock; do sleep 1; done; \
	    test -f nm.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        nm.pb.h nm.pb.cc nm.stamp

MOSTLYCLEANFILES +=                             \
        nm.tmp

EXTRA_DIST +=                                   \
        nm.proto

protoSOURCES += nm.pb.cc nm.pb.h


