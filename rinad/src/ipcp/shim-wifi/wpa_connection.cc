//
// Implementation of the shim WiFi IPC Process connection to the
// hostapd/wpa-supplicant control interface
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
#include "ipcp/shim-wifi/wpa_connection.h"
#include <librina/common.h>
#include <assert.h>
#include <signal.h>

std::string WPA_CONF_FILE = "/etc/wpa_supplicant/wpa_supplicant.conf";
std::string WPA_CTRL_DIR = "/usr/local/irati/var/";

namespace rinad {

WpaConnection::WpaConnection(const std::string& type_) {
	type = type_;

	if (type == rina::SHIM_WIFI_IPC_PROCESS_AP)
		prog_name = "hostapd";
	else if (type == rina::SHIM_WIFI_IPC_PROCESS_STA)
		prog_name = "wpa-supplicant";
	else
		assert(0);

	state = WPA_CREATED;
}

void WpaConnection::launch_wpa(const std::string& wif_name){
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
			execlp(prog_name.c_str(), prog_name.c_str(),
						"-Dnl80211",
						"-i",
						wif_name.c_str(),
						WPA_CONF_FILE.c_str(),
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
	}
}

void WpaConnection::create_ctrl_connection(const std::string& if_name) {
	ctrl_conn = wpa_ctrl_open(if_name.c_str());
	if (ctrl_conn = NULL) {
		LOG_IPCP_ERR("Problems connecting to %s ctrl iface",
							prog_name.c_str());
		kill(cpid, SIGKILL);
		state = WPA_KILLED;
		throw rina::Exception();
	}
	state = WPA_CONNECTED;

	mon_conn = wpa_ctrl_open(if_name.c_str());
	if (mon_conn = NULL) {
		LOG_IPCP_ERR("Problems connecting to %s monitor iface",
							prog_name.c_str());
		wpa_ctrl_close(ctrl_conn);
		kill(cpid, SIGKILL);
		state = WPA_KILLED;
		throw rina::Exception();
	}
	state = WPA_ATTACHED;
}


} //namespace rinad
