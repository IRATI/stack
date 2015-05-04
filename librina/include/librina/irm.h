/*
 * IPC Resource Manager
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_IRM_H
#define LIBRINA_IRM_H

#ifdef __cplusplus

#include "librina/application.h"
#include "librina/internal-events.h"
#include "librina/rib.h"

namespace rina {

/// Decides wether accept or reject a flow
class FlowAcceptor {
public:
	virtual ~FlowAcceptor() { };
	virtual bool accept_flow(const FlowRequestEvent& event) = 0;
};

/// The IPC Resource Manager (IRM) provides the policy coordination among
/// the elements of IPC Management of a DAF. The primary role of the IRM
/// is managing the use of supporting DIFs and in some cases, participate
/// in creating new DIFs (for more detail, see Part 2, Chapter 2)
class IPCResourceManager: public ApplicationEntity {
public:
	IPCResourceManager();
	IPCResourceManager(bool isIPCP);
	virtual ~IPCResourceManager() { };
	virtual void set_application_process(ApplicationProcess * ap);
	void set_flow_acceptor(FlowAcceptor * fa);
	unsigned int allocateNMinus1Flow(const FlowInformation& flowInformation);
	void allocateRequestResult(const AllocateFlowRequestResultEvent& event);
	void flowAllocationRequested(const FlowRequestEvent& event);
	void deallocateNMinus1Flow(int portId);
	void deallocateFlowResponse(const DeallocateFlowResponseEvent& event);
	void flowDeallocatedRemotely(const FlowDeallocatedEvent& event);
	FlowInformation getNMinus1FlowInformation(int portId) const;
	bool isSupportingDIF(const ApplicationProcessNamingInformation& difName);
	std::list<FlowInformation> getAllNMinusOneFlowInformation() const;

protected:
	IRIBDaemon * rib_daemon_;
	CDAPSessionManagerInterface * cdap_session_manager_;
	InternalEventManager * event_manager_;
	FlowAcceptor * flow_acceptor_;

	/// true if the IRM is used by an IPC Process, false otherwise
	bool ipcp;

	///Populate the IPC Process RIB with the objects related to N-1 Flow Management
	void populateRIB();

	///Remove the N-1 flow object from the RIB and send an internal notification
	void cleanFlowAndNotify(int portId);
};

class DIFRegistrationRIBObject: public SimpleSetMemberRIBObject {
public:
	DIFRegistrationRIBObject(IRIBDaemon * rib_daemon,
				 const std::string& object_class,
				 const std::string& object_name,
				 const std::string* dif_name);
	std::string get_displayable_value();
};

class DIFRegistrationSetRIBObject: public BaseRIBObject {
public:
	static const std::string DIF_REGISTRATION_SET_RIB_OBJECT_CLASS;
	static const std::string DIF_REGISTRATION_RIB_OBJECT_CLASS;
	static const std::string DIF_REGISTRATION_SET_RIB_OBJECT_NAME;

	DIFRegistrationSetRIBObject(IRIBDaemon * rib_daemon);
	const void* get_value() const;
	void createObject(const std::string& objectClass,
                          const std::string& objectName,
                          const void* objectValue);
};

class NMinusOneFlowRIBObject: public SimpleSetMemberRIBObject {
public:
	NMinusOneFlowRIBObject(IRIBDaemon * rib_daemon,
			       const std::string& object_class,
			       const std::string& object_name,
			       const rina::FlowInformation* flow_info);
	std::string get_displayable_value();
};

class NMinusOneFlowSetRIBObject: public BaseRIBObject {
public:
	static const std::string N_MINUS_ONE_FLOW_SET_RIB_OBJECT_CLASS;
	static const std::string N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS;
	static const std::string N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME;

	NMinusOneFlowSetRIBObject(IRIBDaemon * rib_daemon);
	const void* get_value() const;
	void createObject(const std::string& objectClass,
                          const std::string& objectName,
                          const void* objectValue);
};

}

#endif

#endif
