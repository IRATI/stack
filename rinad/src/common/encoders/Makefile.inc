protoSOURCES =
DISTCLEANFILES =

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

DISTCLEANFILES +=                               \
        ApplicationProcessNamingInfoMessage.pb.h ApplicationProcessNamingInfoMessage.pb.cc ApplicationProcessNamingInfoMessage.stamp

MOSTLYCLEANFILES +=                             \
        ApplicationProcessNamingInfoMessage.tmp

EXTRA_DIST +=                                   \
        ApplicationProcessNamingInfoMessage.proto

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

DISTCLEANFILES +=                               \
        CommonMessages.pb.h CommonMessages.pb.cc CommonMessages.stamp

MOSTLYCLEANFILES +=                             \
        CommonMessages.tmp

EXTRA_DIST +=                                   \
        CommonMessages.proto

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

DISTCLEANFILES +=                               \
        ConnectionPoliciesMessage.pb.h ConnectionPoliciesMessage.pb.cc ConnectionPoliciesMessage.stamp

MOSTLYCLEANFILES +=                             \
        ConnectionPoliciesMessage.tmp

EXTRA_DIST +=                                   \
        ConnectionPoliciesMessage.proto

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

DISTCLEANFILES +=                               \
        DataTransferConstantsMessage.pb.h DataTransferConstantsMessage.pb.cc DataTransferConstantsMessage.stamp

MOSTLYCLEANFILES +=                             \
        DataTransferConstantsMessage.tmp

EXTRA_DIST +=                                   \
        DataTransferConstantsMessage.proto

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

DISTCLEANFILES +=                               \
        DirectoryForwardingTableEntryArrayMessage.pb.h DirectoryForwardingTableEntryArrayMessage.pb.cc DirectoryForwardingTableEntryArrayMessage.stamp

MOSTLYCLEANFILES +=                             \
        DirectoryForwardingTableEntryArrayMessage.tmp

EXTRA_DIST +=                                   \
        DirectoryForwardingTableEntryArrayMessage.proto

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

DISTCLEANFILES +=                               \
        DirectoryForwardingTableEntryMessage.pb.h DirectoryForwardingTableEntryMessage.pb.cc DirectoryForwardingTableEntryMessage.stamp

MOSTLYCLEANFILES +=                             \
        DirectoryForwardingTableEntryMessage.tmp

EXTRA_DIST +=                                   \
        DirectoryForwardingTableEntryMessage.proto

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

DISTCLEANFILES +=                               \
        EnrollmentInformationMessage.pb.h EnrollmentInformationMessage.pb.cc EnrollmentInformationMessage.stamp

MOSTLYCLEANFILES +=                             \
        EnrollmentInformationMessage.tmp

EXTRA_DIST +=                                   \
        EnrollmentInformationMessage.proto

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

DISTCLEANFILES +=                               \
        FlowMessage.pb.h FlowMessage.pb.cc FlowMessage.stamp

MOSTLYCLEANFILES +=                             \
        FlowMessage.tmp

EXTRA_DIST +=                                   \
        FlowMessage.proto

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

DISTCLEANFILES +=                               \
        FlowStateGroupMessage.pb.h FlowStateGroupMessage.pb.cc FlowStateGroupMessage.stamp

MOSTLYCLEANFILES +=                             \
        FlowStateGroupMessage.tmp

EXTRA_DIST +=                                   \
        FlowStateGroupMessage.proto

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

DISTCLEANFILES +=                               \
        FlowStateMessage.pb.h FlowStateMessage.pb.cc FlowStateMessage.stamp

MOSTLYCLEANFILES +=                             \
        FlowStateMessage.tmp

EXTRA_DIST +=                                   \
        FlowStateMessage.proto

protoSOURCES += FlowStateMessage.pb.cc FlowStateMessage.pb.h


MA-IPCP.stamp: MA-IPCP.proto
	rm -f MA-IPCP.tmp
	touch MA-IPCP.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/MA-IPCP.proto
	mv -f MA-IPCP.tmp $@

MA-IPCP.pb.h MA-IPCP.pb.cc: MA-IPCP.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf MA-IPCP.lock MA-IPCP.stamp' 1 2 13 15; \
	  if mkdir MA-IPCP.lock 2>/dev/null; then \
	    rm -f MA-IPCP.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) MA-IPCP.stamp; \
	    result=$$?; rm -rf MA-IPCP.lock; exit $$result; \
	  else \
	    while test -d MA-IPCP.lock; do sleep 1; done; \
	    test -f MA-IPCP.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        MA-IPCP.pb.h MA-IPCP.pb.cc MA-IPCP.stamp

MOSTLYCLEANFILES +=                             \
        MA-IPCP.tmp

EXTRA_DIST +=                                   \
        MA-IPCP.proto

protoSOURCES += MA-IPCP.pb.cc MA-IPCP.pb.h


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

DISTCLEANFILES +=                               \
        NeighborArrayMessage.pb.h NeighborArrayMessage.pb.cc NeighborArrayMessage.stamp

MOSTLYCLEANFILES +=                             \
        NeighborArrayMessage.tmp

EXTRA_DIST +=                                   \
        NeighborArrayMessage.proto

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

DISTCLEANFILES +=                               \
        NeighborMessage.pb.h NeighborMessage.pb.cc NeighborMessage.stamp

MOSTLYCLEANFILES +=                             \
        NeighborMessage.tmp

EXTRA_DIST +=                                   \
        NeighborMessage.proto

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

DISTCLEANFILES +=                               \
        PolicyDescriptorMessage.pb.h PolicyDescriptorMessage.pb.cc PolicyDescriptorMessage.stamp

MOSTLYCLEANFILES +=                             \
        PolicyDescriptorMessage.tmp

EXTRA_DIST +=                                   \
        PolicyDescriptorMessage.proto

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

DISTCLEANFILES +=                               \
        QoSCubeArrayMessage.pb.h QoSCubeArrayMessage.pb.cc QoSCubeArrayMessage.stamp

MOSTLYCLEANFILES +=                             \
        QoSCubeArrayMessage.tmp

EXTRA_DIST +=                                   \
        QoSCubeArrayMessage.proto

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

DISTCLEANFILES +=                               \
        QoSCubeMessage.pb.h QoSCubeMessage.pb.cc QoSCubeMessage.stamp

MOSTLYCLEANFILES +=                             \
        QoSCubeMessage.tmp

EXTRA_DIST +=                                   \
        QoSCubeMessage.proto

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

DISTCLEANFILES +=                               \
        QoSSpecification.pb.h QoSSpecification.pb.cc QoSSpecification.stamp

MOSTLYCLEANFILES +=                             \
        QoSSpecification.tmp

EXTRA_DIST +=                                   \
        QoSSpecification.proto

protoSOURCES += QoSSpecification.pb.cc QoSSpecification.pb.h


RIBObjectData.stamp: RIBObjectData.proto
	rm -f RIBObjectData.tmp
	touch RIBObjectData.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/RIBObjectData.proto
	mv -f RIBObjectData.tmp $@

RIBObjectData.pb.h RIBObjectData.pb.cc: RIBObjectData.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf RIBObjectData.lock RIBObjectData.stamp' 1 2 13 15; \
	  if mkdir RIBObjectData.lock 2>/dev/null; then \
	    rm -f RIBObjectData.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) RIBObjectData.stamp; \
	    result=$$?; rm -rf RIBObjectData.lock; exit $$result; \
	  else \
	    while test -d RIBObjectData.lock; do sleep 1; done; \
	    test -f RIBObjectData.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        RIBObjectData.pb.h RIBObjectData.pb.cc RIBObjectData.stamp

MOSTLYCLEANFILES +=                             \
        RIBObjectData.tmp

EXTRA_DIST +=                                   \
        RIBObjectData.proto

protoSOURCES += RIBObjectData.pb.cc RIBObjectData.pb.h


RoutingForwarding.stamp: RoutingForwarding.proto
	rm -f RoutingForwarding.tmp
	touch RoutingForwarding.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/RoutingForwarding.proto
	mv -f RoutingForwarding.tmp $@

RoutingForwarding.pb.h RoutingForwarding.pb.cc: RoutingForwarding.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf RoutingForwarding.lock RoutingForwarding.stamp' 1 2 13 15; \
	  if mkdir RoutingForwarding.lock 2>/dev/null; then \
	    rm -f RoutingForwarding.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) RoutingForwarding.stamp; \
	    result=$$?; rm -rf RoutingForwarding.lock; exit $$result; \
	  else \
	    while test -d RoutingForwarding.lock; do sleep 1; done; \
	    test -f RoutingForwarding.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        RoutingForwarding.pb.h RoutingForwarding.pb.cc RoutingForwarding.stamp

MOSTLYCLEANFILES +=                             \
        RoutingForwarding.tmp

EXTRA_DIST +=                                   \
        RoutingForwarding.proto

protoSOURCES += RoutingForwarding.pb.cc RoutingForwarding.pb.h


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

DISTCLEANFILES +=                               \
        WhatevercastNameArrayMessage.pb.h WhatevercastNameArrayMessage.pb.cc WhatevercastNameArrayMessage.stamp

MOSTLYCLEANFILES +=                             \
        WhatevercastNameArrayMessage.tmp

EXTRA_DIST +=                                   \
        WhatevercastNameArrayMessage.proto

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

DISTCLEANFILES +=                               \
        WhatevercastNameMessage.pb.h WhatevercastNameMessage.pb.cc WhatevercastNameMessage.stamp

MOSTLYCLEANFILES +=                             \
        WhatevercastNameMessage.tmp

EXTRA_DIST +=                                   \
        WhatevercastNameMessage.proto

protoSOURCES += WhatevercastNameMessage.pb.cc WhatevercastNameMessage.pb.h

DIFAllocatorMessages.stamp: DIFAllocatorMessages.proto
	rm -f DIFAllocatorMessages.tmp
	touch DIFAllocatorMessages.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/DIFAllocatorMessages.proto
	mv -f DIFAllocatorMessages.tmp $@

DIFAllocatorMessages.pb.h DIFAllocatorMessages.pb.cc: DIFAllocatorMessages.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf DIFAllocatorMessages.lock DIFAllocatorMessages.stamp' 1 2 13 15; \
	  if mkdir DIFAllocatorMessages.lock 2>/dev/null; then \
	    rm -f DIFAllocatorMessages.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) DIFAllocatorMessages.stamp; \
	    result=$$?; rm -rf DIFAllocatorMessages.lock; exit $$result; \
	  else \
	    while test -d DIFAllocatorMessages.lock; do sleep 1; done; \
	    test -f DIFAllocatorMessages.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
	DIFAllocatorMessages.pb.h DIFAllocatorMessages.pb.cc DIFAllocatorMessages.stamp

MOSTLYCLEANFILES +=                             \
	DIFAllocatorMessages.tmp

EXTRA_DIST +=                                   \
	DIFAllocatorMessages.proto

protoSOURCES += DIFAllocatorMessages.pb.cc DIFAllocatorMessages.pb.h


