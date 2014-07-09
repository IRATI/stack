protoSOURCES =


ApplicationProcessNamingInfoMessage.stamp: ApplicationProcessNamingInfoMessage.proto
	rm -f ApplicationProcessNamingInfoMessage.tmp
	touch ApplicationProcessNamingInfoMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/ApplicationProcessNamingInfoMessage.proto
	mv -f ApplicationProcessNamingInfoMessage.tmp 

ApplicationProcessNamingInfoMessage.pb.h ApplicationProcessNamingInfoMessage.pb.cc: ApplicationProcessNamingInfoMessage.stamp
	if test ! -f ; then                           \
	  rm -f ApplicationProcessNamingInfoMessage.stamp;                            \
	  $(MAKE) $(AM_MAKEFLAGS) ApplicationProcessNamingInfoMessage.stamp;        \
	fi

MOSTLYCLEANFILES +=				\
	ApplicationProcessNamingInfoMessage.tmp

EXTRA_DIST +=					\
	ApplicationProcessNamingInfoMessage.proto				\
	ApplicationProcessNamingInfoMessage.stamp

protoSOURCES += ApplicationProcessNamingInfoMessage.pb.cc ApplicationProcessNamingInfoMessage.pb.h

CommonMessages.stamp: CommonMessages.proto
	rm -f CommonMessages.tmp
	touch CommonMessages.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/CommonMessages.proto
	mv -f CommonMessages.tmp 

CommonMessages.pb.h CommonMessages.pb.cc: CommonMessages.stamp
	if test ! -f ; then                           \
	  rm -f CommonMessages.stamp;                            \
	  $(MAKE) $(AM_MAKEFLAGS) CommonMessages.stamp;        \
	fi

MOSTLYCLEANFILES +=				\
	CommonMessages.tmp

EXTRA_DIST +=					\
	CommonMessages.proto				\
	CommonMessages.stamp

protoSOURCES += CommonMessages.pb.cc CommonMessages.pb.h

QoSSpecification.stamp:		\
	QoSSpecification.proto
	rm -f QoSSpecification.tmp
	touch QoSSpecification.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)     \
		--cpp_out=$(builddir)           \
		$(srcdir)/QoSSpecification.proto
	mv -f QoSSpecification.tmp $@

QoSSpecification.pb.h	\
QoSSpecification.pb.cc:	\
QoSSpecification.stamp
	if test ! -f $@; then				\
	  rm -f QoSSpecification.stamp;			\
	  $(MAKE) $(AM_MAKEFLAGS) QoSSpecification.stamp;	\
	fi

MOSTLYCLEANFILES +=				\
	QoSSpecification.tmp

EXTRA_DIST +=					\
	QoSSpecification.proto			\
	QoSSpecification.stamp
	
protoSOURCES += QoSSpecification.pb.cc QoSSpecification.pb.h
	
PolicyDescriptorMessage.stamp:		\
	PolicyDescriptorMessage.proto
	rm -f PolicyDescriptorMessage.tmp
	touch PolicyDescriptorMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)     \
		--cpp_out=$(builddir)           \
		$(srcdir)/PolicyDescriptorMessage.proto
	mv -f PolicyDescriptorMessage.tmp $@

PolicyDescriptorMessage.pb.h	\
PolicyDescriptorMessage.pb.cc:	\
PolicyDescriptorMessage.stamp
	if test ! -f $@; then				\
	  rm -f PolicyDescriptorMessage.stamp;			\
	  $(MAKE) $(AM_MAKEFLAGS) PolicyDescriptorMessage.stamp;	\
	fi

MOSTLYCLEANFILES +=				\
	PolicyDescriptorMessage.tmp

EXTRA_DIST +=					\
	PolicyDescriptorMessage.proto			\
	PolicyDescriptorMessage.stamp	
	
protoSOURCES += PolicyDescriptorMessage.pb.cc PolicyDescriptorMessage.pb.h
	
	
ConnectionPoliciesMessage.stamp:		\
	ConnectionPoliciesMessage.proto
	rm -f ConnectionPoliciesMessage.tmp
	touch ConnectionPoliciesMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)     \
		--cpp_out=$(builddir)           \
		$(srcdir)/ConnectionPoliciesMessage.proto
	mv -f ConnectionPoliciesMessage.tmp $@

ConnectionPoliciesMessage.pb.h	\
ConnectionPoliciesMessage.pb.cc:	\
ConnectionPoliciesMessage.stamp
	if test ! -f $@; then				\
	  rm -f ConnectionPoliciesMessage.stamp;			\
	  $(MAKE) $(AM_MAKEFLAGS) ConnectionPoliciesMessage.stamp;	\
	fi

MOSTLYCLEANFILES +=				\
	ConnectionPoliciesMessage.tmp

EXTRA_DIST +=					\
	ConnectionPoliciesMessage.proto			\
	ConnectionPoliciesMessage.stamp

protoSOURCES += ConnectionPoliciesMessage.pb.cc ConnectionPoliciesMessage.pb.h
	
	
FlowMessage.stamp:		\
	FlowMessage.proto
	rm -f FlowMessage.tmp
	touch FlowMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)     \
		--cpp_out=$(builddir)           \
		$(srcdir)/FlowMessage.proto
	mv -f FlowMessage.tmp $@

FlowMessage.pb.h	\
FlowMessage.pb.cc:	\
FlowMessage.stamp
	if test ! -f $@; then				\
	  rm -f FlowMessage.stamp;			\
	  $(MAKE) $(AM_MAKEFLAGS) FlowMessage.stamp;	\
	fi

MOSTLYCLEANFILES +=				\
	FlowMessage.tmp

EXTRA_DIST +=					\
	FlowMessage.proto			\
	FlowMessage.stamp
	
protoSOURCES += FlowMessage.pb.cc FlowMessage.pb.h






