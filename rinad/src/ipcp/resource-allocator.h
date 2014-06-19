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

#ifdef __cplusplus

#include "ipcp/components.h"

namespace rinad {

class NMinusOneFlowManager: public INMinusOneFlowManager {
public:
	NMinusOneFlowManager();
	void set_ipc_process(IPCProcess * ipc_process);

private:
	IPCProcess * ipc_process_;
	IRIBDaemon * rib_daemon_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;

	///Populate the IPC Process RIB with the objects related to N-1 Flow Management
	void populateRIB();
};

/// Registrations to N-1 DIFs
class DIFRegistrationSetRIBObject : public BaseRIBObject {
public:
	static const std::string DIF_REGISTRATION_SET_RIB_OBJECT_CLASS;
	static const std::string DIF_REGISTRATION_RIB_OBJECT_CLASS;
	static const std::string DIF_REGISTRATION_SET_RIB_OBJECT_NAME;

	DIFRegistrationSetRIBObject(IPCProcess * ipcProcess,
			INMinusOneFlowManager * nMinus1FlowManager);
    void createObject(const std::string& objectClass, const std::string& objectName,
    		void* objectValue) throw (Exception);
    void* get_value();

private:
    INMinusOneFlowManager * n_minus_one_flow_manager_;
};


}

#endif

#endif
