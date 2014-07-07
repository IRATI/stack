CDAP.stamp: CDAP.proto
	rm -f CDAP.tmp
	touch CDAP.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/CDAP.proto
	mv -f CDAP.tmp 

CDAP.pb.h CDAP.pb.cc: CDAP.stamp
	if test ! -f ; then                           \
	  rm -f CDAP.stamp;                            \
	  $(MAKE) $(AM_MAKEFLAGS) CDAP.stamp;        \
	fi

MOSTLYCLEANFILES +=				\
	CDAP.tmp

EXTRA_DIST +=					\
	CDAP.proto				\
	CDAP.stamp

