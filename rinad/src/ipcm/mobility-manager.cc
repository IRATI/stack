/*
 * Mobility Manager (manages mobility locally in a system)
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

#define RINA_PREFIX     "ipcm.mobility-manager"
#include <librina/logs.h>

#include "mobility-manager.h"
#include "ipcm.h"

using namespace std;

namespace rinad {

//Class Mobility Manager
const std::string MobilityManager::NAME = "mobman";

MobilityManager::MobilityManager(IPCMIPCProcessFactory * ipcp_factory) :
		Addon(MobilityManager::NAME)
{
	factory = ipcp_factory;
	LOG_INFO("Mobility Manager initialized");

	TriggerHandoverTimerTask * task = new TriggerHandoverTimerTask(this, &timer);
	timer.scheduleTask(task, 10000);
}

void MobilityManager::media_reports_handler(rina::MediaReportEvent * event)
{
	LOG_DBG("Received new media report %s", event->media_report.toString().c_str());
}

int MobilityManager::execute_handover(unsigned short ipcp_id,
		      	      	      const rina::ApplicationProcessNamingInformation& dif_name,
				      const rina::ApplicationProcessNamingInformation& neigh_name)
{
	IPCMIPCProcess * ipcp;
	IPCMIPCProcess * reg_ipcp;
	std::list<rina::ApplicationProcessNamingInformation>::iterator it;
	std::list<IPCMIPCProcess*> affected_ipcps;
	std::list<IPCMIPCProcess*>::iterator ipcp_it;
	std::list<rina::ApplicationProcessNamingInformation> neigh_names;
	std::list<rina::ApplicationProcessNamingInformation>::iterator neighs_it;
	Promise promise;
	NeighborData neigh_data;
	NeighborData high_neigh_data;

	// Prevent any insertion or deletion to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	ipcp = factory->getIPCProcess(ipcp_id);
	if (ipcp == NULL) {
		LOG_ERR("Could not find IPCP %u", ipcp_id);
		return -1;
	}

	if (ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_AP &&
			ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", ipcp->get_type().c_str());
		return -1;
	}

	if (ipcp->get_state() != IPCMIPCProcess::IPCM_IPCP_ASSIGNED_TO_DIF) {
		LOG_ERR("Wrong IPCP state: %d", ipcp->get_state());
		return -1;
	}

	// Get IPCPs that are registered at the shim, disconnect them from the neighbors
	for (it = ipcp->registeredApplications.begin();
			it!= ipcp->registeredApplications.end(); ++it) {
		reg_ipcp = factory->getIPCProcessByName(*it);
		if (!reg_ipcp || reg_ipcp->get_state() != IPCMIPCProcess::IPCM_IPCP_ASSIGNED_TO_DIF)
			continue;

		neigh_names = reg_ipcp->get_neighbors_with_n1dif(ipcp->dif_name_);
		if (neigh_names.size() == 0)
			continue;

		for (neighs_it = neigh_names.begin(); neighs_it != neigh_names.end(); ++neighs_it) {
			if(IPCManager->disconnect_neighbor(this, &promise, reg_ipcp->get_id(), *neighs_it) == IPCM_FAILURE ||
					promise.wait() != IPCM_SUCCESS) {
				LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
						reg_ipcp->get_id());
			}
		}

		affected_ipcps.push_back(reg_ipcp);
	}

	//Enroll the shim to the new DIF/AP (handover)
	neigh_data.apName = neigh_name;
	neigh_data.difName = dif_name;
	if(IPCManager->enroll_to_dif(this, &promise, ipcp_id, neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
			 ipcp_id,
			 dif_name.processName.c_str(),
			 neigh_name.processName.c_str());
		return -1;
	}

	//Get IPCPs that are registered to the shim, enroll again
	for(ipcp_it = affected_ipcps.begin();
			ipcp_it != affected_ipcps.end(); ++ ipcp_it) {
		high_neigh_data.supportingDifName = ipcp->dif_name_;
		high_neigh_data.difName = (*ipcp_it)->dif_name_;

		if(IPCManager->enroll_to_dif(this, &promise, (*ipcp_it)->get_id(), high_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					ipcp_id,
					high_neigh_data.difName.processName.c_str(),
					high_neigh_data.supportingDifName.processName.c_str());
			continue;
		}
	}

	return 0;
}

TriggerHandoverTimerTask::TriggerHandoverTimerTask(MobilityManager * mobman,
			 	 	 	   rina::Timer * tim)
{
	mobility_manager = mobman;
	timer = tim;
}

void TriggerHandoverTimerTask::run()
{
	rina::ApplicationProcessNamingInformation dif_name;
	rina::ApplicationProcessNamingInformation neigh_name;
	dif_name.processName = "0";
	neigh_name.processName = "a5:cf:23:8d:29:56";

	mobility_manager->execute_handover(1, dif_name, neigh_name);

	//Re-schedule
	TriggerHandoverTimerTask * task = new TriggerHandoverTimerTask(mobility_manager, timer);
	timer->scheduleTask(task, 20000);
}

} //namespace rinad
