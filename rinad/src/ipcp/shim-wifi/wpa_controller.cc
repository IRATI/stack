//
// Implementation of the shim WiFi IPC Process' WPA controller to manage
// hostapd/wpa-supplicant process and its control interface
//
//    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#define IPCP_MODULE "shim-wifi-ipcp"
#include "ipcp-logging.h"
#include "ipcp/ipc-process.h"
#include "ipcp/shim-wifi/wpa_controller.h"
#include "ipcp/shim-wifi/shim-wifi-ipc-process.h"
#include <librina/common.h>
#include <assert.h>
#include <signal.h>
#include <fstream>

namespace rinad {

WpaController::WpaController(ShimWifiIPCProcessImpl * ipcp_,
						const std::string& type_,
						const std::string& folder_) {
	lock = new rina::Lockable();
	ipcp = ipcp_;
	type = type_;
	base_dir = folder_;
	ctrl_conn = NULL;
	mon_conn = NULL;
	mon_keep_running = false;
	cpid = -1;

	if (type == rina::SHIM_WIFI_IPC_PROCESS_AP)
		prog_name = "hostapd";
	else if (type == rina::SHIM_WIFI_IPC_PROCESS_STA)
		prog_name = "wpa_supplicant";
	else
		assert(0);

	state = WPA_CREATED;
}

WpaController::~WpaController(){

	void* status;

	mon_keep_running = false;

	if(ctrl_conn){
		LOG_IPCP_DBG("Closing ctrl connection...");
		wpa_ctrl_close(ctrl_conn);
	}
	if(mon_conn){
		LOG_IPCP_DBG("Closing monitoring connection...");
		wpa_ctrl_detach(mon_conn);
		wpa_ctrl_close(mon_conn);
	}

	if(mon_thread){
		LOG_IPCP_DBG("Joining monitoring thread...");
		mon_thread->join(&status);
		delete mon_thread;
	}
}

int WpaController::launch_wpa(const std::string& wif_name){

	rina::ScopedLock g(*lock);

	if(state != WPA_CREATED){
		LOG_IPCP_ERR("%s is already running!", prog_name.c_str());
		return -1;
	}

	std::stringstream ss;
	ss << base_dir << "/etc/" << wif_name << ".conf";

	cpid = fork();
	if (cpid < 0) {
		LOG_IPCP_ERR("Problems forking %s", prog_name.c_str());
		exit(EXIT_FAILURE);
	} else if (cpid == 0) {
		//child
		int dnfd;
		dnfd = open("/dev/null", O_WRONLY);
		dup2(dnfd, STDOUT_FILENO);
		dup2(STDOUT_FILENO, STDERR_FILENO);

		if (type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
			LOG_IPCP_DBG("Going to execute: %s %s %s %s %s %s",
						prog_name.c_str(),
						"-Dnl80211",
						"-i",
						wif_name.c_str(),
						"-c",
						ss.str().c_str());
			execlp(prog_name.c_str(), prog_name.c_str(),
						"-Dnl80211",
						"-i",
						wif_name.c_str(),
						"-c",
						ss.str().c_str(),
						NULL);
		} else if (type == rina::SHIM_WIFI_IPC_PROCESS_AP) {
			execlp(prog_name.c_str(), prog_name.c_str(), NULL);
		}

		LOG_IPCP_ERR("Problems launching %s", prog_name.c_str());
		exit(EXIT_FAILURE);
	} else{

		std::ifstream file(ss.str().c_str());
		std::string line;
		bool in_network = false;
		std::string ssid, bssid;
		int start, end;
		unsigned int id = 0;
		while (std::getline(file, line)){
			if(line.find("#") == 0)
				continue;
			if(in_network && (line.find("}") == std::string::npos)){
				if(start = line.find("bssid=")
							!= std::string::npos){
					start+=6;
					bssid = line.substr(start,
						line.length() - start);
				}else if(start = line.find("ssid=\"")
							!= std::string::npos){
					start+=6;
					ssid = line.substr(start,
						line.length() - start -1);
				}
			}else if(in_network && (line.find("}") != std::string::npos)){
				in_network = false;
				network_key_t key =
						{.ssid = ssid, .bssid = bssid};
				network_map[key] = id++;
				LOG_IPCP_DBG("Added Network %d with SSID %s and BSSID %s",
								network_map[key],
								ssid.c_str(),
								bssid.c_str());
			}else if(line.find("network") != std::string::npos){
				in_network = true;
			}
		}

		LOG_IPCP_DBG("%s launched with PID %d", prog_name.c_str(),
									cpid);
		state = WPA_LAUNCHED;
		return 0;
	}
}

void WpaController::__process_msg(std::string msg){

	if (msg.find("CTRL-EVENT-CONNECTED") != std::string::npos) {
		LOG_IPCP_DBG("CTRL-EVENT-CONNECTED event received");
	} else if (msg.find("CTRL-EVENT-DISCONNECTED") != std::string::npos) {
		LOG_IPCP_DBG("CTRL-EVENT-DISCONNECTED event received");
	} else if (msg.find("CTRL-EVENT-TERMINATING") != std::string::npos) {
		LOG_IPCP_DBG("CTRL-EVENT-TERMINATING event received");
	} else if (msg.find("CTRL-EVENT-SCAN-STARTED") != std::string::npos) {
		LOG_IPCP_DBG("CTRL-EVENT-SCAN-STARTED event received");
	} else if (msg.find("CTRL-EVENT-SCAN-RESULTS") != std::string::npos) {
		LOG_IPCP_DBG("CTRL-EVENT-SCAN-RESULTS event received");
	}else{
		LOG_IPCP_DBG("Received not expected message %s", msg.c_str());
	}
}

void * WpaController::__mon_trampoline(void * opaque){
	WpaController * instance = static_cast<WpaController*>(opaque);
	instance->mon_keep_running = true;
	instance->__mon_loop();
	return NULL;
}

void WpaController::__mon_loop() {

	LOG_IPCP_DBG("Starting monitoring events loop...");

	while(mon_keep_running){
		while (wpa_ctrl_pending(mon_conn) > 0) {
			char buf[4096];
			size_t len = sizeof(buf) - 1;
			if (wpa_ctrl_recv(mon_conn, buf, &len) == 0) {
				buf[len] = '\0';
				__process_msg(std::string(buf));
			}
		}

		if (wpa_ctrl_pending(mon_conn) < 0) {
			LOG_IPCP_ERR("Connection to %s monitoring interface lost!",
							prog_name.c_str());
		}
	}
}

int WpaController::create_ctrl_connection(const std::string& if_name) {

	rina::ScopedLock g(*lock);

	std::stringstream ss;
	ss << base_dir << "/var/run/" << if_name;

	LOG_IPCP_DBG("Connecting to control interface %s",
						ss.str().c_str());
	ctrl_conn = wpa_ctrl_open(ss.str().c_str());
	if (!ctrl_conn) {
		LOG_IPCP_ERR("Problems connecting to %s ctrl iface",
							prog_name.c_str());
		kill(cpid, SIGKILL);
		state = WPA_KILLED;
		return -1;
	}

	mon_conn = wpa_ctrl_open(ss.str().c_str());
	if (!mon_conn) {
		LOG_IPCP_ERR("Problems connecting to %s monitor iface",
							prog_name.c_str());
		wpa_ctrl_close(ctrl_conn);
		kill(cpid, SIGKILL);
		state = WPA_KILLED;
		return -1;
	}

	int rv = wpa_ctrl_attach(mon_conn);
	if (rv != 0) {
		LOG_IPCP_ERR("Problems attaching to %s monitor iface",
							prog_name.c_str());
		wpa_ctrl_close(ctrl_conn);
		wpa_ctrl_close(mon_conn);
		kill(cpid, SIGKILL);
		state = WPA_KILLED;
		return -1;
	}

	LOG_IPCP_DBG("Connected to %s Ctrl and Monitoring interfaces",
							prog_name.c_str());

	//Launch monitoring thread to receive unsolicited events
	mon_thread_attrs.setJoinable();
	mon_thread_attrs.setName(prog_name.append("-monitoring_thread"));
	mon_thread = new rina::Thread(__mon_trampoline, this,
							&mon_thread_attrs);
        mon_thread->start();
	state = WPA_CTRL_CONNECTED;
	return 0;
}

int WpaController::__send_command(const std::string& cmd,
						std::string * out = NULL){

	char buf[4096];
	size_t len = sizeof(buf);
	int ret;

	if (ctrl_conn == NULL) {
		LOG_IPCP_ERR("Could not send command %s, not connected to %s ctrl iface",
							cmd.c_str(),
							prog_name.c_str());
		return -1;
	}

	ret = wpa_ctrl_request(ctrl_conn, cmd.c_str(), cmd.length(), buf, &len,
									NULL);
	if (ret == -2) {
		LOG_IPCP_ERR("'%s' command timed out.\n", cmd.c_str());
		return -2;
	} else if (ret < 0) {
		printf("'%s' command failed.\n", cmd.c_str());
		return -1;
	}

	buf[len] = '\0';
	if(out){
		*out = buf;
		return 0;
	}else{
		return (strncmp(buf, "OK", 2)) ? -1 : 0;
	}
}

int WpaController::scan(){
	std::string cmd = "SCAN";
	int rv = __send_command(cmd.c_str());
	assert(rv == 0);
}

int WpaController::scan_results(std::string& out){
	std::string cmd = "SCAN_RESULTS";
	int rv = __send_command(cmd.c_str(), &out);
	assert(rv == 0);
}

int WpaController::enable_network(const std::string& ssid,
						const std::string& bssid){
	int rv;
	std::stringstream ss;

	if(ssid == "all")
		ss << "ENABLE_NETWORK all";
	else{
		network_key_t key = {.ssid = ssid, .bssid = bssid};
		ss << "ENABLE_NETWORK " << network_map[key];
	}
	return __send_command(ss.str().c_str());
}

int WpaController::disable_network(const std::string& ssid,
						const std::string& bssid){
	int rv;
	std::stringstream ss;

	if(ssid == "all")
		ss << "DISABLE_NETWORK all";
	else{
		network_key_t key = {.ssid = ssid, .bssid = bssid};
		ss << "DISABLE_NETWORK " << network_map[key];
	}
	return __send_command(ss.str().c_str());
}

int WpaController::select_network(const std::string& ssid,
						const std::string& bssid){
	int rv;
	std::stringstream ss;
	network_key_t key = {.ssid = ssid, .bssid = bssid};

	ss << "SELECT_NETWORK " << network_map[key];
	return __send_command(ss.str().c_str());
}

} //namespace rinad
