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

class ShimWifiIPCProcessImpl;

typedef struct network_key_s {
	std::string ssid;
	std::string bssid;
} network_key_t;

inline bool operator<(const network_key_t& k1, const network_key_t& k2){
	return memcmp((const void*)&k1, (const void*)&k2,
						sizeof(network_key_t)) < 0;
};

class WpaController {
public:
	WpaController(ShimWifiIPCProcessImpl * ipcp,
						const std::string& type,
						const std::string& folder);
	~WpaController();
	int launch_wpa(const std::string& wif_name);
	int create_ctrl_connection(const std::string& if_name);
	int scan(void);
	int scan_results(std::string& output);
	int enable_network(const std::string& network, std::string& output);
	int disable_network(const std::string& network, std::string& output);
	int select_network(const std::string& network, std::string& output);

private:
	rina::Lockable * lock;

	//Owner Shim WiFi IPCP
	ShimWifiIPCProcessImpl * ipcp;

	//Monitoring thread
	rina::Thread * mon_thread;
	rina::ThreadAttributes mon_thread_attrs;
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
	std::map<network_key_t, unsigned int> network_map;

	static void * __mon_trampoline(void * opaque);
	void __mon_loop(void);
	void __process_msg(std::string msg);
	int __send_command(const std::string& cmd, std::string& output);
};

} //namespace rinad

#endif
