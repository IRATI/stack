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


FlowStateGroupMessage.stamp: FlowStateGroupMessage.proto
	rm -f FlowStateGroupMessage.tmp
	touch FlowStateGroupMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/FlowStateGroupMessage.proto
	mv -f FlowStateGroupMessage.tmp $@

FlowStateGroupMessage.pb.h FlowStateGroupMessage.pb.cc: FlowStateGroupMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf FlowStateGroupMessage.lock FlowStateGroupMessage.stamp' 1 2 13 15; \
	  if mkdir FlowStateGroupMessage.lock 2>/dev/null; then \
	    rm -f FlowStateGroupMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) FlowStateGroupMessage.stamp; \
	    result=$$?; rm -rf FlowStateGroupMessage.lock; exit $$result; \
	  else \
	    while test -d FlowStateGroupMessage.lock; do sleep 1; done; \
	    test -f FlowStateGroupMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	FlowStateGroupMessage.tmp

EXTRA_DIST +=					\
	FlowStateGroupMessage.proto				\
	FlowStateGroupMessage.stamp

protoSOURCES += FlowStateGroupMessage.pb.cc FlowStateGroupMessage.pb.h


FlowStateMessage.stamp: FlowStateMessage.proto
	rm -f FlowStateMessage.tmp
	touch FlowStateMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/FlowStateMessage.proto
	mv -f FlowStateMessage.tmp $@

FlowStateMessage.pb.h FlowStateMessage.pb.cc: FlowStateMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf FlowStateMessage.lock FlowStateMessage.stamp' 1 2 13 15; \
	  if mkdir FlowStateMessage.lock 2>/dev/null; then \
	    rm -f FlowStateMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) FlowStateMessage.stamp; \
	    result=$$?; rm -rf FlowStateMessage.lock; exit $$result; \
	  else \
	    while test -d FlowStateMessage.lock; do sleep 1; done; \
	    test -f FlowStateMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	FlowStateMessage.tmp

EXTRA_DIST +=					\
	FlowStateMessage.proto				\
	FlowStateMessage.stamp

protoSOURCES += FlowStateMessage.pb.cc FlowStateMessage.pb.h


NeighborArrayMessage.stamp: NeighborArrayMessage.proto
	rm -f NeighborArrayMessage.tmp
	touch NeighborArrayMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/NeighborArrayMessage.proto
	mv -f NeighborArrayMessage.tmp $@

NeighborArrayMessage.pb.h NeighborArrayMessage.pb.cc: NeighborArrayMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf NeighborArrayMessage.lock NeighborArrayMessage.stamp' 1 2 13 15; \
	  if mkdir NeighborArrayMessage.lock 2>/dev/null; then \
	    rm -f NeighborArrayMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) NeighborArrayMessage.stamp; \
	    result=$$?; rm -rf NeighborArrayMessage.lock; exit $$result; \
	  else \
	    while test -d NeighborArrayMessage.lock; do sleep 1; done; \
	    test -f NeighborArrayMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	NeighborArrayMessage.tmp

EXTRA_DIST +=					\
	NeighborArrayMessage.proto				\
	NeighborArrayMessage.stamp

protoSOURCES += NeighborArrayMessage.pb.cc NeighborArrayMessage.pb.h


NeighborMessage.stamp: NeighborMessage.proto
	rm -f NeighborMessage.tmp
	touch NeighborMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/NeighborMessage.proto
	mv -f NeighborMessage.tmp $@

NeighborMessage.pb.h NeighborMessage.pb.cc: NeighborMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf NeighborMessage.lock NeighborMessage.stamp' 1 2 13 15; \
	  if mkdir NeighborMessage.lock 2>/dev/null; then \
	    rm -f NeighborMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) NeighborMessage.stamp; \
	    result=$$?; rm -rf NeighborMessage.lock; exit $$result; \
	  else \
	    while test -d NeighborMessage.lock; do sleep 1; done; \
	    test -f NeighborMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	NeighborMessage.tmp

EXTRA_DIST +=					\
	NeighborMessage.proto				\
	NeighborMessage.stamp

protoSOURCES += NeighborMessage.pb.cc NeighborMessage.pb.h


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


WhatevercastNameArrayMessage.stamp: WhatevercastNameArrayMessage.proto
	rm -f WhatevercastNameArrayMessage.tmp
	touch WhatevercastNameArrayMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/WhatevercastNameArrayMessage.proto
	mv -f WhatevercastNameArrayMessage.tmp $@

WhatevercastNameArrayMessage.pb.h WhatevercastNameArrayMessage.pb.cc: WhatevercastNameArrayMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf WhatevercastNameArrayMessage.lock WhatevercastNameArrayMessage.stamp' 1 2 13 15; \
	  if mkdir WhatevercastNameArrayMessage.lock 2>/dev/null; then \
	    rm -f WhatevercastNameArrayMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) WhatevercastNameArrayMessage.stamp; \
	    result=$$?; rm -rf WhatevercastNameArrayMessage.lock; exit $$result; \
	  else \
	    while test -d WhatevercastNameArrayMessage.lock; do sleep 1; done; \
	    test -f WhatevercastNameArrayMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	WhatevercastNameArrayMessage.tmp

EXTRA_DIST +=					\
	WhatevercastNameArrayMessage.proto				\
	WhatevercastNameArrayMessage.stamp

protoSOURCES += WhatevercastNameArrayMessage.pb.cc WhatevercastNameArrayMessage.pb.h


WhatevercastNameMessage.stamp: WhatevercastNameMessage.proto
	rm -f WhatevercastNameMessage.tmp
	touch WhatevercastNameMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/WhatevercastNameMessage.proto
	mv -f WhatevercastNameMessage.tmp $@

WhatevercastNameMessage.pb.h WhatevercastNameMessage.pb.cc: WhatevercastNameMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf WhatevercastNameMessage.lock WhatevercastNameMessage.stamp' 1 2 13 15; \
	  if mkdir WhatevercastNameMessage.lock 2>/dev/null; then \
	    rm -f WhatevercastNameMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) WhatevercastNameMessage.stamp; \
	    result=$$?; rm -rf WhatevercastNameMessage.lock; exit $$result; \
	  else \
	    while test -d WhatevercastNameMessage.lock; do sleep 1; done; \
	    test -f WhatevercastNameMessage.stamp; \
	  fi; \
	fi

MOSTLYCLEANFILES +=				\
	WhatevercastNameMessage.tmp

EXTRA_DIST +=					\
	WhatevercastNameMessage.proto				\
	WhatevercastNameMessage.stamp

protoSOURCES += WhatevercastNameMessage.pb.cc WhatevercastNameMessage.pb.h


