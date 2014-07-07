#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

CLEANFILES       =
MOSTLYCLEANFILES =
EXTRA_DIST       =

ApplicationProcessNamingInfoMessage.stamp:		\
	ApplicationProcessNamingInfoMessage.proto
	rm -f ApplicationProcessNamingInfoMessage.tmp
	touch ApplicationProcessNamingInfoMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)     \
		--cpp_out=$(builddir)           \
		$(srcdir)/ApplicationProcessNamingInfoMessage.proto
	mv -f ApplicationProcessNamingInfoMessage.tmp $@

ApplicationProcessNamingInfoMessage.pb.h	\
ApplicationProcessNamingInfoMessage.pb.cc:	\
ApplicationProcessNamingInfoMessage.stamp
	if test ! -f $@; then						     \
	  rm -f ApplicationProcessNamingInfoMessage.stamp;		     \
	  $(MAKE) $(AM_MAKEFLAGS) ApplicationProcessNamingInfoMessage.stamp; \
	fi

MOSTLYCLEANFILES +=				\
	ApplicationProcessNamingInfoMessage.tmp

EXTRA_DIST +=						\
	ApplicationProcessNamingInfoMessage.proto	\
	ApplicationProcessNamingInfoMessage.stamp


ApplicationRegistrationMessage.stamp:		\
	ApplicationRegistrationMessage.proto
	rm -f ApplicationRegistrationMessage.tmp
	touch ApplicationRegistrationMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)     \
		--cpp_out=$(builddir)           \
		$(srcdir)/ApplicationRegistrationMessage.proto
	mv -f ApplicationRegistrationMessage.tmp $@

ApplicationRegistrationMessage.pb.h	\
ApplicationRegistrationMessage.pb.cc:	\
ApplicationRegistrationMessage.stamp
	if test ! -f $@; then						\
	  rm -f ApplicationRegistrationMessage.stamp;			\
	  $(MAKE) $(AM_MAKEFLAGS) ApplicationRegistrationMessage.stamp;	\
	fi

MOSTLYCLEANFILES +=				\
	ApplicationRegistrationMessage.tmp

EXTRA_DIST +=					\
	ApplicationRegistrationMessage.proto	\
	ApplicationRegistrationMessage.stamp


CommonMessages.stamp:		\
	CommonMessages.proto
	rm -f CommonMessages.tmp
	touch CommonMessages.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)     \
		--cpp_out=$(builddir)           \
		$(srcdir)/CommonMessages.proto
	mv -f CommonMessages.tmp $@

CommonMessages.pb.h	\
CommonMessages.pb.cc:	\
CommonMessages.stamp
	if test ! -f $@; then				\
	  rm -f CommonMessages.stamp;			\
	  $(MAKE) $(AM_MAKEFLAGS) CommonMessages.stamp;	\
	fi

MOSTLYCLEANFILES +=				\
	CommonMessages.tmp

EXTRA_DIST +=					\
	CommonMessages.proto			\
	CommonMessages.stamp



noinst_LTLIBRARIES =				\
	libencoders.la

libencoders_la_SOURCES =				\
	ApplicationProcessNamingInfoMessage.pb.cc	\
	ApplicationProcessNamingInfoMessage.pb.h	\
	ApplicationRegistrationMessage.pb.cc		\
	ApplicationRegistrationMessage.pb.h		\
	CommonMessages.pb.cc				\
	CommonMessages.pb.h
