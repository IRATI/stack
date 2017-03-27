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

#include <fstream>
#include <sstream>
#include <sys/time.h>

#define RINA_PREFIX     "ipcm.mobility-manager"
#include <librina/logs.h>
#include <librina/json/json.h>
#include <librina/timer.h>
#include <debug.h>

#include "mobility-manager.h"
#include "../ipcm.h"

using namespace std;

namespace rinad {

//Class Mobility Manager
const std::string MobilityManager::NAME = "mobman";

void MobilityManager::parse_configuration(const rinad::RINAConfiguration& config)
{
	Json::Reader    reader;
	Json::Value     root;
        Json::Value     mobman_conf;
	std::ifstream   fin;

	fin.open(config.configuration_file.c_str());
	if (fin.fail()) {
		LOG_ERR("Failed to open config file");
		return;
	}

	if (!reader.parse(fin, root, false)) {
		std::stringstream ss;

		ss << "Failed to parse JSON configuration" << std::endl
			<< reader.getFormatedErrorMessages() << std::endl;
		FLUSH_LOG(ERR, ss);

		return;
	}

	fin.close();

	mobman_conf = root["addons"]["mobman"];
        if (mobman_conf != 0) {
        	Json::Value wireless_difs = mobman_conf["wirelessDIFs"];

        	if (wireless_difs != 0) {
        		std::string dif_name;
        		std::string bs_address;
        		for (unsigned i = 0; i< wireless_difs.size(); i++) {
        			WirelessDIFInfo info;

        			dif_name = wireless_difs[i].get("difName",
        							dif_name).asString();
        			bs_address = wireless_difs[i].get("bsAddress",
        							  bs_address).asString();

        			info.dif_name = dif_name;
        			info.bs_address = bs_address;
        			wireless_dif_info.push_back(info);
        		}
        	}

                LOG_INFO("Mobility Manager configuration parsed, found %d wireless DIFs",
                	 wireless_dif_info.size());
        }

        if (wireless_dif_info.size() == 0) {
        	LOG_INFO("No wireless DIF info specified adding default one: "
        			"irati (net_name) 1c:65:9d:22:4e:a8 (bs_address)");

        	WirelessDIFInfo info;
        	info.dif_name = "irati";
        	info.bs_address = "1c:65:9d:22:4e:a8";
        	wireless_dif_info.push_back(info);
        }
}

MobilityManager::MobilityManager(const rinad::RINAConfiguration& config) :
		AppAddon(MobilityManager::NAME)
{
	parse_configuration(config);
	factory = IPCManager->get_ipcp_factory();

	LOG_INFO("Mobility Manager initialized");

	TriggerHandoverTimerTask * task = new TriggerHandoverTimerTask(this,
								       &timer,
								       wireless_dif_info.front());
	timer.scheduleTask(task, 10000);
}

void MobilityManager::process_librina_event(rina::IPCEvent** event_)
{
	rina::IPCEvent* event = *event_;

	if (!event)
		return;

	switch (event->eventType) {
	case rina::IPCM_MEDIA_REPORT_EVENT:
		media_reports_handler(dynamic_cast<rina::MediaReportEvent*>(event));
		break;

	default:
		//Ignore, I'm not interested in this event
		break;
	}

	delete event;
	*event_ = NULL;
}

void MobilityManager::process_ipcm_event(const IPCMEvent& event)
{
	//Ignore we are not interested in them
	(void) event;
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
	int t0, t1, t2, t3;

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

	t0 = rina::Time::get_time_in_ms();

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

	t1 = rina::Time::get_time_in_ms();

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

	t2 = rina::Time::get_time_in_ms();

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

	t3 = rina::Time::get_time_in_ms();
	LOG_WARN("Handover completed in %d ms", (t3-t0));
	LOG_WARN("Partial times: %d, %d, %d", (t1-t0), (t2-t1), (t3-t2));

	return 0;
}

// Class TriggerHandoverTimerTask
TriggerHandoverTimerTask::TriggerHandoverTimerTask(MobilityManager * mobman,
			 	 	 	   rina::Timer * tim,
						   const WirelessDIFInfo& info)
{
	mobility_manager = mobman;
	timer = tim;
	dif_info = info;
}

void TriggerHandoverTimerTask::run()
{
	rina::ApplicationProcessNamingInformation dif_name;
	rina::ApplicationProcessNamingInformation neigh_name;
	dif_name.processName = dif_info.dif_name;
	neigh_name.processName = dif_info.bs_address;

	mobility_manager->execute_handover(1, dif_name, neigh_name);

	//Re-schedule
	TriggerHandoverTimerTask * task = new TriggerHandoverTimerTask(mobility_manager,
								       timer,
								       dif_info);
	timer->scheduleTask(task, 20000);
}

} //namespace rinad
