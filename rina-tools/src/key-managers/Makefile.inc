protoSOURCES =
DISTCLEANFILES =

km.stamp: km.proto
	rm -f km.tmp
	touch km.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/km.proto
	mv -f km.tmp $@

km.pb.h km.pb.cc: km.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf km.lock km.stamp' 1 2 13 15; \
	  if mkdir km.lock 2>/dev/null; then \
	    rm -f km.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) km.stamp; \
	    result=$$?; rm -rf km.lock; exit $$result; \
	  else \
	    while test -d km.lock; do sleep 1; done; \
	    test -f km.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        km.pb.h km.pb.cc km.stamp

MOSTLYCLEANFILES +=                             \
        km.tmp

EXTRA_DIST +=                                   \
        km.proto

protoSOURCES += km.pb.cc km.pb.h


