/*
 * Implementation of the IPC Process
 *
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

#ifndef IPCP_IPC_PROCESS_HH
#define IPCP_IPC_PROCESS_HH

#ifdef __cplusplus

#include <map>

#include "common/event-loop.h"
#include "ipcp/components.h"

namespace rinad {

class IPCProcessImpl: public IPCProcess, public EventLoopData {
public:
	IPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
			unsigned short id, unsigned int ipc_manager_port);
	~IPCProcessImpl();
	unsigned short get_id();
	const rina::ApplicationProcessNamingInformation& get_name() const;
	IDelimiter * get_delimiter();
	Encoder * get_encoder();
	rina::CDAPSessionManagerInterface* get_cdap_session_manager();
	IEnrollmentTask * get_enrollment_task();
	IFlowAllocator * get_flow_allocator();
	INamespaceManager * get_namespace_manager();
	IResourceAllocator * get_resource_allocator();
	ISecurityManager * get_security_manager();
	IRIBDaemon * get_rib_daemon();
	const std::list<rina::Neighbor*> get_neighbors() const;
	const IPCProcessOperationalState& get_operational_state() const;
	void set_operational_state(const IPCProcessOperationalState& operational_state);
	const rina::DIFInformation& get_dif_information() const;
	void set_dif_information(const rina::DIFInformation& dif_information);
	unsigned int get_address() const;
	void set_address(unsigned int address);
	unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name);
	void processAssignToDIFRequestEvent(const rina::AssignToDIFRequestEvent& event);
	void processAssignToDIFResponseEvent(const rina::AssignToDIFResponseEvent& event);
	void requestPDUFTEDump();
	void logPDUFTE(const rina::DumpFTResponseEvent& event);

	IDelimiter * delimiter_;
	Encoder * encoder_;
	rina::CDAPSessionManagerInterface* cdap_session_manager_;
	IEnrollmentTask * enrollment_task_;
	IFlowAllocator * flow_allocator_;
	INamespaceManager * namespace_manager_;
	IResourceAllocator * resource_allocator_;
	ISecurityManager * security_manager_;
	IRIBDaemon * rib_daemon_;

private:
	void init_delimiter();
	void init_cdap_session_manager();
	void init_encoder();
	void init_enrollment_task();
	void init_flow_allocator();
	void init_namespace_manager();
	void init_resource_allocator();
	void init_security_manager();
	void init_rib_daemon();
	void populate_rib();

	rina::ApplicationProcessNamingInformation name_;
	IPCProcessOperationalState state_;
	std::map<unsigned int, rina::AssignToDIFRequestEvent> pending_events_;
	rina::Lockable * lock_;
	rina::DIFInformation dif_information_;
};

void register_handlers_all(EventLoop& loop);

}

/* Macro useful to perform downcasts in declarations. */
#define DOWNCAST_DECL(_var,_class,_name)        \
        _class *_name = dynamic_cast<_class*>(_var);

#endif

#endif
