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

#ifndef WPA_CONNECTION_HH
#define WPA_CONNECTION_HH

#include <string>

extern "C"{
	#include "wpa_supplicant/wpa_ctrl.h"
}

namespace rinad {

class WpaConnection {
public:
	WpaConnection(const std::string& type);
	~WpaConnection();
	int launch_wpa(const std::string& wif_name);
	int create_ctrl_connection(const std::string& if_name);
	int send_command(const std::string& cmd, bool print);

private:
	std::string prog_name;
	std::string type;
	pid_t cpid;
	struct wpa_ctrl * ctrl_conn;
	struct wpa_ctrl * mon_conn;
	enum {
		WPA_CREATED,
		WPA_LAUNCHED,
		WPA_CONNECTED,
		WPA_ATTACHED,
		WPA_KILLED,
	} state;
};

} //namespace rinad

#endif
