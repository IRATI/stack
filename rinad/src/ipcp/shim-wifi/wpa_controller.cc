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

	if (type == rina::SHIM_WIFI_IPC_PROCESS_AP)
		prog_name = "hostapd";
	else if (type == rina::SHIM_WIFI_IPC_PROCESS_STA)
		prog_name = "wpa-supplicant";
	else
		assert(0);

	state = WPA_CREATED;
}

int WpaController::launch_wpa(const std::string& wif_name){

	rina::ScopedLock g(*lock);

	if(state != WPA_CREATED){
		LOG_IPCP_ERR("%s is already running!", prog_name.c_str());
		return -1;
	}

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

		std::stringstream ss;
		ss << base_dir << "/etc/" << wif_name << ".conf";

		if (type == rina::SHIM_WIFI_IPC_PROCESS_STA) {
			execlp(prog_name.c_str(), prog_name.c_str(),
						"-Dnl80211",
						"-i",
						wif_name.c_str(),
						ss.str().c_str(),
						"-B",
						NULL);
		} else if (type == rina::SHIM_WIFI_IPC_PROCESS_AP) {
			execlp(prog_name.c_str(), prog_name.c_str(), NULL);
		}

		LOG_IPCP_ERR("Problems launching %s", prog_name.c_str());
		exit(EXIT_FAILURE);
	} else{

		LOG_IPCP_DBG("%s launched with PID %d", prog_name.c_str(),
									cpid);
		state = WPA_LAUNCHED;
		return 0;
	}
}

void WpaController::__process_msg(std::string msg){

	if (msg.find("CTRL-EVENT-CONNECTED")) {
		LOG_IPCP_DBG("CTRL-EVENT-CONNECTED event received");
	} else if (msg.find("CTRL-EVENT-DISCONNECTED")) {
		LOG_IPCP_DBG("CTRL-EVENT-DISCONNECTED event received");
	} else if (msg.find("CTRL-EVENT-TERMINATING")) {
		LOG_IPCP_DBG("CTRL-EVENT-TERMINATING event received");
	} else if (msg.find("CTRL-EVENT-SCAN-RESULTS")) {
		LOG_IPCP_DBG("CTRL-EVENT-SCAN-RESLTS event received");
		return ipcp->push_scan_results(msg);
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
	ctrl_conn = wpa_ctrl_open(if_name.c_str());
	if (ctrl_conn = NULL) {
		LOG_IPCP_ERR("Problems connecting to %s ctrl iface",
							prog_name.c_str());
		kill(cpid, SIGKILL);
		state = WPA_KILLED;
		return -1;
	}

	mon_conn = wpa_ctrl_open(if_name.c_str());
	if (mon_conn = NULL) {
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

	//Launch monitoring thread to receive unsolicited events
	mon_thread_attrs.setJoinable();
	mon_thread_attrs.setName(prog_name.append("-monitoring_thread"));
	mon_thread = new rina::Thread(__mon_trampoline, this,
							&mon_thread_attrs);
        mon_thread->start();
	state = WPA_CTRL_CONNECTED;
	return 0;
}

int WpaController::__send_command(const std::string& cmd, std::string& output){

	char buf[4096];
	size_t len;
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
	LOG_IPCP_DBG("%s", buf);
	output = buf;
	return 0;
}

int WpaController::scan(){
	std::string output;
	return __send_command("SCAN", output);
}

int WpaController::scan_results(std::string& output){
	return __send_command("SCAN_RESULTS", output);
}

int WpaController::enable_network(const std::string& network,
							std::string& output){
	std::stringstream ss;
	ss << "ENABLE_NETWORK " << network;
	return __send_command(ss.str().c_str(), output);
}

int WpaController::disable_network(const std::string& network,
							std::string& output){
	std::stringstream ss;
	ss << "DISABLE_NETWORK " << network;
	return __send_command(ss.str().c_str(), output);
}

int WpaController::select_network(const std::string& network,
							std::string& output){
	int rv;
	std::stringstream ss;
	ss << "SELECT_NETWORK " << network;
	rv = __send_command(ss.str().c_str(), output);
	assert(!rv);

}

} //namespace rinad
