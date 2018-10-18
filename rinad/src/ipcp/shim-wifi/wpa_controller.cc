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

WpaNetwork::WpaNetwork()
{
	id = 0;
}

WpaNetwork::WpaNetwork(unsigned int id, std::string ssid, std::string bssid,
		std::string psk): id(id), ssid(ssid), bssid(bssid), psk(psk){
}

WpaController::WpaController(ShimWifiStaIPCProcessImpl * ipcp_,
			     const std::string& type_,
			     const std::string& folder_)
{
	ipcp = ipcp_;
	type = type_;
	base_dir = folder_;
	ctrl_conn = NULL;
	mon_conn = NULL;
	mon_keep_running = false;
	mon_thread = NULL;

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

int WpaController::launch_wpa(const std::string& wif_name,
			      const std::string& driver){
	std::stringstream ss;
	std::string driver_opts;
	std::string log_file;
	std::string conf_file;

	rina::ScopedLock g(lock);

	if(state != WPA_CREATED){
		LOG_IPCP_ERR("%s is already running!", prog_name.c_str());
		return -1;
	}

	ss << "-D" << driver;
	driver_opts = ss.str();
	ss.str(std::string());
	ss << base_dir << "/var/log/wpas-" << wif_name << ".log";
	log_file = ss.str();
	ss.str(std::string());
	ss << base_dir << "/etc/" << wif_name << ".conf";
	conf_file = ss.str();

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
			LOG_IPCP_DBG("Going to execute: %s %s %s %s %s %s %s %s",
						prog_name.c_str(),
						driver_opts.c_str(),
						"-i",
						wif_name.c_str(),
						"-c",
						conf_file.c_str(),
						"-f",
						log_file.c_str());
			execlp(prog_name.c_str(), prog_name.c_str(),
						driver_opts.c_str(),
						"-i",
						wif_name.c_str(),
						"-c",
						conf_file.c_str(),
						"-f",
						log_file.c_str(),
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
		std::string ssid, bssid, psk;
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
				}else if(start = line.find("psk=")
							!= std::string::npos){
					start+=4;
					psk = line.substr(start,
						line.length() - start);
				}
			}else if(in_network && (line.find("}") != std::string::npos)){
				in_network = false;
				rinad::WpaNetwork nw(id, ssid, bssid, psk);
				network_map[ssid] = nw;
				id++;
				LOG_IPCP_DBG("Added Network %d with SSID %s, BSSID %s and PSK %s",
							nw.id,
							nw.ssid.c_str(),
							nw.bssid.c_str(),
							nw.psk.c_str());
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

void WpaController::__process_msg(const std::string& msg)
{
	LOG_IPCP_DBG("Received message from WPA Supplicant: %s", msg.c_str());

	if (msg.find("Trying to associate with") != std::string::npos) {
		__process_try_association_message(msg);
	} else if (msg.find("Associated with") != std::string::npos) {
		__process_association_message(msg);
	} else if (msg.find("Key negotiation completed with") != std::string::npos) {
		__process_key_negotiation_message(msg);
	} else if (msg.find("CTRL-EVENT-CONNECTED") != std::string::npos) {
		__process_connected_message(msg);
	} else if (msg.find("CTRL-EVENT-DISCONNECTED") != std::string::npos) {
		__process_disconnected_message(msg);
	} else if (msg.find("CTRL-EVENT-TERMINATING") != std::string::npos) {
		LOG_IPCP_DBG("CTRL-EVENT-TERMINATING event received");
	} else if (msg.find("CTRL-EVENT-SCAN-STARTED") != std::string::npos) {
		LOG_IPCP_DBG("CTRL-EVENT-SCAN-STARTED event received");
	} else if (msg.find("CTRL-EVENT-SCAN-RESULTS") != std::string::npos) {
		__process_scan_results_message();
	}else{
		LOG_IPCP_DBG("Ignoring message");
	}
}

void WpaController::__process_try_association_message(const std::string& msg)
{
	std::string delimiter1 = "Trying to associate with ";
	std::string delimiter2 = "SSID='";
	std::string neigh_name;
	std::string dif_name;

	std::string token1 = msg.substr(msg.find(delimiter1) + delimiter1.length(),
				        msg.length() - 1);
	neigh_name = token1.substr(0, token1.find(" "));

	std::string token2 = msg.substr(msg.find(delimiter2) + delimiter2.length(),
				        msg.length() - 1);
	dif_name = token2.substr(0, token2.find("'"));

	LOG_IPCP_DBG("DIF name: %s, neighbor name: %s",
		     dif_name.c_str(),
		     neigh_name.c_str());

	ipcp->notify_trying_to_associate(dif_name, neigh_name);
}

void WpaController::__process_association_message(const std::string& msg)
{
	std::string delimiter = "Associated with ";
	std::string neigh_name;

	neigh_name = msg.substr(msg.find(delimiter) + delimiter.length(),
			        msg.length() - 1);
	LOG_IPCP_DBG("Neighbor name: %s", neigh_name.c_str());

	ipcp->notify_associated(neigh_name);
}

void WpaController::__process_key_negotiation_message(const std::string& msg)
{
	std::string delimiter = "WPA: Key negotiation completed with ";
	std::string neigh_name;

	std::string token = msg.substr(msg.find(delimiter) + delimiter.length(),
				        msg.length() - 1);
	neigh_name = token.substr(0, token.find(" "));

	LOG_IPCP_DBG("Neighbor name: %s", neigh_name.c_str());

	ipcp->notify_key_negotiated(neigh_name);
}

void WpaController::__process_connected_message(const std::string& msg)
{
	std::string delimiter = "Connection to ";
	std::string neigh_name;

	std::string token = msg.substr(msg.find(delimiter) + delimiter.length(),
				        msg.length() - 1);
	neigh_name = token.substr(0, token.find(" "));

	LOG_IPCP_DBG("Neighbor name: %s", neigh_name.c_str());

	ipcp->notify_connected(neigh_name);
}

void WpaController::__process_disconnected_message(const std::string& msg)
{
	std::string delimiter = "bssid=";
	std::string neigh_name;

	std::string token = msg.substr(msg.find(delimiter) + delimiter.length(),
				        msg.length() - 1);
	neigh_name = token.substr(0, token.find(" "));

	LOG_IPCP_DBG("Neighbor name: %s", neigh_name.c_str());

	ipcp->notify_disconnected(neigh_name);
}

void WpaController::__process_scan_results_message()
{
	ipcp->notify_scan_results();
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

	rina::ScopedLock g(lock);

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
	mon_thread = new rina::Thread(__mon_trampoline, this,
				      prog_name.append("-monitoring_thread"), false);
        mon_thread->start();
	state = WPA_CTRL_CONNECTED;
	return 0;
}

int WpaController::__send_command(const std::string& cmd)
{
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
		LOG_IPCP_ERR("'%s' command failed.\n", cmd.c_str());
		return -1;
	}

	LOG_IPCP_DBG("Sent command to WPA supplicant: %s", cmd.c_str());

	buf[len] = '\0';

	LOG_IPCP_DBG("Got reply from WPA supplicant: %s", buf);

	return (strncmp(buf, "OK", 2)) ? -1 : 0;
}

std::string WpaController::__send_command_with_results(const std::string& cmd)
{
	char buf[4096];
	size_t len = sizeof(buf);
	int ret;

	if (ctrl_conn == NULL) {
		LOG_IPCP_ERR("Could not send command %s, not connected to %s ctrl iface",
							cmd.c_str(),
							prog_name.c_str());
		return "";
	}

	ret = wpa_ctrl_request(ctrl_conn, cmd.c_str(), cmd.length(), buf, &len,
									NULL);

	if (ret == -2) {
		LOG_IPCP_ERR("'%s' command timed out.\n", cmd.c_str());
		return "";
	} else if (ret < 0) {
		LOG_IPCP_ERR("'%s' command failed.\n", cmd.c_str());
		return "";
	}

	LOG_IPCP_DBG("Sent command to WPA supplicant: %s", cmd.c_str());

	buf[len] = '\0';

	LOG_IPCP_DBG("Got reply from WPA supplicant: %s", buf);

	return std::string(buf);
}

int WpaController::scan()
{
	std::string cmd = "SCAN";
	return __send_command(cmd.c_str());
}

std::string WpaController::scan_results()
{
	std::string cmd = "SCAN_RESULTS";
	return __send_command_with_results(cmd.c_str());
}

int WpaController::__get_network_id_and_set_bssid(const std::string& ssid,
						const std::string& bssid,
						unsigned int& id)
{
	if(network_map.find(ssid) == network_map.end())
		return -1;

	network_map[ssid].bssid = bssid;

	id = network_map[ssid].id;
	return 0;
}

int WpaController::__get_network_id(const std::string& ssid,
						const std::string& bssid,
						unsigned int& id)
{
	if(network_map.find(ssid) == network_map.end())
		return -1;

	if(network_map[ssid].bssid != bssid)
		return -1;

	id = network_map[ssid].id;
	return 0;
}

int WpaController::__common_enable_network(const std::string cmd,
						const std::string& ssid,
						const std::string& bssid)
{
	int rv;
	unsigned int id;
	std::stringstream ss;

	rina::ScopedLock g(lock);

	if(ssid == "all"){
		ss << cmd << " all";
		return __send_command(ss.str().c_str());
	}

	if(__get_network_id_and_set_bssid(ssid, bssid, id) < 0)
		return -1;

	ss << "BSSID " << id << " " << bssid;
	rv = __send_command(ss.str().c_str());
	if (rv != 0) {
		LOG_WARN("Command %s returned error", ss.str().c_str());
		return rv;
	}

	ss.str(std::string());

	ss << cmd << " " << id;
	return __send_command(ss.str().c_str());
}

int WpaController::disable_network(const std::string& ssid,
						const std::string& bssid){
	int rv;
	unsigned int id;
	std::stringstream ss;

	rina::ScopedLock g(lock);

	if(ssid == "all"){
		ss << "DISABLE_NETWORK all";
		return __send_command(ss.str().c_str());
	}

	if(__get_network_id(ssid, bssid, id) < 0)
		return -1;

	ss << "DISABLE_NETWORK " << id;
	return __send_command(ss.str().c_str());
}

int WpaController::enable_network(const std::string& ssid,
						const std::string& bssid){
	return __common_enable_network(std::string("ENABLE_NETWORK"), ssid,
									bssid);
}

int WpaController::select_network(const std::string& ssid,
						const std::string& bssid){
	return __common_enable_network(std::string("SELECT_NETWORK"), ssid,
									bssid);
}

int WpaController::bssid_reassociate(const std::string& ssid, const std::string& bssid)
{
	int rv;
	unsigned int id;
	std::stringstream ss;

	rina::ScopedLock g(lock);

	if(__get_network_id_and_set_bssid(ssid, bssid, id) < 0)
		return -1;

	ss << "BSSID " << id << " " << bssid;
	rv = __send_command(ss.str().c_str());
	if (rv != 0) {
		LOG_WARN("Command %s returned error", ss.str().c_str());
		return rv;
	}

	ss.str(std::string());

	ss << "REASSOCIATE";
	return __send_command(ss.str().c_str());
}

int WpaController::disconnect()
{
	std::string cmd = "DISCONNECT";
	return __send_command(cmd.c_str());
}

} //namespace rinad
