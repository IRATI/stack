/*
 * IP VPN Manager. Manages creation and deletion of IP over RINA flows
 * and VPNs. Manages registration of IP prefixes as application names
 * in RINA DIFs, as well as allocation of flows between IP prefixes,
 * the creation of VPNs and the reconfiguration of the IP forwarding table.
 *
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

#define RINA_PREFIX     "ipcm.ip-vpn-manager"
#include <librina/logs.h>

#include "ip-vpn-manager.h"
#include "ipcm.h"

using namespace std;


namespace rinad {

ipcm_res_t IPCManager_::register_ip_range_to_dif(Addon* callee,
			   	    	    	 Promise* promise,
						 const std::string& ip_range,
						 const rina::ApplicationProcessNamingInformation& dif_name)
{
	IPCMIPCProcess *slave_ipcp = NULL;
	APPregTransState* trans = NULL;
	rina::ApplicationRegistrationRequestEvent event;
	rina::ApplicationProcessNamingInformation daf_name;

	// Select an IPC process from the DIF specified in the request
	slave_ipcp = select_ipcp_by_dif(dif_name);
	if (!slave_ipcp) {
		LOG_ERR("Cannot find a suitable DIF to "
			"register IP range %s ", ip_range.c_str());
		return IPCM_FAILURE;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(slave_ipcp->rwlock, false);

	//Perform the registration
	try {
		//Create a transaction
		trans = new APPregTransState(NULL, promise, slave_ipcp->get_id(), event);
		if(!trans){
			LOG_ERR("Unable to allocate memory for the transaction object. Out of memory! ");
			return IPCM_FAILURE;
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			LOG_ERR("Unable to add transaction; out of memory? ");
			return IPCM_FAILURE;
		}

		event.applicationRegistrationInformation.appName.processName = ip_range;
		event.applicationRegistrationInformation.appName.entityName = RINA_IP_FLOW_ENT_NAME;
		slave_ipcp->registerApplication(event.applicationRegistrationInformation.appName,
						daf_name,
						0,
						trans->tid);

		LOG_INFO("Requested registration of IP range %s"
				" to IPC process %s ",
				ip_range.c_str(),
				slave_ipcp->get_name().toString().c_str());
	} catch (rina::IpcmRegisterApplicationException& e) {
		//Remove the transaction and return
		remove_transaction_state(trans->tid);

		LOG_ERR("Error while registering application "
			 , app_name.toString().c_str());
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

} //namespace rinad
