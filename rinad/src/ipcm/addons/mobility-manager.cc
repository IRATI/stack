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
	hand_state.do_it_now = false;
	hand_state.current_dif = "";

	LOG_INFO("Mobility Manager initialized");
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

	rina::ScopedLock g(lock);

	if (!hand_state.do_it_now) {
		hand_state.do_it_now = true;
		return;
	}

	hand_state.do_it_now = false;
	execute_handover(event->media_report);
}

void MobilityManager::execute_handover(const rina::MediaReport& report)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, high_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * normal_ipcp, * next_ipcp, * current_ipcp;
	Promise promise;
	std::string next_dif;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get all IPCPS
	wifi1_ipcp = factory->getIPCProcess(1);
	if (wifi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		return;
	}

	normal_ipcp = factory->getIPCProcess(3);
	if (normal_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return;
	}

	if (normal_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", normal_ipcp->get_type().c_str());
		return;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.current_dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = report.available_difs.find("arcfire");
		if (difs_it == report.available_difs.end()) {
			LOG_WARN("No members of DIF 'arcfire' are within range");
			return;
		}

		bs_info = difs_it->second.available_bs_ipcps.front();

		//Enroll the shim to the new DIF/AP
		neigh_data.apName.processName = bs_info.ipcp_address;
		neigh_data.difName.processName = "arcfire";
		if(IPCManager->enroll_to_dif(this, &promise, wifi1_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
				 wifi1_ipcp->get_id(),
				 neigh_data.difName.processName.c_str(),
				 bs_info.ipcp_address.c_str());
			return;
		}

		//Enroll to DIF
		high_neigh_data.supportingDifName.processName = "arcfire";
		high_neigh_data.difName.processName = "mobile.DIF";

		if(IPCManager->enroll_to_dif(this, &promise, normal_ipcp->get_id(), high_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					normal_ipcp->get_id(),
					high_neigh_data.difName.processName.c_str(),
					high_neigh_data.supportingDifName.processName.c_str());
			return;
		}

		hand_state.current_dif = "arcfire";
		sleep.sleepForMili(1000);
		LOG_DBG("Initialized");
	}

	if (hand_state.current_dif == "arcfire") {
		next_dif = "pristine";
		next_ipcp = wifi2_ipcp;
		current_ipcp = wifi1_ipcp;
		neighbor.processName = "ap1.mobile";
		neighbor.processInstance = "1";
	} else {
		next_dif = "arcfire";
		next_ipcp = wifi1_ipcp;
		current_ipcp = wifi2_ipcp;
		neighbor.processName = "ap2.mobile";
		neighbor.processInstance = "1";
	}

	// 3. Enroll to next DIF (become multihomed)
	difs_it = report.available_difs.find(next_dif);
	if (difs_it == report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", next_dif.c_str());
		return;
	}

	bs_info = difs_it->second.available_bs_ipcps.front();

	//Enroll the shim to the new DIF/AP
	neigh_data.apName.processName = bs_info.ipcp_address;
	neigh_data.difName.processName = next_dif;
	if(IPCManager->enroll_to_dif(this, &promise, next_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
			  next_ipcp->get_id(),
			 neigh_data.difName.processName.c_str(),
			 bs_info.ipcp_address.c_str());
		return;
	}

	//Enroll to DIF
	high_neigh_data.supportingDifName.processName = next_dif;
	high_neigh_data.difName.processName = "mobile.DIF";

	if(IPCManager->enroll_to_dif(this, &promise, normal_ipcp->get_id(), high_neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
				normal_ipcp->get_id(),
				high_neigh_data.difName.processName.c_str(),
				high_neigh_data.supportingDifName.processName.c_str());
		return;
	}

	//4 Now it is multihomed, wait 5 seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(5000);

	hand_state.current_dif = next_dif;

	if(IPCManager->disconnect_neighbor(this, &promise, normal_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				normal_ipcp->get_id());
		return;
	}

	if(IPCManager->disconnect_neighbor(this, &promise, current_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				current_ipcp->get_id());
		return;
	}

	LOG_DBG("Handover done!");
}

} //namespace rinad
