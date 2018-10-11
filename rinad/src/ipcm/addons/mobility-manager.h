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

#ifndef __MOBILITY_MANAGER_H__
#define __MOBILITY_MANAGER_H__

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/timer.h>

#include "rina-configuration.h"
#include "../addon.h"
#include "../ipcp.h"

namespace rinad{

struct WirelessDIFInfo {
	std::string bs_address;
	std::string dif_name;
};

struct HandoverState {
	std::string dif;
	IPCMIPCProcess * ipcp;
	bool first_report;
	std::string hand_type;
	bool change_mob_dif;
	int disc_wait_time_ms;
	int hand_period_ms;
};

//
// @brief Handles the mobility of the system locally (decides when to
// change attachment to Base Stations, etc.)
//
//
class MobilityManager: public AppAddon {
public:
	static const std::string NAME;
	static const std::string ARCFIRE_EXP5_OMEC_ROAMING;
	static const std::string ARCFIRE_EXP5_2OPERATOR_DMM;
	static const std::string ARCFIRE_EXP5_MAC;
	static const std::string ARCFIRE_EXP5_TIP_UE1;
	static const std::string ARCFIRE_EXP5_TIP_UE2;
	static const int DEFAULT_DISC_WAIT_TIME_MS;
	static const int DEFAULT_HANDOVER_PERIOD_MS;
	static const int DEFAULT_BOOTSTRAP_WAIT_TIME_MS;

	MobilityManager(const rinad::RINAConfiguration& config);
	virtual ~MobilityManager(void) {
	};

	void boostrap(void);
	void initialize(void);
	void execute_handover(void);

protected:
	int boostrap_exp5(void);
	int initialize_arcfire_exp5_omec(void);
	int initialize_arcfire_exp5_2operator_dmm(void);
	int initialize_arcfire_exp5_mac(void);
	int initialize_arcfire_exp5_tip(bool ue1);
	int execute_handover_arcfire_exp5_omec(void);
	int excecute_handover_arcfire_exp5_2operator_dmm(void);
	int execute_handover_arcfire_exp5_mac(void);
	int execute_handover_arcfire_exp5_mac_wifi_hand(void);
	int execute_handover_arcfire_exp5_mac_wifi_fixed(void);
	int execute_handover_arcfire_exp5_mac_fixed_wifi(void);
	int execute_handover_arcfire_exp5_tip(bool ue1);

	//Process flow event
	void process_librina_event(rina::IPCEvent** event);

	//Process ipcm event
	void process_ipcm_event(const IPCMEvent& event);

private:
	IPCMIPCProcessFactory * factory;
	rina::Timer timer;
	rina::MediaReport last_media_report;
	HandoverState hand_state;
	rina::Lockable lock;

	void parse_configuration(const rinad::RINAConfiguration& config);

	//Called when a media report by an IPC Process has been delivered
	void media_reports_handler(rina::MediaReportEvent *event);
};

class HandoverTimerTask: public rina::TimerTask {
public:
	HandoverTimerTask(MobilityManager * mm): mobman(mm){};
	~HandoverTimerTask() throw() {};
	void run();
	std::string name() const {
		return "mobman-handover";
	}

private:
	MobilityManager * mobman;
};

class BoostrapTimerTask: public rina::TimerTask {
public:
	BoostrapTimerTask(MobilityManager * mm): mobman(mm){};
	~BoostrapTimerTask() throw() {};
	void run();
	std::string name() const {
		return "mobman-bootstrap";
	}

private:
	MobilityManager * mobman;
};

class InitializeTimerTask: public rina::TimerTask {
public:
	InitializeTimerTask(MobilityManager * mm): mobman(mm){};
	~InitializeTimerTask() throw() {};
	void run();
	std::string name() const {
		return "mobman-initialize";
	}

private:
	MobilityManager * mobman;
};

}//rinad namespace

#endif  /* __MOBILITY_MANAGER_H__ */
