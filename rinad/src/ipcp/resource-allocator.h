/*
 * Resource Allocator
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

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

class RMTN1Flow {
public:
	RMTN1Flow(int pid, unsigned short ipcp) : ipcp_id(ipcp),
		port_id(pid), enabled(true), queued_pdus(0),
		dropped_pdus(0), error_pdus(0), tx_pdus(0),
		rx_pdus(0), tx_bytes(0), rx_bytes(0) { };

	void sync_with_kernel();

	unsigned short ipcp_id;
	int port_id;
	bool enabled;
	unsigned int queued_pdus;
	unsigned int dropped_pdus;
	unsigned int error_pdus;
	unsigned int tx_pdus;
	unsigned int rx_pdus;
	unsigned long tx_bytes;
	unsigned long rx_bytes;
};

class RMTN1FlowRIBObj: public rina::rib::RIBObj {
public:
	RMTN1FlowRIBObj(RMTN1Flow * flow);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	RMTN1Flow * n1_flow;
};

class NextHopTEntryRIBObj: public rina::rib::RIBObj {
public:
	NextHopTEntryRIBObj(rina::RoutingTableEntry* entry);
	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

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

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

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
	void set_dif_configuration(const rina::DIFInformation& dif_information);
	void processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event);;
	std::list<int> getNMinusOneFlowsToNeighbour(unsigned int address);
	std::list<int> getNMinusOneFlowsToNeighbour(const std::string& name);
	std::list<int> getManagementFlowsToAllNeighbors(void);
	int getManagementFlowToNeighbour(const std::string& name);
	int getManagementFlowToNeighbour(unsigned int address);
	int get_n1flow_to_neighbor(const rina::FlowSpecification& fspec,
				   const std::string& name);
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

class ResourceAllocator: public IResourceAllocator, rina::InternalEventListener {
public:
	ResourceAllocator();
	~ResourceAllocator();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFInformation& dif_information);
	INMinusOneFlowManager * get_n_minus_one_flow_manager() const;
	std::list<rina::QoSCube*> getQoSCubes();
	void addQoSCube(const rina::QoSCube& cube);

	std::list<rina::PDUForwardingTableEntry> get_pduft_entries();
	/// This operation takes ownership of the entries
	void set_pduft_entries(const std::list<rina::PDUForwardingTableEntry*>& pduft);

	std::list<rina::RoutingTableEntry> get_rt_entries();
	/// This operation takes ownership of the entries
	void set_rt_entries(const std::list<rina::RoutingTableEntry*>& rt);
	int get_next_hop_addresses(unsigned int dest_address,
				   std::list<unsigned int>& addresses);
	int get_next_hop_name(const std::string& dest_name,
			      std::string& name);
	unsigned int get_n1_port_to_address(unsigned int dest_address);

	/// Add a temporary entry to the PDU FTE, until the routing policy
	/// provides it when it modifies the forwarding table.
	/// Takes ownership of the entry.
	void add_temp_pduft_entry(unsigned int dest_address, int port_id);
	void remove_temp_pduft_entry(unsigned int dest_address);

	void eventHappened(rina::InternalEvent * event);

	void sync_with_kernel();

private:
	/// Create initial RIB objects
	void populateRIB();
	void subscribeToEvents();

	/// Called by the RIB Daemon when the flow supporting the CDAP session with the remote peer
	/// has been deallocated
	/// @param event
	void nMinusOneFlowDeallocated(rina::NMinusOneFlowDeallocatedEvent  * event);

	/// Called when a new N-1 flow has been allocated
	// @param portId
	void nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * flowEvent);

	bool contains_temp_entry(unsigned int dest_address);
	bool entry_is_in_pduft(unsigned int dest_address);
	void update_temp_entries(void);

	INMinusOneFlowManager * n_minus_one_flow_manager_;
	IPCPRIBDaemon * rib_daemon_;
	rina::Lockable lock;

	std::list<rina::PDUForwardingTableEntry*> temp_entries;
	std::map<std::string, rina::PDUForwardingTableEntry *> pduft;
	rina::ReadWriteLockable pduft_lock;

	std::map<std::string, rina::RoutingTableEntry *> rt;
	rina::ReadWriteLockable rt_lock;

	std::map<int, RMTN1Flow *> n1_flows;
	rina::Lockable n1_flows_lock;
};

} //namespace rinad

#endif //IPCP_RESOURCE_ALLOCATOR_HH
