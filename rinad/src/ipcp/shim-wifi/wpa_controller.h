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

#ifndef WPA_CONTROLLER_HH
#define WPA_CONTROLLER_HH

#include <string>

extern "C"{
	#include "wpa_supplicant/wpa_ctrl.h"
}

namespace rinad {

class ShimWifiStaIPCProcessImpl;

class WpaNetwork {
public:
	WpaNetwork();
	WpaNetwork(unsigned int id, std::string ssid, std::string bssid,
							std::string psk);
	unsigned int id;
	std::string ssid;
	std::string bssid;
	std::string psk;
};

class WpaController {
public:
	WpaController(ShimWifiStaIPCProcessImpl * ipcp,
		      const std::string& type,
		      const std::string& folder);
	~WpaController();
	int launch_wpa(const std::string& wif_name, const std::string& driver);
	int create_ctrl_connection(const std::string& if_name);
	int scan(void);
	std::string scan_results(void);
	int enable_network(const std::string& ssid, const std::string& bssid);
	int disable_network(const std::string& ssid, const std::string& bssid);
	int select_network(const std::string& ssid, const std::string& bssid);
	int bssid_reassociate(const std::string& ssid, const std::string& bssid);
	int disconnect(void);

private:
	rina::Lockable lock;

	//Owner Shim WiFi IPCP
	ShimWifiStaIPCProcessImpl * ipcp;

	//Monitoring thread
	rina::Thread * mon_thread;
	bool mon_keep_running;

	std::string prog_name;
	std::string type;
	std::string base_dir;
	pid_t cpid;
	struct wpa_ctrl * ctrl_conn;
	struct wpa_ctrl * mon_conn;
	enum {
		WPA_CREATED,
		WPA_LAUNCHED,
		WPA_CTRL_CONNECTED,
		WPA_ASSOCIATING,
		WPA_ASSOCIATED,
		WPA_KILLED,
	} state;

	//Map to retrieve internal Network id from SSID&BSSID
	std::map<std::string, WpaNetwork> network_map;

	static void * __mon_trampoline(void * opaque);
	void __mon_loop(void);
	void __process_msg(const std::string& msg);
	void __process_try_association_message(const std::string& msg);
	void __process_association_message(const std::string& msg);
	void __process_key_negotiation_message(const std::string& msg);
	void __process_connected_message(const std::string& msg);
	void __process_disconnected_message(const std::string& msg);
	void __process_scan_results_message(void);
	int __send_command(const std::string& cmd);
	std::string __send_command_with_results(const std::string& cmd);
	int __get_network_id_and_set_bssid(const std::string& ssid,
						const std::string& bssid,
						unsigned int& id);
	int __get_network_id(const std::string& ssid, const std::string& bssid,
							unsigned int& id);
	int __common_enable_network(const std::string cmd,
						const std::string& ssid,
						const std::string& bssid);
};

} //namespace rinad

#endif
