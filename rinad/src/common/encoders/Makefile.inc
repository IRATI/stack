protoSOURCES =


ApplicationProcessNamingInfoMessage.stamp: ApplicationProcessNamingInfoMessage.proto
	rm -f ApplicationProcessNamingInfoMessage.tmp
	touch ApplicationProcessNamingInfoMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/ApplicationProcessNamingInfoMessage.proto
	mv -f ApplicationProcessNamingInfoMessage.tmp $@

ApplicationProcessNamingInfoMessage.pb.h ApplicationProcessNamingInfoMessage.pb.cc: ApplicationProcessNamingInfoMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf ApplicationProcessNamingInfoMessage.lock ApplicationProcessNamingInfoMessage.stamp' 1 2 13 15; \
	  if mkdir ApplicationProcessNamingInfoMessage.lock 2>/dev/null; then \
	    rm -f ApplicationProcessNamingInfoMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) ApplicationProcessNamingInfoMessage.stamp; \
	    result=$$?; rm -rf ApplicationProcessNamingInfoMessage.lock; exit $$result; \
	  else \
	    while test -d ApplicationProcessNamingInfoMessage.lock; do sleep 1; done; \
	    test -f ApplicationProcessNamingInfoMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	ApplicationProcessNamingInfoMessage.tmp

EXTRA_DIST +=					\
	ApplicationProcessNamingInfoMessage.proto				\
	ApplicationProcessNamingInfoMessage.stamp

protoSOURCES += ApplicationProcessNamingInfoMessage.pb.cc ApplicationProcessNamingInfoMessage.pb.h


ApplicationRegistrationMessage.stamp: ApplicationRegistrationMessage.proto
	rm -f ApplicationRegistrationMessage.tmp
	touch ApplicationRegistrationMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/ApplicationRegistrationMessage.proto
	mv -f ApplicationRegistrationMessage.tmp $@

ApplicationRegistrationMessage.pb.h ApplicationRegistrationMessage.pb.cc: ApplicationRegistrationMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf ApplicationRegistrationMessage.lock ApplicationRegistrationMessage.stamp' 1 2 13 15; \
	  if mkdir ApplicationRegistrationMessage.lock 2>/dev/null; then \
	    rm -f ApplicationRegistrationMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) ApplicationRegistrationMessage.stamp; \
	    result=$$?; rm -rf ApplicationRegistrationMessage.lock; exit $$result; \
	  else \
	    while test -d ApplicationRegistrationMessage.lock; do sleep 1; done; \
	    test -f ApplicationRegistrationMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	ApplicationRegistrationMessage.tmp

EXTRA_DIST +=					\
	ApplicationRegistrationMessage.proto				\
	ApplicationRegistrationMessage.stamp

protoSOURCES += ApplicationRegistrationMessage.pb.cc ApplicationRegistrationMessage.pb.h


CommonMessages.stamp: CommonMessages.proto
	rm -f CommonMessages.tmp
	touch CommonMessages.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/CommonMessages.proto
	mv -f CommonMessages.tmp $@

CommonMessages.pb.h CommonMessages.pb.cc: CommonMessages.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf CommonMessages.lock CommonMessages.stamp' 1 2 13 15; \
	  if mkdir CommonMessages.lock 2>/dev/null; then \
	    rm -f CommonMessages.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) CommonMessages.stamp; \
	    result=$$?; rm -rf CommonMessages.lock; exit $$result; \
	  else \
	    while test -d CommonMessages.lock; do sleep 1; done; \
	    test -f CommonMessages.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	CommonMessages.tmp

EXTRA_DIST +=					\
	CommonMessages.proto				\
	CommonMessages.stamp

protoSOURCES += CommonMessages.pb.cc CommonMessages.pb.h


ConnectionPoliciesMessage.stamp: ConnectionPoliciesMessage.proto
	rm -f ConnectionPoliciesMessage.tmp
	touch ConnectionPoliciesMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/ConnectionPoliciesMessage.proto
	mv -f ConnectionPoliciesMessage.tmp $@

ConnectionPoliciesMessage.pb.h ConnectionPoliciesMessage.pb.cc: ConnectionPoliciesMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf ConnectionPoliciesMessage.lock ConnectionPoliciesMessage.stamp' 1 2 13 15; \
	  if mkdir ConnectionPoliciesMessage.lock 2>/dev/null; then \
	    rm -f ConnectionPoliciesMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) ConnectionPoliciesMessage.stamp; \
	    result=$$?; rm -rf ConnectionPoliciesMessage.lock; exit $$result; \
	  else \
	    while test -d ConnectionPoliciesMessage.lock; do sleep 1; done; \
	    test -f ConnectionPoliciesMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	ConnectionPoliciesMessage.tmp

EXTRA_DIST +=					\
	ConnectionPoliciesMessage.proto				\
	ConnectionPoliciesMessage.stamp

protoSOURCES += ConnectionPoliciesMessage.pb.cc ConnectionPoliciesMessage.pb.h


FlowMessage.stamp: FlowMessage.proto
	rm -f FlowMessage.tmp
	touch FlowMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/FlowMessage.proto
	mv -f FlowMessage.tmp $@

FlowMessage.pb.h FlowMessage.pb.cc: FlowMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf FlowMessage.lock FlowMessage.stamp' 1 2 13 15; \
	  if mkdir FlowMessage.lock 2>/dev/null; then \
	    rm -f FlowMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) FlowMessage.stamp; \
	    result=$$?; rm -rf FlowMessage.lock; exit $$result; \
	  else \
	    while test -d FlowMessage.lock; do sleep 1; done; \
	    test -f FlowMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	FlowMessage.tmp

EXTRA_DIST +=					\
	FlowMessage.proto				\
	FlowMessage.stamp

protoSOURCES += FlowMessage.pb.cc FlowMessage.pb.h


PolicyDescriptorMessage.stamp: PolicyDescriptorMessage.proto
	rm -f PolicyDescriptorMessage.tmp
	touch PolicyDescriptorMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/PolicyDescriptorMessage.proto
	mv -f PolicyDescriptorMessage.tmp $@

PolicyDescriptorMessage.pb.h PolicyDescriptorMessage.pb.cc: PolicyDescriptorMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf PolicyDescriptorMessage.lock PolicyDescriptorMessage.stamp' 1 2 13 15; \
	  if mkdir PolicyDescriptorMessage.lock 2>/dev/null; then \
	    rm -f PolicyDescriptorMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) PolicyDescriptorMessage.stamp; \
	    result=$$?; rm -rf PolicyDescriptorMessage.lock; exit $$result; \
	  else \
	    while test -d PolicyDescriptorMessage.lock; do sleep 1; done; \
	    test -f PolicyDescriptorMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	PolicyDescriptorMessage.tmp

EXTRA_DIST +=					\
	PolicyDescriptorMessage.proto				\
	PolicyDescriptorMessage.stamp

protoSOURCES += PolicyDescriptorMessage.pb.cc PolicyDescriptorMessage.pb.h


QoSSpecification.stamp: QoSSpecification.proto
	rm -f QoSSpecification.tmp
	touch QoSSpecification.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/QoSSpecification.proto
	mv -f QoSSpecification.tmp $@

QoSSpecification.pb.h QoSSpecification.pb.cc: QoSSpecification.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf QoSSpecification.lock QoSSpecification.stamp' 1 2 13 15; \
	  if mkdir QoSSpecification.lock 2>/dev/null; then \
	    rm -f QoSSpecification.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) QoSSpecification.stamp; \
	    result=$$?; rm -rf QoSSpecification.lock; exit $$result; \
	  else \
	    while test -d QoSSpecification.lock; do sleep 1; done; \
	    test -f QoSSpecification.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	QoSSpecification.tmp

EXTRA_DIST +=					\
	QoSSpecification.proto				\
	QoSSpecification.stamp

protoSOURCES += QoSSpecification.pb.cc QoSSpecification.pb.h