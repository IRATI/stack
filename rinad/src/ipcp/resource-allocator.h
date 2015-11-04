/*
 * Resource Allocator
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IPCP_RESOURCE_ALLOCATOR_HH
#define IPCP_RESOURCE_ALLOCATOR_HH

#include "ipcp/components.h"

namespace rinad {

class QoSCubeRIBObject: public rina::rib::RIBObj {
public:
	QoSCubeRIBObject(rina::QoSCube* cube);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	rina::QoSCube * qos_cube;

};

/// Representation of a set of QoS cubes in the RIB
class QoSCubesRIBObject: public IPCPRIBObj {
public:
	QoSCubesRIBObject(IPCProcess * ipc_process);
	const std::string& get_class() const {
		return class_name;
	};

	//Create
	void create(const rina::cdap_rib::con_handle_t &con,
		    const std::string& fqn,
		    const std::string& class_,
		    const rina::cdap_rib::filt_info_t &filt,
		    const int invoke_id,
		    const rina::ser_obj_t &obj_req,
		    rina::ser_obj_t &obj_reply,
		    rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;
};

class NextHopTEntryRIBObj: public rina::rib::RIBObj {
public:
	NextHopTEntryRIBObj(rina::RoutingTableEntry* entry);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	const static std::string parent_class_name;
	const static std::string parent_object_name;
	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	rina::RoutingTableEntry * rt_entry;
};

class PDUFTEntryRIBObj: public rina::rib::RIBObj {
public:
	PDUFTEntryRIBObj(rina::PDUForwardingTableEntry* entry);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	const static std::string parent_class_name;
	const static std::string parent_object_name;
	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	rina::PDUForwardingTableEntry * ft_entry;
};

class NMinusOneFlowManager: public INMinusOneFlowManager {
public:
	NMinusOneFlowManager();
	~NMinusOneFlowManager();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	void processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event);;
	std::list<int> getNMinusOneFlowsToNeighbour(unsigned int address);
	int getManagementFlowToNeighbour(unsigned int address);
	unsigned int numberOfFlowsToNeighbour(const std::string& apn,
			const std::string& api);

private:
	IPCProcess * ipc_process_;
	rina::FlowAcceptor * flow_acceptor_;
};

class IPCPFlowAcceptor : public rina::FlowAcceptor {
public:
	IPCPFlowAcceptor(IPCProcess * ipcp) : ipcp_(ipcp) { };
	~IPCPFlowAcceptor() { };
	bool accept_flow(const rina::FlowRequestEvent& event);

	IPCProcess * ipcp_;
};

class ResourceAllocator: public IResourceAllocator {
public:
	ResourceAllocator();
	~ResourceAllocator();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	INMinusOneFlowManager * get_n_minus_one_flow_manager() const;
	std::list<rina::QoSCube*> getQoSCubes();
	void addQoSCube(const rina::QoSCube& cube);

	std::list<rina::PDUForwardingTableEntry> get_pduft_entries();
	/// This operation takes ownership of the entries
	void set_pduft_entries(const std::list<rina::PDUForwardingTableEntry*>& pduft);

	std::list<rina::RoutingTableEntry> get_rt_entries();
	/// This operation takes ownership of the entries
	void set_rt_entries(const std::list<rina::RoutingTableEntry*>& rt);

private:
	/// Create initial RIB objects
	void populateRIB();

	INMinusOneFlowManager * n_minus_one_flow_manager_;
	IPCPRIBDaemon * rib_daemon_;
	rina::Lockable lock;
	rina::ThreadSafeMapOfPointers<std::string, rina::PDUForwardingTableEntry> pduft;
	rina::ThreadSafeMapOfPointers<std::string, rina::RoutingTableEntry> rt;
};

} //namespace rinad

#endif //IPCP_RESOURCE_ALLOCATOR_HH
