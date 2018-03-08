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
const int MobilityManager::DEFAULT_HANDOVER_TYPE = 4;
const unsigned int MobilityManager::DEFAULT_DISC_WAIT_TIME_MS = 5000;

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

	//TODO: Set a default and enable override via configuration
	hand_state.hand_type = DEFAULT_HANDOVER_TYPE;
	if (config.ipcProcessesToCreate.size() == 5) {
		hand_state.hand_type = 3;
	}
	hand_state.disc_wait_time_ms = DEFAULT_DISC_WAIT_TIME_MS;

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

        	Json::Value general_conf = mobman_conf["generalConf"];
        	if (general_conf != 0) {
        		hand_state.hand_type = general_conf.get("handType",
        				hand_state.hand_type).asInt();
        		hand_state.disc_wait_time_ms = general_conf.get("discWaitTime",
        				hand_state.disc_wait_time_ms).asInt();
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
	if (hand_state.hand_type == 2 || hand_state.hand_type == 3
			|| hand_state.hand_type == 4) {
		hand_state.do_it_now = false;
	} else {
		hand_state.do_it_now = true;
	}
	hand_state.dif = "";
	hand_state.ipcp = 0;

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

	if (hand_state.hand_type == 2 || hand_state.hand_type == 3
			|| hand_state.hand_type == 4) {
		hand_state.do_it_now = false;
	}

	HandoverTimerTask * task = new HandoverTimerTask(this, event->media_report);
	timer.scheduleTask(task, 0);
}

void MobilityManager::execute_handover(const rina::MediaReport& report)
{
	if (hand_state.hand_type == 1) {
		execute_handover1(report);
	} else if (hand_state.hand_type == 2) {
		execute_handover2(report);
	} else if (hand_state.hand_type == 3) {
		execute_handover3(report);
	} else if (hand_state.hand_type == 4) {
		execute_handover4(report);
	} else if (hand_state.hand_type == 5) {
		execute_handover5(report);
	}else {
		LOG_ERR("Unknown handover type: %d", hand_state.hand_type);
	}
}

void MobilityManager::execute_handover1(const rina::MediaReport& report)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, high_neigh_data;
	IPCMIPCProcess * wifi_ipcp, * normal_ipcp;
	Promise promise;
	std::string next_dif;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get all IPCPS
	wifi_ipcp = factory->getIPCProcess(1);
	if (wifi_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return;
	}

	if (wifi_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi_ipcp->get_type().c_str());
		return;
	}

	normal_ipcp = factory->getIPCProcess(2);
	if (normal_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return;
	}

	if (normal_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", normal_ipcp->get_type().c_str());
		return;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = report.available_difs.find("irati");
		if (difs_it == report.available_difs.end()) {
			LOG_WARN("No members of DIF 'irati' are within range");
			return;
		}

		bs_info = difs_it->second.available_bs_ipcps.front();

		//Enroll the shim to the new DIF/AP
		neigh_data.apName.processName = bs_info.ipcp_address;
		neigh_data.difName.processName = "irati";
		if(IPCManager->enroll_to_dif(this, &promise, wifi_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
				 wifi_ipcp->get_id(),
				 neigh_data.difName.processName.c_str(),
				 bs_info.ipcp_address.c_str());
			return;
		}

		//Enroll to DIF
		high_neigh_data.supportingDifName.processName = "irati";
		high_neigh_data.difName.processName = "mobile.DIF";

		if(IPCManager->enroll_to_dif(this, &promise, normal_ipcp->get_id(), high_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					normal_ipcp->get_id(),
					high_neigh_data.difName.processName.c_str(),
					high_neigh_data.supportingDifName.processName.c_str());
			return;
		}

		hand_state.dif = "irati";
		hand_state.ipcp = wifi_ipcp;
		sleep.sleepForMili(1000);
		LOG_DBG("Initialized");
	}

	//3 Update DIF name (irati -> pristine -> arcfire and start with irati again)
	if (hand_state.dif == "irati") {
		next_dif = "pristine";
		neighbor.processName = "ap1.mobile";
		neighbor.processInstance = "1";
	} else if (hand_state.dif == "pristine") {
		next_dif = "arcfire";
		neighbor.processName = "ap2.mobile";
		neighbor.processInstance = "1";
	}else {
		next_dif = "irati";
		neighbor.processName = "ap3.mobile";
		neighbor.processInstance = "1";
	}

	// 4. Disconnect from current neighbor
	if(IPCManager->disconnect_neighbor(this, &promise, normal_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				normal_ipcp->get_id());
		return;
	}

	if(IPCManager->disconnect_neighbor(this, &promise, wifi_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				wifi_ipcp->get_id());
		return;
	}

	// 5. Enroll to next DIF
	difs_it = report.available_difs.find(next_dif);
	if (difs_it == report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", next_dif.c_str());
		return;
	}

	bs_info = difs_it->second.available_bs_ipcps.front();

	//Enroll the shim to the new DIF/AP
	neigh_data.apName.processName = bs_info.ipcp_address;
	neigh_data.difName.processName = next_dif;
	if(IPCManager->enroll_to_dif(this, &promise, wifi_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
			  wifi_ipcp->get_id(),
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

	LOG_DBG("Handover done!");
}

void MobilityManager::execute_handover2(const rina::MediaReport& report)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, high_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * normal_ipcp, * ipcp_enroll, * ipcp_disc;
	Promise promise;
	std::string next_dif, disc_dif;
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
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = report.available_difs.find("irati");
		if (difs_it == report.available_difs.end()) {
			LOG_WARN("No members of DIF 'irati' are within range");
			return;
		}

		bs_info = difs_it->second.available_bs_ipcps.front();

		//Enroll the shim to the new DIF/AP
		neigh_data.apName.processName = bs_info.ipcp_address;
		neigh_data.difName.processName = "irati";
		if(IPCManager->enroll_to_dif(this, &promise, wifi1_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
				 wifi1_ipcp->get_id(),
				 neigh_data.difName.processName.c_str(),
				 bs_info.ipcp_address.c_str());
			return;
		}

		//Enroll to DIF
		high_neigh_data.supportingDifName.processName = "irati";
		high_neigh_data.difName.processName = "mobile.DIF";

		if(IPCManager->enroll_to_dif(this, &promise, normal_ipcp->get_id(), high_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					normal_ipcp->get_id(),
					high_neigh_data.difName.processName.c_str(),
					high_neigh_data.supportingDifName.processName.c_str());
			return;
		}

		hand_state.dif = "irati";
		hand_state.ipcp = wifi1_ipcp;
		sleep.sleepForMili(1000);
		LOG_DBG("Initialized");
	}

	//3 Update wifi IPCPs to use
	if (hand_state.ipcp == wifi1_ipcp) {
		ipcp_enroll = wifi2_ipcp;
		ipcp_disc = wifi1_ipcp;
	} else {
		ipcp_enroll = wifi1_ipcp;
		ipcp_disc = wifi2_ipcp;
	}

	//4 Update DIF name (irati -> pristine -> arcfire and start with irati again)
	disc_dif = hand_state.dif;
	if (hand_state.dif == "irati") {
		next_dif = "pristine";
		neighbor.processName = "ap1.mobile";
		neighbor.processInstance = "1";
	} else if (hand_state.dif == "pristine") {
		next_dif = "arcfire";
		neighbor.processName = "ap2.mobile";
		neighbor.processInstance = "1";
	}else {
		next_dif = "irati";
		neighbor.processName = "ap3.mobile";
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
	if(IPCManager->enroll_to_dif(this, &promise, ipcp_enroll->get_id(), neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
			  ipcp_enroll->get_id(),
			 neigh_data.difName.processName.c_str(),
			 bs_info.ipcp_address.c_str());
		return;
	}

	//Enroll to DIF
	high_neigh_data.supportingDifName.processName = next_dif;
	high_neigh_data.difName.processName = "mobile.DIF";

	if(IPCManager->enroll_to_dif(this, &promise, normal_ipcp->get_id(), high_neigh_data, true, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
				normal_ipcp->get_id(),
				high_neigh_data.difName.processName.c_str(),
				high_neigh_data.supportingDifName.processName.c_str());
		return;
	}

	//4 Now it is multihomed, wait a specified time and break connectivity through former N-1 DIF
	sleep.sleepForMili(hand_state.disc_wait_time_ms);

	hand_state.dif = next_dif;
	hand_state.ipcp = ipcp_enroll;

	if(IPCManager->disconnect_neighbor(this, &promise, normal_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				normal_ipcp->get_id());
		return;
	}

	difs_it = report.available_difs.find(disc_dif);
	if (difs_it == report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", disc_dif.c_str());
	} else {
		bs_info = difs_it->second.available_bs_ipcps.front();
	}

	neighbor.processName = bs_info.ipcp_address;
	neighbor.processInstance = "";

	if(IPCManager->disconnect_neighbor(this, &promise, ipcp_disc->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				ipcp_disc->get_id());
		return;
	}

	LOG_DBG("Handover done!");
}

//Roaming UE, two provider scenario with static DIF Allocator
void MobilityManager::execute_handover3(const rina::MediaReport& report)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data, int_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * mobi1_ipcp;
	IPCMIPCProcess * mobi2_ipcp, * inet_ipcp, *ipcp_enroll;
	IPCMIPCProcess * ipcp_disc, * mob_ipcp_enroll, * mob_ipcp_disc;
	Promise promise;
	std::string next_dif, disc_dif, next_mob_dif, dic_mob_dif;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor, int_neighbor;

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

	mobi1_ipcp = factory->getIPCProcess(3);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return;
	}

	mobi2_ipcp = factory->getIPCProcess(4);
	if (mobi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 4");
		return;
	}

	if (mobi2_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi2_ipcp->get_type().c_str());
		return;
	}

	inet_ipcp = factory->getIPCProcess(5);
	if (inet_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 5");
		return;
	}

	if (inet_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", inet_ipcp->get_type().c_str());
		return;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = report.available_difs.find("irati");
		if (difs_it == report.available_difs.end()) {
			LOG_WARN("No members of DIF 'irati' are within range");
			return;
		}

		bs_info = difs_it->second.available_bs_ipcps.front();

		//Enroll the shim to the new DIF/AP
		neigh_data.apName.processName = bs_info.ipcp_address;
		neigh_data.difName.processName = "irati";
		if(IPCManager->enroll_to_dif(this, &promise, wifi1_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
				 wifi1_ipcp->get_id(),
				 neigh_data.difName.processName.c_str(),
				 bs_info.ipcp_address.c_str());
			return;
		}

		//Enroll to the mobile DIF
		mob_neigh_data.supportingDifName.processName = "irati";
		mob_neigh_data.difName.processName = "mobile.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, mobi1_ipcp->get_id(), mob_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					mobi1_ipcp->get_id(),
					mob_neigh_data.difName.processName.c_str(),
					mob_neigh_data.supportingDifName.processName.c_str());
			return;
		}

		sleep.sleepForMili(1000);

		//Enroll to the internet DIF
		int_neigh_data.supportingDifName.processName = "mobile.DIF";
		int_neigh_data.difName.processName = "internet.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, inet_ipcp->get_id(), int_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					inet_ipcp->get_id(),
					int_neigh_data.difName.processName.c_str(),
					int_neigh_data.supportingDifName.processName.c_str());
			return;
		}

		hand_state.dif = "irati";
		hand_state.ipcp = wifi1_ipcp;

		LOG_DBG("Initialized");

		return;
	}

	//3 Update wifi IPCPs to use
	if (hand_state.ipcp == wifi1_ipcp) {
		ipcp_enroll = wifi2_ipcp;
		ipcp_disc = wifi1_ipcp;
	} else {
		ipcp_enroll = wifi1_ipcp;
		ipcp_disc = wifi2_ipcp;
	}

	//4 Update DIF name (irati -> pristine -> arcfire -> irina
	// -> rinaisense -> ocarina and start with irati again)
	disc_dif = hand_state.dif;
	if (hand_state.dif == "irati") {
		next_dif = "pristine";
		next_mob_dif = "mobile.DIF";
		neighbor.processName = "ap1.mobile";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = false;
		mob_ipcp_enroll = mobi1_ipcp;
		mob_ipcp_disc = mobi1_ipcp;
	} else if (hand_state.dif == "pristine") {
		next_dif = "arcfire";
		next_mob_dif = "mobile.DIF";
		neighbor.processName = "ap2.mobile";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = false;
		mob_ipcp_enroll = mobi1_ipcp;
		mob_ipcp_disc = mobi1_ipcp;
	} else if (hand_state.dif == "arcfire"){
		next_dif = "irina";
		next_mob_dif = "mobile2.DIF";
		neighbor.processName = "ap3.mobile";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = true;
		mob_ipcp_enroll = mobi2_ipcp;
		mob_ipcp_disc = mobi1_ipcp;
		int_neighbor.processName = "core1.internet";
		int_neighbor.processInstance = "1";
	} else if (hand_state.dif == "irina"){
		next_dif = "rinaisense";
		next_mob_dif = "mobile2.DIF";
		neighbor.processName = "ap4.mobile2";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = false;
		mob_ipcp_enroll = mobi2_ipcp;
		mob_ipcp_disc = mobi2_ipcp;
	} else if (hand_state.dif == "rinaisense"){
		next_dif = "ocarina";
		next_mob_dif = "mobile2.DIF";
		neighbor.processName = "ap5.mobile2";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = false;
		mob_ipcp_enroll = mobi2_ipcp;
		mob_ipcp_disc = mobi2_ipcp;
	} else {
		next_dif = "irati";
		next_mob_dif = "mobile.DIF";
		neighbor.processName = "ap6.mobile2";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = true;
		mob_ipcp_enroll = mobi1_ipcp;
		mob_ipcp_disc = mobi2_ipcp;
		int_neighbor.processName = "core2.internet";
		int_neighbor.processInstance = "1";
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
	if(IPCManager->enroll_to_dif(this, &promise, ipcp_enroll->get_id(), neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
			  ipcp_enroll->get_id(),
			 neigh_data.difName.processName.c_str(),
			 bs_info.ipcp_address.c_str());
		return;
	}

	//Enroll to mobile DIF
	mob_neigh_data.supportingDifName.processName = next_dif;
	mob_neigh_data.difName.processName = next_mob_dif;

	if(IPCManager->enroll_to_dif(this, &promise, mob_ipcp_enroll->get_id(), mob_neigh_data, true, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
				mob_ipcp_enroll->get_id(),
				mob_neigh_data.difName.processName.c_str(),
				mob_neigh_data.supportingDifName.processName.c_str());
		return;
	}

	if (hand_state.change_mob_dif) {
		sleep.sleepForMili(1000);
		int_neigh_data.supportingDifName.processName = next_mob_dif;
		int_neigh_data.difName.processName = "internet.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, inet_ipcp->get_id(), int_neigh_data,
				true, int_neighbor) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					inet_ipcp->get_id(),
					int_neigh_data.difName.processName.c_str(),
					int_neigh_data.supportingDifName.processName.c_str());
			return;
		}
	}

	//4 Now it is multihomed, wait 5 seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(hand_state.disc_wait_time_ms);

	hand_state.dif = next_dif;
	hand_state.ipcp = ipcp_enroll;

	if (hand_state.change_mob_dif) {
		if(IPCManager->disconnect_neighbor(this, &promise, inet_ipcp->get_id(), int_neighbor) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
					inet_ipcp->get_id());
			return;
		}
	}

	if(IPCManager->disconnect_neighbor(this, &promise, mob_ipcp_disc->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				mob_ipcp_disc->get_id());
		return;
	}

	difs_it = report.available_difs.find(disc_dif);
	if (difs_it == report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", disc_dif.c_str());
	} else {
		bs_info = difs_it->second.available_bs_ipcps.front();
	}

	neighbor.processName = bs_info.ipcp_address;
	neighbor.processInstance = "";

	if(IPCManager->disconnect_neighbor(this, &promise, ipcp_disc->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				ipcp_disc->get_id());
		return;
	}

	LOG_DBG("Handover done!");
}

//Roaming UE, single provider with DC scenario (MEC), with dynamic DIF allocator
void MobilityManager::execute_handover4(const rina::MediaReport& report)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data, int_neigh_data, slice_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * mobi1_ipcp;
	IPCMIPCProcess * ipcp_enroll, * ipcp_disc;
	Promise promise;
	std::string next_dif, disc_dif;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor, int_neighbor;

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

	mobi1_ipcp = factory->getIPCProcess(3);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = report.available_difs.find("pristine");
		if (difs_it == report.available_difs.end()) {
			LOG_WARN("No members of DIF 'pristine' are within range");
			return;
		}

		bs_info = difs_it->second.available_bs_ipcps.front();

		//Enroll the shim to the new DIF/AP
		neigh_data.apName.processName = bs_info.ipcp_address;
		neigh_data.difName.processName = "pristine";
		if(IPCManager->enroll_to_dif(this, &promise, wifi1_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
				 wifi1_ipcp->get_id(),
				 neigh_data.difName.processName.c_str(),
				 bs_info.ipcp_address.c_str());
			return;
		}

		//Enroll to the mobile DIF
		mob_neigh_data.supportingDifName.processName = "pristine";
		mob_neigh_data.difName.processName = "mobile.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, mobi1_ipcp->get_id(), mob_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					mobi1_ipcp->get_id(),
					mob_neigh_data.difName.processName.c_str(),
					mob_neigh_data.supportingDifName.processName.c_str());
			return;
		}

		hand_state.dif = "pristine";
		hand_state.ipcp = wifi1_ipcp;

		LOG_DBG("Initialized");

		return;
	}

	//3 Update wifi IPCPs to use
	if (hand_state.ipcp == wifi1_ipcp) {
		ipcp_enroll = wifi2_ipcp;
		ipcp_disc = wifi1_ipcp;
	} else {
		ipcp_enroll = wifi1_ipcp;
		ipcp_disc = wifi2_ipcp;
	}

	//4 Update DIF name (irati -> pristine -> arcfire -> irina
	// -> rinaisense -> ocarina and start with irati again)
	disc_dif = hand_state.dif;
	if (hand_state.dif == "irati") {
		next_dif = "pristine";
		neighbor.processName = "ar1.mobile";
		neighbor.processInstance = "1";
	} else if (hand_state.dif == "pristine") {
		next_dif = "arcfire";
		neighbor.processName = "ar2.mobile";
		neighbor.processInstance = "1";
	} else if (hand_state.dif == "arcfire"){
		next_dif = "rinaisense";
		neighbor.processName = "ar3.mobile";
		neighbor.processInstance = "1";
	} else if (hand_state.dif == "irina"){
		next_dif = "rinaisense";
		neighbor.processName = "ar4.mobile";
		neighbor.processInstance = "1";
	} else if (hand_state.dif == "rinaisense"){
		next_dif = "ocarina";
		neighbor.processName = "ar5.mobile";
		neighbor.processInstance = "1";
	} else {
		next_dif = "pristine";
		neighbor.processName = "ar6.mobile";
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
	if(IPCManager->enroll_to_dif(this, &promise, ipcp_enroll->get_id(), neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
			  ipcp_enroll->get_id(),
			 neigh_data.difName.processName.c_str(),
			 bs_info.ipcp_address.c_str());
		return;
	}

	//Enroll to mobile DIF
	mob_neigh_data.supportingDifName.processName = next_dif;
	mob_neigh_data.difName.processName = "mobile.DIF";

	if(IPCManager->enroll_to_dif(this, &promise, mobi1_ipcp->get_id(), mob_neigh_data, true, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
				mobi1_ipcp->get_id(),
				mob_neigh_data.difName.processName.c_str(),
				mob_neigh_data.supportingDifName.processName.c_str());
		return;
	}

	//4 Now it is multihomed, wait 5 seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(hand_state.disc_wait_time_ms);

	hand_state.dif = next_dif;
	hand_state.ipcp = ipcp_enroll;

	if(IPCManager->disconnect_neighbor(this, &promise, mobi1_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				mobi1_ipcp->get_id());
		return;
	}

	difs_it = report.available_difs.find(disc_dif);
	if (difs_it == report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", disc_dif.c_str());
	} else {
		bs_info = difs_it->second.available_bs_ipcps.front();
	}

	neighbor.processName = bs_info.ipcp_address;
	neighbor.processInstance = "";

	if(IPCManager->disconnect_neighbor(this, &promise, ipcp_disc->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				ipcp_disc->get_id());
		return;
	}

	LOG_DBG("Handover done!");
}

//Static UE
void MobilityManager::execute_handover5(const rina::MediaReport& report)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * mobi1_ipcp;
	Promise promise;
	rina::ApplicationProcessNamingInformation neighbor, int_neighbor;

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

	mobi1_ipcp = factory->getIPCProcess(2);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = report.available_difs.find("pristine");
		if (difs_it == report.available_difs.end()) {
			LOG_WARN("No members of DIF 'pristine' are within range");
			return;
		}

		bs_info = difs_it->second.available_bs_ipcps.front();

		//Enroll the shim to the new DIF/AP
		neigh_data.apName.processName = bs_info.ipcp_address;
		neigh_data.difName.processName = "pristine";
		if(IPCManager->enroll_to_dif(this, &promise, wifi1_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
				 wifi1_ipcp->get_id(),
				 neigh_data.difName.processName.c_str(),
				 bs_info.ipcp_address.c_str());
			return;
		}

		//Enroll to the mobile DIF
		mob_neigh_data.supportingDifName.processName = "pristine";
		mob_neigh_data.difName.processName = "mobile.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, mobi1_ipcp->get_id(), mob_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					mobi1_ipcp->get_id(),
					mob_neigh_data.difName.processName.c_str(),
					mob_neigh_data.supportingDifName.processName.c_str());
			return;
		}

		hand_state.dif = "pristine";
		hand_state.ipcp = wifi1_ipcp;

		LOG_DBG("Initialized");

		return;
	}
}

void HandoverTimerTask::run()
{
	mobman->execute_handover(report);
}

} //namespace rinad
