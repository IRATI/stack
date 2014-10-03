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
        void processSetPolicySetParamRequestEvent(
                const rina::SetPolicySetParamRequestEvent& event);
        void processSetPolicySetParamResponseEvent(
                const rina::SetPolicySetParamResponseEvent& event);

private:
	void init_cdap_session_manager();
	void init_encoder();

	IPCProcessOperationalState state;
	std::map<unsigned int, rina::AssignToDIFRequestEvent> pending_events_;
        std::map<unsigned int, rina::SetPolicySetParamRequestEvent>
                pending_set_policy_set_param_events;
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
