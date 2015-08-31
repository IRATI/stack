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

#include <map>

#include "ipcp/components.h"

namespace rinad {

class IPCProcessImpl: public IPCProcess {
public:
        IPCProcessImpl(const rina::ApplicationProcessNamingInformation& name,
                        unsigned short id, unsigned int ipc_manager_port,
                        std::string log_level, std::string log_file);
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

	// Policy Management
        int dispatchSelectPolicySet(const std::string& path,
                                    const std::string& name,
                                    bool& got_in_userspace);
        void processSetPolicySetParamRequestEvent(
                const rina::SetPolicySetParamRequestEvent& event);
        void processSetPolicySetParamResponseEvent(
                const rina::SetPolicySetParamResponseEvent& event);
        void processSelectPolicySetRequestEvent(
                const rina::SelectPolicySetRequestEvent& event);
        void processSelectPolicySetResponseEvent(
                const rina::SelectPolicySetResponseEvent& event);
        void processPluginLoadRequestEvent(
                const rina::PluginLoadRequestEvent& event);
        void processFwdCDAPMsgEvent(
                const rina::FwdCDAPMsgEvent& event);


	//Event loop (run)
	void event_loop(void);

private:
        void init_cdap_session_manager();
		void init_encoder();

        IPCProcessOperationalState state;
		std::map<unsigned int, rina::AssignToDIFRequestEvent> pending_events_;
        std::map<unsigned int, rina::SetPolicySetParamRequestEvent>
                pending_set_policy_set_param_events;
        std::map<unsigned int, rina::SelectPolicySetRequestEvent>
                pending_select_policy_set_events;
        rina::Lockable * lock_;
		rina::DIFInformation dif_information_;
};

} //namespace rinad

#endif //IPCP_IPC_PROCESS_HH
