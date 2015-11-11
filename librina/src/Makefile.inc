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


auth-policies.stamp: auth-policies.proto
	rm -f auth-policies.tmp
	touch auth-policies.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/auth-policies.proto
	mv -f auth-policies.tmp $@

auth-policies.pb.h auth-policies.pb.cc: auth-policies.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf auth-policies.lock auth-policies.stamp' 1 2 13 15; \
	  if mkdir auth-policies.lock 2>/dev/null; then \
	    rm -f auth-policies.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) auth-policies.stamp; \
	    result=$$?; rm -rf auth-policies.lock; exit $$result; \
	  else \
	    while test -d auth-policies.lock; do sleep 1; done; \
	    test -f auth-policies.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        auth-policies.pb.h auth-policies.pb.cc auth-policies.stamp

MOSTLYCLEANFILES +=                             \
        auth-policies.tmp

EXTRA_DIST +=                                   \
        auth-policies.proto

protoSOURCES += auth-policies.pb.cc auth-policies.pb.h


irm.stamp: irm.proto
	rm -f irm.tmp
	touch irm.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/irm.proto
	mv -f irm.tmp $@

irm.pb.h irm.pb.cc: irm.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf irm.lock irm.stamp' 1 2 13 15; \
	  if mkdir irm.lock 2>/dev/null; then \
	    rm -f irm.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) irm.stamp; \
	    result=$$?; rm -rf irm.lock; exit $$result; \
	  else \
	    while test -d irm.lock; do sleep 1; done; \
	    test -f irm.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        irm.pb.h irm.pb.cc irm.stamp

MOSTLYCLEANFILES +=                             \
        irm.tmp

EXTRA_DIST +=                                   \
        irm.proto

protoSOURCES += irm.pb.cc irm.pb.h


