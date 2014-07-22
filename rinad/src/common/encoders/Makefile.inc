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


DataTransferConstantsMessage.stamp: DataTransferConstantsMessage.proto
	rm -f DataTransferConstantsMessage.tmp
	touch DataTransferConstantsMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/DataTransferConstantsMessage.proto
	mv -f DataTransferConstantsMessage.tmp $@

DataTransferConstantsMessage.pb.h DataTransferConstantsMessage.pb.cc: DataTransferConstantsMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf DataTransferConstantsMessage.lock DataTransferConstantsMessage.stamp' 1 2 13 15; \
	  if mkdir DataTransferConstantsMessage.lock 2>/dev/null; then \
	    rm -f DataTransferConstantsMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) DataTransferConstantsMessage.stamp; \
	    result=$$?; rm -rf DataTransferConstantsMessage.lock; exit $$result; \
	  else \
	    while test -d DataTransferConstantsMessage.lock; do sleep 1; done; \
	    test -f DataTransferConstantsMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	DataTransferConstantsMessage.tmp

EXTRA_DIST +=					\
	DataTransferConstantsMessage.proto				\
	DataTransferConstantsMessage.stamp

protoSOURCES += DataTransferConstantsMessage.pb.cc DataTransferConstantsMessage.pb.h


DirectoryForwardingTableEntryMessage.stamp: DirectoryForwardingTableEntryMessage.proto
	rm -f DirectoryForwardingTableEntryMessage.tmp
	touch DirectoryForwardingTableEntryMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/DirectoryForwardingTableEntryMessage.proto
	mv -f DirectoryForwardingTableEntryMessage.tmp $@

DirectoryForwardingTableEntryMessage.pb.h DirectoryForwardingTableEntryMessage.pb.cc: DirectoryForwardingTableEntryMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf DirectoryForwardingTableEntryMessage.lock DirectoryForwardingTableEntryMessage.stamp' 1 2 13 15; \
	  if mkdir DirectoryForwardingTableEntryMessage.lock 2>/dev/null; then \
	    rm -f DirectoryForwardingTableEntryMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) DirectoryForwardingTableEntryMessage.stamp; \
	    result=$$?; rm -rf DirectoryForwardingTableEntryMessage.lock; exit $$result; \
	  else \
	    while test -d DirectoryForwardingTableEntryMessage.lock; do sleep 1; done; \
	    test -f DirectoryForwardingTableEntryMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	DirectoryForwardingTableEntryMessage.tmp

EXTRA_DIST +=					\
	DirectoryForwardingTableEntryMessage.proto				\
	DirectoryForwardingTableEntryMessage.stamp

protoSOURCES += DirectoryForwardingTableEntryMessage.pb.cc DirectoryForwardingTableEntryMessage.pb.h


DirectoryForwardingTableEntryArrayMessage.stamp: DirectoryForwardingTableEntryArrayMessage.proto
	rm -f DirectoryForwardingTableEntryArrayMessage.tmp
	touch DirectoryForwardingTableEntryArrayMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/DirectoryForwardingTableEntryArrayMessage.proto
	mv -f DirectoryForwardingTableEntryArrayMessage.tmp $@

DirectoryForwardingTableEntryArrayMessage.pb.h DirectoryForwardingTableEntryArrayMessage.pb.cc: DirectoryForwardingTableEntryArrayMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf DirectoryForwardingTableEntryArrayMessage.lock DirectoryForwardingTableEntryArrayMessage.stamp' 1 2 13 15; \
	  if mkdir DirectoryForwardingTableEntryArrayMessage.lock 2>/dev/null; then \
	    rm -f DirectoryForwardingTableEntryArrayMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) DirectoryForwardingTableEntryArrayMessage.stamp; \
	    result=$$?; rm -rf DirectoryForwardingTableEntryArrayMessage.lock; exit $$result; \
	  else \
	    while test -d DirectoryForwardingTableEntryArrayMessage.lock; do sleep 1; done; \
	    test -f DirectoryForwardingTableEntryArrayMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	DirectoryForwardingTableEntryArrayMessage.tmp

EXTRA_DIST +=					\
	DirectoryForwardingTableEntryArrayMessage.proto				\
	DirectoryForwardingTableEntryArrayMessage.stamp

protoSOURCES += DirectoryForwardingTableEntryArrayMessage.pb.cc DirectoryForwardingTableEntryArrayMessage.pb.h


EnrollmentInformationMessage.stamp: EnrollmentInformationMessage.proto
	rm -f EnrollmentInformationMessage.tmp
	touch EnrollmentInformationMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/EnrollmentInformationMessage.proto
	mv -f EnrollmentInformationMessage.tmp $@

EnrollmentInformationMessage.pb.h EnrollmentInformationMessage.pb.cc: EnrollmentInformationMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf EnrollmentInformationMessage.lock EnrollmentInformationMessage.stamp' 1 2 13 15; \
	  if mkdir EnrollmentInformationMessage.lock 2>/dev/null; then \
	    rm -f EnrollmentInformationMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) EnrollmentInformationMessage.stamp; \
	    result=$$?; rm -rf EnrollmentInformationMessage.lock; exit $$result; \
	  else \
	    while test -d EnrollmentInformationMessage.lock; do sleep 1; done; \
	    test -f EnrollmentInformationMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	EnrollmentInformationMessage.tmp

EXTRA_DIST +=					\
	EnrollmentInformationMessage.proto				\
	EnrollmentInformationMessage.stamp

protoSOURCES += EnrollmentInformationMessage.pb.cc EnrollmentInformationMessage.pb.h


QoSCubeMessage.stamp: QoSCubeMessage.proto
	rm -f QoSCubeMessage.tmp
	touch QoSCubeMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/QoSCubeMessage.proto
	mv -f QoSCubeMessage.tmp $@

QoSCubeMessage.pb.h QoSCubeMessage.pb.cc: QoSCubeMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf QoSCubeMessage.lock QoSCubeMessage.stamp' 1 2 13 15; \
	  if mkdir QoSCubeMessage.lock 2>/dev/null; then \
	    rm -f QoSCubeMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) QoSCubeMessage.stamp; \
	    result=$$?; rm -rf QoSCubeMessage.lock; exit $$result; \
	  else \
	    while test -d QoSCubeMessage.lock; do sleep 1; done; \
	    test -f QoSCubeMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	QoSCubeMessage.tmp

EXTRA_DIST +=					\
	QoSCubeMessage.proto				\
	QoSCubeMessage.stamp

protoSOURCES += QoSCubeMessage.pb.cc QoSCubeMessage.pb.h


QoSCubeArrayMessage.stamp: QoSCubeArrayMessage.proto
	rm -f QoSCubeArrayMessage.tmp
	touch QoSCubeArrayMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/QoSCubeArrayMessage.proto
	mv -f QoSCubeArrayMessage.tmp $@

QoSCubeArrayMessage.pb.h QoSCubeArrayMessage.pb.cc: QoSCubeArrayMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf QoSCubeArrayMessage.lock QoSCubeArrayMessage.stamp' 1 2 13 15; \
	  if mkdir QoSCubeArrayMessage.lock 2>/dev/null; then \
	    rm -f QoSCubeArrayMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) QoSCubeArrayMessage.stamp; \
	    result=$$?; rm -rf QoSCubeArrayMessage.lock; exit $$result; \
	  else \
	    while test -d QoSCubeArrayMessage.lock; do sleep 1; done; \
	    test -f QoSCubeArrayMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	QoSCubeArrayMessage.tmp

EXTRA_DIST +=					\
	QoSCubeArrayMessage.proto				\
	QoSCubeArrayMessage.stamp

protoSOURCES += QoSCubeArrayMessage.pb.cc QoSCubeArrayMessage.pb.h