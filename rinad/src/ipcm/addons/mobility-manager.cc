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
const std::string MobilityManager::ARCFIRE_EXP5_OMEC_ROAMING = "arcfire-exp5-omec-roaming";
const std::string MobilityManager::ARCFIRE_EXP5_2OPERATOR_DMM = "arcfire-exp5-2operator-dmm";
const std::string MobilityManager::ARCFIRE_EXP5_MAC = "arcfire-exp5-mac";
const std::string MobilityManager::ARCFIRE_EXP5_TIP_UE1 = "arcfire-exp5-tip-ue1";
const std::string MobilityManager::ARCFIRE_EXP5_TIP_UE2 = "arcfire-exp5-tip-ue2";

const int MobilityManager::DEFAULT_DISC_WAIT_TIME_MS = 5000;
const int MobilityManager::DEFAULT_HANDOVER_PERIOD_MS = 60000;
const int MobilityManager::DEFAULT_BOOTSTRAP_WAIT_TIME_MS = 20000;

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
        	Json::Value general_conf = mobman_conf["generalConf"];
        	if (general_conf != 0) {
        		hand_state.hand_type = general_conf.get("handType",
        				hand_state.hand_type).asString();
        		hand_state.disc_wait_time_ms = general_conf.get("discWaitTime",
        				hand_state.disc_wait_time_ms).asInt();
        		hand_state.hand_period_ms = general_conf.get("handPeriodMs",
        				hand_state.hand_period_ms).asInt();
        	}

                LOG_DBG("Mobility Manager configuration parsed");
        }
}

MobilityManager::MobilityManager(const rinad::RINAConfiguration& config) :
		AppAddon(MobilityManager::NAME)
{
	hand_state.hand_period_ms = DEFAULT_HANDOVER_PERIOD_MS;
	hand_state.disc_wait_time_ms = DEFAULT_DISC_WAIT_TIME_MS;
	hand_state.hand_type = ARCFIRE_EXP5_OMEC_ROAMING;
	parse_configuration(config);
	factory = IPCManager->get_ipcp_factory();
	hand_state.first_report = true;
	hand_state.dif = "";
	hand_state.ipcp = 0;

	LOG_INFO("Mobility Manager instance created, scheduling bootstrap task in 20s");
        LOG_INFO("Handover type: %s", hand_state.hand_type.c_str());
        LOG_INFO("Handover period (ms): %d", hand_state.hand_period_ms);
        LOG_INFO("Disconnect time (ms): %d", hand_state.disc_wait_time_ms);

	BoostrapTimerTask * task = new BoostrapTimerTask(this);
	timer.scheduleTask(task, DEFAULT_BOOTSTRAP_WAIT_TIME_MS);
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

	last_media_report = event->media_report;

	if (hand_state.first_report) {
		hand_state.first_report = false;

	        InitializeTimerTask * task = new InitializeTimerTask(this);
	        timer.scheduleTask(task, 0);
	}
}

void MobilityManager::boostrap()
{
	int result;

	rina::ScopedLock g(lock);
	LOG_DBG("Boostrapping Mobility Manager");

	result = boostrap_exp5();
	if (result != 0) {
	        BoostrapTimerTask * task = new BoostrapTimerTask(this);
	        timer.scheduleTask(task, DEFAULT_BOOTSTRAP_WAIT_TIME_MS);
	}
}

int MobilityManager::boostrap_exp5()
{
	IPCMIPCProcess * wifi_ipcp;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get shim IPCP on WiFi interface
	wifi_ipcp = factory->getIPCProcess(1);
	if (wifi_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return -1;
	}

	if (wifi_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi_ipcp->get_type().c_str());
		return -1;
	}

	// 2. Trigger first scan
	wifi_ipcp->proxy_->scan_media();

	return 0;
}

void MobilityManager::initialize()
{
	int result;

	rina::ScopedLock g(lock);
	LOG_DBG("Initializing Mobility Manager");

	if (hand_state.hand_type == ARCFIRE_EXP5_OMEC_ROAMING) {
		result = initialize_arcfire_exp5_omec();
	} else if (hand_state.hand_type == ARCFIRE_EXP5_2OPERATOR_DMM) {
		result = initialize_arcfire_exp5_2operator_dmm();
	} else if (hand_state.hand_type == ARCFIRE_EXP5_MAC) {
		result = initialize_arcfire_exp5_mac();
	} else if (hand_state.hand_type == ARCFIRE_EXP5_TIP_UE1) {
		result = initialize_arcfire_exp5_tip(true);
	} else if (hand_state.hand_type == ARCFIRE_EXP5_TIP_UE2) {
		result = initialize_arcfire_exp5_tip(false);
	}

	if (result != 0) {
	        InitializeTimerTask * task = new InitializeTimerTask(this);
	        timer.scheduleTask(task, 1000);
	}
}

int MobilityManager::initialize_arcfire_exp5_omec()
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * mobi1_ipcp;
	Promise promise;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get all IPCPS
	wifi1_ipcp = factory->getIPCProcess(1);
	if (wifi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return -1;
	}

	mobi1_ipcp = factory->getIPCProcess(3);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return -1;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return -1;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = last_media_report.available_difs.find("arcfire");
		if (difs_it == last_media_report.available_difs.end()) {
			LOG_WARN("No members of DIF 'pristine' are within range");
			return -1;
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
			return -1;
		}

		//Enroll to the mobile DIF
		mob_neigh_data.supportingDifName.processName = "arcfire";
		mob_neigh_data.difName.processName = "mobile.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, mobi1_ipcp->get_id(), mob_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					mobi1_ipcp->get_id(),
					mob_neigh_data.difName.processName.c_str(),
					mob_neigh_data.supportingDifName.processName.c_str());
			return -1;
		}

		hand_state.dif = "arcfire";
		hand_state.ipcp = wifi1_ipcp;

		LOG_DBG("Initialized");
	}

	if (hand_state.hand_period_ms != 0) {
		HandoverTimerTask * task = new HandoverTimerTask(this);
		timer.scheduleTask(task, hand_state.hand_period_ms);
	}

	return 0;
}

int MobilityManager::initialize_arcfire_exp5_2operator_dmm()
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data, int_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * mobi1_ipcp;
	IPCMIPCProcess * mobi2_ipcp, * inet_ipcp, *ipcp_enroll;
	IPCMIPCProcess * ipcp_disc, * mob_ipcp_enroll, * mob_ipcp_disc;
	Promise promise;
	rina::Sleep sleep;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get all IPCPS
	wifi1_ipcp = factory->getIPCProcess(1);
	if (wifi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return -1;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return -1;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		return -1;
	}

	mobi1_ipcp = factory->getIPCProcess(3);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return -1;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return -1;
	}

	mobi2_ipcp = factory->getIPCProcess(4);
	if (mobi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 4");
		return -1;
	}

	if (mobi2_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi2_ipcp->get_type().c_str());
		return -1;
	}

	inet_ipcp = factory->getIPCProcess(5);
	if (inet_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 5");
		return -1;
	}

	if (inet_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", inet_ipcp->get_type().c_str());
		return -1;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = last_media_report.available_difs.find("pristine");
		if (difs_it == last_media_report.available_difs.end()) {
			LOG_WARN("No members of DIF 'irati' are within range");
			return -1;
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
			return -1;
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
			return -1;
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
			return -1;
		}

		hand_state.dif = "pristine";
		hand_state.ipcp = wifi1_ipcp;

		LOG_DBG("Initialized");
	}

	if (hand_state.hand_period_ms != 0) {
		HandoverTimerTask * task = new HandoverTimerTask(this);
		timer.scheduleTask(task, hand_state.hand_period_ms);
	}

	return 0;
}

int MobilityManager::initialize_arcfire_exp5_tip(bool ue1)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data, int_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * prov_ipcp;
	IPCMIPCProcess *slice_ipcp, *ipcp_enroll;
	IPCMIPCProcess * ipcp_disc, * mob_ipcp_enroll, * mob_ipcp_disc;
	std::string ssid, prov_dif_name, slice_dif_name;
	Promise promise;
	rina::Sleep sleep;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get all IPCPS
	wifi1_ipcp = factory->getIPCProcess(1);
	if (wifi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return -1;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return -1;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		return -1;
	}

	if (ue1)
		prov_ipcp = factory->getIPCProcess(3);
	else
		prov_ipcp = factory->getIPCProcess(4);
	if (prov_ipcp == NULL) {
		LOG_ERR("Could not find IPCP for the provider DIF");
		return -1;
	}

	if (prov_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", prov_ipcp->get_type().c_str());
		return -1;
	}

	slice_ipcp = factory->getIPCProcess(5);
	if (slice_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 5");
		return -1;
	}

	if (slice_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", slice_ipcp->get_type().c_str());
		return -1;
	}

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		if (ue1) {
			ssid = "irati";
			prov_dif_name = "accprov1";
			slice_dif_name = "slice1";
		} else {
			ssid = "arcfire";
			prov_dif_name = "accprov2";
			slice_dif_name = "slice2";
		}

		//Not enrolled anywhere yet, enroll for first time
		difs_it = last_media_report.available_difs.find(ssid);
		if (difs_it == last_media_report.available_difs.end()) {
			LOG_WARN("No members of DIF '%s' are within range", ssid.c_str());
			return -1;
		}

		bs_info = difs_it->second.available_bs_ipcps.front();

		//Enroll the shim to the new DIF/AP
		neigh_data.apName.processName = bs_info.ipcp_address;
		neigh_data.difName.processName = ssid;
		if(IPCManager->enroll_to_dif(this, &promise, wifi1_ipcp->get_id(), neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via neighbor %s",
				 wifi1_ipcp->get_id(),
				 neigh_data.difName.processName.c_str(),
				 bs_info.ipcp_address.c_str());
			return -1;
		}

		//Enroll to the provider DIF
		mob_neigh_data.supportingDifName.processName = ssid;
		mob_neigh_data.difName.processName = prov_dif_name;
		if(IPCManager->enroll_to_dif(this, &promise, prov_ipcp->get_id(), mob_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					prov_ipcp->get_id(),
					mob_neigh_data.difName.processName.c_str(),
					mob_neigh_data.supportingDifName.processName.c_str());
			return -1;
		}

		sleep.sleepForMili(1000);

		//Enroll to the slice DIF
		int_neigh_data.supportingDifName.processName = prov_dif_name;
		int_neigh_data.difName.processName = slice_dif_name;
		if(IPCManager->enroll_to_dif(this, &promise, slice_ipcp->get_id(), int_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					slice_ipcp->get_id(),
					int_neigh_data.difName.processName.c_str(),
					int_neigh_data.supportingDifName.processName.c_str());
			return -1;
		}

		hand_state.dif = ssid;
		hand_state.ipcp = wifi1_ipcp;

		LOG_DBG("Initialized");
	}

	if (hand_state.hand_period_ms != 0) {
		HandoverTimerTask * task = new HandoverTimerTask(this);
		timer.scheduleTask(task, hand_state.hand_period_ms);
	}

	return 0;
}

int MobilityManager::initialize_arcfire_exp5_mac()
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data, int_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * mobi1_ipcp;
	IPCMIPCProcess * ipcp_enroll;
	IPCMIPCProcess * ipcp_disc, * mob_ipcp_enroll, * mob_ipcp_disc;
	Promise promise;
	CreateIPCPPromise c_promise;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation ipcp_name, dif_name, sdif_name;
	rinad::DIFTemplateMapping template_mapping;
	rinad::DIFTemplate dif_template;

	// Prevent any insertion or deletion of IPC Processes to happen
	factory->rwlock.readlock();

	// 1. Get all IPCPS
	wifi1_ipcp = factory->getIPCProcess(1);
	if (wifi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		factory->rwlock.unlock();
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		factory->rwlock.unlock();
		return -1;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		factory->rwlock.unlock();
		return -1;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		factory->rwlock.unlock();
		return -1;
	}

	mobi1_ipcp = factory->getIPCProcess(4);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 4");
		factory->rwlock.unlock();
		return -1;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		factory->rwlock.unlock();
		return -1;
	}

	factory->rwlock.unlock();

	// 2. See if it has been initialized before, otherwise do it
	if (hand_state.dif == "") {
		//Not enrolled anywhere yet, enroll for first time
		difs_it = last_media_report.available_difs.find("irati");
		if (difs_it == last_media_report.available_difs.end()) {
			LOG_WARN("No members of DIF 'irati' are within range");
			return -1;
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
			return -1;
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
			return -1;
		}

		sleep.sleepForMili(500);

		//Create IPCP for the Internet DIF
		ipcp_name.processName = "ue.internet";
		ipcp_name.processInstance = "1";
                if (IPCManager->create_ipcp(this, &c_promise, ipcp_name, rina::NORMAL_IPC_PROCESS, "internet.DIF") == IPCM_FAILURE
                		|| c_promise.wait() != IPCM_SUCCESS) {
                	LOG_WARN("Problems creating IPCP %s of type %s",
                			ipcp_name.toString().c_str(),
					rina::NORMAL_IPC_PROCESS.c_str());
                	return -1;
                }

                //Assign to DIF
                if (IPCManager->dif_template_manager->get_dif_template("internet-mac.dif", dif_template) != 0) {
                	LOG_WARN("Could not find DIF template called internet-mac.dif");
                	return -1;
                }

                dif_name.processName = "internet.DIF";
                if (IPCManager->assign_to_dif(this, &promise, c_promise.ipcp_id,
                        			dif_template, dif_name) == IPCM_FAILURE
                        			|| promise.wait() != IPCM_SUCCESS) {
                	LOG_WARN("Problems assigning IPCP to DIF");
                	return -1;
                }

                //Register at N-1 DIF
                sdif_name.processName = "mobile.DIF";
                if (IPCManager->register_at_dif(this, &promise, c_promise.ipcp_id, sdif_name)
                        			== IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
                	LOG_WARN("Problems registering to N-1 DIF");
                	return -1;
                }

		//Enroll to the internet DIF
		int_neigh_data.supportingDifName.processName = "mobile.DIF";
		int_neigh_data.difName.processName = "internet.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, c_promise.ipcp_id, int_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					c_promise.ipcp_id,
					int_neigh_data.difName.processName.c_str(),
					int_neigh_data.supportingDifName.processName.c_str());
			return -1;
		}

		//Create IPCP for the Slice1 DIF
		ipcp_name.processName = "ue.slice1";
		ipcp_name.processInstance = "1";
                if (IPCManager->create_ipcp(this, &c_promise, ipcp_name, rina::NORMAL_IPC_PROCESS, "slice1.DIF") == IPCM_FAILURE
                		|| c_promise.wait() != IPCM_SUCCESS) {
                	LOG_WARN("Problems creating IPCP %s of type %s",
                			ipcp_name.toString().c_str(),
					rina::NORMAL_IPC_PROCESS.c_str());
                	return -1;
                }

                //Assign to DIF
                if (IPCManager->dif_template_manager->get_dif_template("slice1-mac.dif", dif_template) != 0) {
                	LOG_WARN("Could not find DIF template called slice1-mac.dif");
                	return -1;
                }

                dif_name.processName = "slice1.DIF";
                if (IPCManager->assign_to_dif(this, &promise, c_promise.ipcp_id,
                        			dif_template, dif_name) == IPCM_FAILURE
                        			|| promise.wait() != IPCM_SUCCESS) {
                	LOG_WARN("Problems assigning IPCP to DIF");
                	return -1;
                }

                //Register at N-1 DIF
                sdif_name.processName = "mobile.DIF";
                if (IPCManager->register_at_dif(this, &promise, c_promise.ipcp_id, sdif_name)
                        			== IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
                	LOG_WARN("Problems registering to N-1 DIF");
                	return -1;
                }

		//Enroll to the slice1 DIF
		int_neigh_data.supportingDifName.processName = "mobile.DIF";
		int_neigh_data.difName.processName = "slice1.DIF";
		if(IPCManager->enroll_to_dif(this, &promise, c_promise.ipcp_id, int_neigh_data) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					c_promise.ipcp_id,
					int_neigh_data.difName.processName.c_str(),
					int_neigh_data.supportingDifName.processName.c_str());
			return -1;
		}

		hand_state.dif = "irati_down";
		hand_state.ipcp = wifi1_ipcp;

		LOG_DBG("Initialized");
	}

	HandoverTimerTask * task = new HandoverTimerTask(this);
	timer.scheduleTask(task, 20000);

	return 0;
}

void MobilityManager::execute_handover()
{
	int result;

	rina::ScopedLock g(lock);
	LOG_DBG("Executing handover");

	if (hand_state.hand_type == ARCFIRE_EXP5_OMEC_ROAMING) {
		result = execute_handover_arcfire_exp5_omec();
	} else if (hand_state.hand_type == ARCFIRE_EXP5_2OPERATOR_DMM) {
		result = excecute_handover_arcfire_exp5_2operator_dmm();
	} else if (hand_state.hand_type == ARCFIRE_EXP5_MAC) {
		result = execute_handover_arcfire_exp5_mac();
	} else if (hand_state.hand_type == ARCFIRE_EXP5_TIP_UE1) {
		result = execute_handover_arcfire_exp5_tip(true);
	} else if (hand_state.hand_type == ARCFIRE_EXP5_TIP_UE2) {
		result = execute_handover_arcfire_exp5_tip(false);
	}

	if (result != 0) {
	        HandoverTimerTask * task = new HandoverTimerTask(this);
	        timer.scheduleTask(task, 5000);
	}
}

int MobilityManager::execute_handover_arcfire_exp5_tip(bool ue1)
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data, slice_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * prov1_ipcp;
	IPCMIPCProcess * prov2_ipcp, * slice_ipcp, *ipcp_enroll;
	IPCMIPCProcess * ipcp_disc, * prov_ipcp_enroll, * prov_ipcp_disc;
	Promise promise;
	std::string next_dif, disc_dif, next_prov_dif, dic_prov_dif;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor, slice_neighbor;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get all IPCPS
	wifi1_ipcp = factory->getIPCProcess(1);
	if (wifi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return -1;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return -1;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		return -1;
	}

	prov1_ipcp = factory->getIPCProcess(3);
	if (prov1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return -1;
	}

	if (prov1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", prov1_ipcp->get_type().c_str());
		return -1;
	}

	prov2_ipcp = factory->getIPCProcess(4);
	if (prov2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 4");
		return -1;
	}

	if (prov2_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", prov2_ipcp->get_type().c_str());
		return -1;
	}

	slice_ipcp = factory->getIPCProcess(5);
	if (slice_ipcp == NULL) {
			LOG_ERR("Could not find IPCP from slice1.DIF");
			return -1;
	}

	if (slice_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", slice_ipcp->get_type().c_str());
		return -1;
	}

	//3 Update wifi IPCPs to use
	if (hand_state.ipcp == wifi1_ipcp) {
		ipcp_enroll = wifi2_ipcp;
		ipcp_disc = wifi1_ipcp;
	} else {
		ipcp_enroll = wifi1_ipcp;
		ipcp_disc = wifi2_ipcp;
	}

	//4 Update DIF name (irati -> pristine -> arcfire -> irina and start again for UE1)
	// (rinaisense -> ocarina and start again for UE2)
	disc_dif = hand_state.dif;
	if (hand_state.dif == "irati") {
		next_dif = "pristine";
		next_prov_dif = "accprov1";
		neighbor.processName = "ar1.accprov1";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = false;
		prov_ipcp_enroll = prov1_ipcp;
		prov_ipcp_disc = prov1_ipcp;
	} else if (hand_state.dif == "pristine") {
		next_dif = "arcfire";
		next_prov_dif = "accprov2";
		neighbor.processName = "ar2.accprov1";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = true;
		prov_ipcp_enroll = prov2_ipcp;
		prov_ipcp_disc = prov1_ipcp;
		if (ue1)
			slice_neighbor.processName = "isp1.slice1";
		else
			slice_neighbor.processName = "isp1.slice2";
		slice_neighbor.processInstance = "1";
	} else if (hand_state.dif == "arcfire"){
		next_dif = "irina";
		next_prov_dif = "accprov2";
		neighbor.processName = "ar3.accprov2";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = false;
		prov_ipcp_enroll = prov2_ipcp;
		prov_ipcp_disc = prov2_ipcp;
	} else if (hand_state.dif == "irina"){
		next_dif = "irati";
		next_prov_dif = "accprov1";
		neighbor.processName = "ar4.accprov2";
		neighbor.processInstance = "1";
		hand_state.change_mob_dif = true;
		prov_ipcp_enroll = prov1_ipcp;
		prov_ipcp_disc = prov2_ipcp;
		if (ue1)
			slice_neighbor.processName = "isp2.slice1";
		else
			slice_neighbor.processName = "isp2.slice2";
		slice_neighbor.processInstance = "1";
	}

	// 3. Enroll to next DIF (become multihomed)
	difs_it = last_media_report.available_difs.find(next_dif);
	if (difs_it == last_media_report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", next_dif.c_str());
		return -1;
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
		return -1;
	}

	//Enroll to provider DIF
	mob_neigh_data.supportingDifName.processName = next_dif;
	mob_neigh_data.difName.processName = next_prov_dif;

	if(IPCManager->enroll_to_dif(this, &promise, prov_ipcp_enroll->get_id(), mob_neigh_data, true, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
				prov_ipcp_enroll->get_id(),
				mob_neigh_data.difName.processName.c_str(),
				mob_neigh_data.supportingDifName.processName.c_str());
		return -1;
	}

	if (hand_state.change_mob_dif) {
		sleep.sleepForMili(1000);
		slice_neigh_data.supportingDifName.processName = next_prov_dif;
		if (ue1)
			slice_neigh_data.difName.processName = "slice1";
		else
			slice_neigh_data.difName.processName = "slice2";
		if(IPCManager->enroll_to_dif(this, &promise, slice_ipcp->get_id(), slice_neigh_data,
				true, slice_neighbor) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
					slice_ipcp->get_id(),
					slice_neigh_data.difName.processName.c_str(),
					slice_neigh_data.supportingDifName.processName.c_str());
			return -1;
		}
	}

	//4 Now it is multihomed, wait 5 seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(hand_state.disc_wait_time_ms);

	hand_state.dif = next_dif;
	hand_state.ipcp = ipcp_enroll;

	if (hand_state.change_mob_dif) {
		if(IPCManager->disconnect_neighbor(this, &promise, slice_ipcp->get_id(), slice_neighbor) == IPCM_FAILURE ||
				promise.wait() != IPCM_SUCCESS) {
			LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
					slice_ipcp->get_id());
			return -1;
		}
	}

	if(IPCManager->disconnect_neighbor(this, &promise, prov_ipcp_disc->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				prov_ipcp_disc->get_id());
		return -1;
	}

	difs_it = last_media_report.available_difs.find(disc_dif);
	if (difs_it == last_media_report.available_difs.end()) {
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
		return -1;
	}

	LOG_DBG("Handover done!");

	//Re-schedule handover task
	if (hand_state.hand_period_ms != 0) {
		HandoverTimerTask * task = new HandoverTimerTask(this);
		timer.scheduleTask(task, hand_state.hand_period_ms);
	}

	return 0;
}

//Roaming UE, two provider scenario with static DIF Allocator
int MobilityManager::excecute_handover_arcfire_exp5_2operator_dmm()
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
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return -1;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return -1;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		return -1;
	}

	mobi1_ipcp = factory->getIPCProcess(3);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return -1;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return -1;
	}

	mobi2_ipcp = factory->getIPCProcess(4);
	if (mobi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 4");
		return -1;
	}

	if (mobi2_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi2_ipcp->get_type().c_str());
		return -1;
	}

	inet_ipcp = factory->getIPCProcess(5);
	if (inet_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 5");
		return -1;
	}

	if (inet_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", inet_ipcp->get_type().c_str());
		return -1;
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
		next_dif = "rinaisense";
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
		next_dif = "pristine";
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
	difs_it = last_media_report.available_difs.find(next_dif);
	if (difs_it == last_media_report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", next_dif.c_str());
		return -1;
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
		return -1;
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
		return -1;
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
			return -1;
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
			return -1;
		}
	}

	if(IPCManager->disconnect_neighbor(this, &promise, mob_ipcp_disc->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				mob_ipcp_disc->get_id());
		return -1;
	}

	difs_it = last_media_report.available_difs.find(disc_dif);
	if (difs_it == last_media_report.available_difs.end()) {
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
		return -1;
	}

	LOG_DBG("Handover done!");

	//Re-schedule handover task
	if (hand_state.hand_period_ms != 0) {
		HandoverTimerTask * task = new HandoverTimerTask(this);
		timer.scheduleTask(task, hand_state.hand_period_ms);
	}

	return 0;
}

//Roaming UE, single provider with DC scenario (MEC), with dynamic DIF allocator
int MobilityManager::execute_handover_arcfire_exp5_omec()
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
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return -1;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return -1;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		return -1;
	}

	mobi1_ipcp = factory->getIPCProcess(3);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 3");
		return -1;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return -1;
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
	difs_it = last_media_report.available_difs.find(next_dif);
	if (difs_it == last_media_report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", next_dif.c_str());
		return -1;
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
		return -1;
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
		return -1;
	}

	//4 Now it is multihomed, wait 5 seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(hand_state.disc_wait_time_ms);

	hand_state.dif = next_dif;
	hand_state.ipcp = ipcp_enroll;

	if(IPCManager->disconnect_neighbor(this, &promise, mobi1_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				mobi1_ipcp->get_id());
		return -1;
	}

	difs_it = last_media_report.available_difs.find(disc_dif);
	if (difs_it == last_media_report.available_difs.end()) {
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
		return -1;
	}

	LOG_DBG("Handover done!");

	//Re-schedule handover task
	if (hand_state.hand_period_ms != 0) {
		HandoverTimerTask * task = new HandoverTimerTask(this);
		timer.scheduleTask(task, hand_state.hand_period_ms);
	}

	return 0;
}

int MobilityManager::execute_handover_arcfire_exp5_mac()
{
	if (hand_state.dif == "irati_down") {
		return execute_handover_arcfire_exp5_mac_wifi_hand();
	}

	if (hand_state.dif == "pristine_down") {
		return execute_handover_arcfire_exp5_mac_wifi_hand();
	}

	if (hand_state.dif == "arcfire_down") {
		return execute_handover_arcfire_exp5_mac_wifi_fixed();
	}

	if (hand_state.dif == "arcfire_up") {
		return execute_handover_arcfire_exp5_mac_fixed_wifi();
	}

	if (hand_state.dif == "pristine_up") {
		return execute_handover_arcfire_exp5_mac_wifi_hand();
	}

	if (hand_state.dif == "irati_up") {
		return execute_handover_arcfire_exp5_mac_wifi_hand();
	}

	LOG_WARN("Unknown hand_state.dif %s", hand_state.dif.c_str());
	return -1;
}

int MobilityManager::execute_handover_arcfire_exp5_mac_wifi_hand()
{
	std::map<std::string, rina::MediaDIFInfo>::const_iterator difs_it;
	rina::BaseStationInfo bs_info;
	NeighborData neigh_data, mob_neigh_data, int_neigh_data, slice_neigh_data;
	IPCMIPCProcess * wifi1_ipcp, * wifi2_ipcp, * mobi1_ipcp;
	IPCMIPCProcess * ipcp_enroll, * ipcp_disc;
	Promise promise;
	std::string next_dif, disc_dif, next_state;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor, int_neighbor;

	// Prevent any insertion or deletion of IPC Processes to happen
	rina::ReadScopedLock readlock(factory->rwlock);

	// 1. Get all IPCPS
	wifi1_ipcp = factory->getIPCProcess(1);
	if (wifi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 1");
		return -1;
	}

	if (wifi1_ipcp->get_type () != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi1_ipcp->get_type().c_str());
		return -1;
	}

	wifi2_ipcp = factory->getIPCProcess(2);
	if (wifi2_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 2");
		return -1;
	}

	if (wifi2_ipcp->get_type() != rina::SHIM_WIFI_IPC_PROCESS_STA) {
		LOG_ERR("Wrong IPCP type: %s", wifi2_ipcp->get_type().c_str());
		return -1;
	}

	mobi1_ipcp = factory->getIPCProcess(4);
	if (mobi1_ipcp == NULL) {
		LOG_ERR("Could not find IPCP with ID 4");
		return -1;
	}

	if (mobi1_ipcp->get_type() != rina::NORMAL_IPC_PROCESS) {
		LOG_ERR("Wrong IPCP type: %s", mobi1_ipcp->get_type().c_str());
		return -1;
	}

	//3 Update wifi IPCPs to use
	if (hand_state.ipcp == wifi1_ipcp) {
		ipcp_enroll = wifi2_ipcp;
		ipcp_disc = wifi1_ipcp;
	} else {
		ipcp_enroll = wifi1_ipcp;
		ipcp_disc = wifi2_ipcp;
	}

	//4 Update DIF name (irati -> pristine -> arcfire ->
	// -> pristine -> irati and go to pristine again)
	disc_dif = hand_state.dif;
	if (hand_state.dif == "irati_down") {
		disc_dif = "irati";
		next_dif = "pristine";
		neighbor.processName = "ar1.mobile";
		neighbor.processInstance = "1";
		next_state = "pristine_down";
	} else if (hand_state.dif == "pristine_down") {
		disc_dif = "pristine";
		next_dif = "arcfire";
		neighbor.processName = "ar2.mobile";
		neighbor.processInstance = "1";
		next_state = "arcfire_down";
	} else if (hand_state.dif == "pristine_up"){
		disc_dif = "arcfire";
		next_dif = "pristine";
		neighbor.processName = "ar3.mobile";
		neighbor.processInstance = "1";
		next_state = "irati_up";
	} else if (hand_state.dif == "irati_up"){
		disc_dif = "pristine";
		next_dif = "irati";
		neighbor.processName = "ar2.mobile";
		neighbor.processInstance = "1";
		next_state = "irati_down";
	}

	// 3. Enroll to next DIF (become multihomed)
	difs_it = last_media_report.available_difs.find(next_dif);
	if (difs_it == last_media_report.available_difs.end()) {
		LOG_WARN("No members of DIF '%s' are within range", next_dif.c_str());
		return -1;
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
		return -1;
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
		return -1;
	}

	//4 Now it is multihomed, wait N seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(hand_state.disc_wait_time_ms);

	hand_state.dif = next_state;
	hand_state.ipcp = ipcp_enroll;

	if(IPCManager->disconnect_neighbor(this, &promise, mobi1_ipcp->get_id(), neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP %u",
				mobi1_ipcp->get_id());
		return -1;
	}

	difs_it = last_media_report.available_difs.find(disc_dif);
	if (difs_it == last_media_report.available_difs.end()) {
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
		return -1;
	}

	LOG_DBG("Handover done!");

	//Re-schedule handover task
	HandoverTimerTask * task = new HandoverTimerTask(this);
	timer.scheduleTask(task, 20000);

	return 0;
}

int MobilityManager::execute_handover_arcfire_exp5_mac_wifi_fixed()
{
	Promise promise;
	CreateIPCPPromise c_promise;
	rina::ApplicationProcessNamingInformation ipcp_name, dif_name, sdif_name;
	rinad::DIFTemplateMapping template_mapping;
	rinad::DIFTemplate dif_template;
	NeighborData fixed_neigh_data, int_neigh_data;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor;

	//Create IPCP for the fixed DIF
	ipcp_name.processName = "ue.fixed";
	ipcp_name.processInstance = "1";
        if (IPCManager->create_ipcp(this, &c_promise, ipcp_name, rina::NORMAL_IPC_PROCESS, "fixed.DIF") == IPCM_FAILURE
        		|| c_promise.wait() != IPCM_SUCCESS) {
        	LOG_WARN("Problems creating IPCP %s of type %s",
        			ipcp_name.toString().c_str(),
				rina::NORMAL_IPC_PROCESS.c_str());
        	return -1;
        }

        //Assign to DIF
        if (IPCManager->dif_template_manager->get_dif_template("fixed.dif", dif_template) != 0) {
        	LOG_WARN("Could not find DIF template called fixed.dif");
        	return -1;
        }

        dif_name.processName = "fixed.DIF";
        if (IPCManager->assign_to_dif(this, &promise, c_promise.ipcp_id,
                			dif_template, dif_name) == IPCM_FAILURE
                			|| promise.wait() != IPCM_SUCCESS) {
        	LOG_WARN("Problems assigning IPCP to DIF");
        	return -1;
        }

        //Register at N-1 DIF
        sdif_name.processName = "40";
        if (IPCManager->register_at_dif(this, &promise, c_promise.ipcp_id, sdif_name)
                			== IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
        	LOG_WARN("Problems registering to N-1 DIF");
        	return -1;
        }

	//Enroll to the fixed DIF
        fixed_neigh_data.supportingDifName.processName = "40";
        fixed_neigh_data.difName.processName = "fixed.DIF";
	if(IPCManager->enroll_to_dif(this, &promise, c_promise.ipcp_id, fixed_neigh_data) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP %u to DIF %s via supporting DIF %s",
				c_promise.ipcp_id,
				int_neigh_data.difName.processName.c_str(),
				int_neigh_data.supportingDifName.processName.c_str());
		return -1;
	}

	sleep.sleepForMili(500);

	//Enroll to Internet DIF via fixed DIF
	int_neigh_data.supportingDifName.processName = "fixed.DIF";
	int_neigh_data.difName.processName = "internet.DIF";
	neighbor.processName = "core1.internet";
	neighbor.processInstance = "1";
	if(IPCManager->enroll_to_dif(this, &promise, 5, int_neigh_data, true, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP 5 to DIF %s via supporting DIF %s",
				int_neigh_data.difName.processName.c_str(),
				int_neigh_data.supportingDifName.processName.c_str());
		return -1;
	}

	//4 Now it is multihomed, wait N seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(2500);

	if(IPCManager->disconnect_neighbor(this, &promise, 5, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP 5");
		return -1;
	}

	LOG_DBG("Handover done!");

	hand_state.dif = "arcfire_up";

	//Re-schedule handover task
	HandoverTimerTask * task = new HandoverTimerTask(this);
	timer.scheduleTask(task, 60000);

	return 0;
}

int MobilityManager::execute_handover_arcfire_exp5_mac_fixed_wifi()
{
	Promise promise;
	rina::ApplicationProcessNamingInformation ipcp_name, dif_name, sdif_name;
	NeighborData fixed_neigh_data, int_neigh_data;
	rina::Sleep sleep;
	rina::ApplicationProcessNamingInformation neighbor;

	//Enroll to Internet DIF via mobile DIF
	int_neigh_data.supportingDifName.processName = "mobile.DIF";
	int_neigh_data.difName.processName = "internet.DIF";
	neighbor.processName = "core2.internet";
	neighbor.processInstance = "1";
	if(IPCManager->enroll_to_dif(this, &promise, 5, int_neigh_data, true, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems enrolling IPCP 5 to DIF %s via supporting DIF %s",
				int_neigh_data.difName.processName.c_str(),
				int_neigh_data.supportingDifName.processName.c_str());
		return -1;
	}

	//4 Now it is multihomed, wait N seconds and break connectivity through former N-1 DIF
	sleep.sleepForMili(2500);

	if(IPCManager->disconnect_neighbor(this, &promise, 5, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP 5");
		return -1;
	}

	neighbor.processName = "cpe1.fixed";
	neighbor.processInstance = "1";

	if(IPCManager->disconnect_neighbor(this, &promise, 7, neighbor) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS) {
		LOG_WARN("Problems invoking disconnect from neighbor on IPCP 7");
		return -1;
	}

	sleep.sleepForMili(500);

	if (IPCManager->destroy_ipcp(this, 7) == IPCM_FAILURE) {
		LOG_WARN("Problems destroying IPCP 7");
	}

	LOG_DBG("Handover done!");

	hand_state.dif = "pristine_up";

	//Re-schedule handover task
	HandoverTimerTask * task = new HandoverTimerTask(this);
	timer.scheduleTask(task, 20000);

	return 0;
}

void HandoverTimerTask::run()
{
	mobman->execute_handover();
}

void BoostrapTimerTask::run()
{
	mobman->boostrap();
}

void InitializeTimerTask::run()
{
	mobman->initialize();
}

} //namespace rinad
