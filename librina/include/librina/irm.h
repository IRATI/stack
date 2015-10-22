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
#include "librina/rib_v2.h"

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
	void set_rib_handle(rina::rib::rib_handle_t rib_handle);
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
	rib::RIBDaemonProxy * rib_daemon_;
	rina::rib::rib_handle_t rib;
	InternalEventManager * event_manager_;
	FlowAcceptor * flow_acceptor_;

	/// true if the IRM is used by an IPC Process, false otherwise
	bool ipcp;

	///Populate the IPC Process RIB with the objects related to N-1 Flow Management
	void populateRIB();

	///Remove the N-1 flow object from the RIB and send an internal notification
	void cleanFlowAndNotify(int portId);
};

class UnderlayingRegistrationRIBObj: public rib::RIBObj {
public:
	UnderlayingRegistrationRIBObj(const std::string& dif_name_);
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name_prefix;
	const static std::string parent_class_name;
	const static std::string parent_object_name;

private:
	std::string dif_name;
};

class UnderlayingFlowRIBObj: public rib::RIBObj {
public:
	UnderlayingFlowRIBObj(const rina::FlowInformation& flow_info);
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name_prefix;
	const static std::string parent_class_name;
	const static std::string parent_object_name;

private:
	FlowInformation flow_information;
};

class UnderlayingDIFRIBObj: public rib::RIBObj {
public:
	UnderlayingDIFRIBObj(const rina::DIFProperties& dif_properties);
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name_prefix;
	const static std::string parent_class_name;
	const static std::string parent_object_name;

private:
	DIFProperties dif_properties;
};

}

#endif

#endif
