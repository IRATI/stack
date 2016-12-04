//
// Utilities
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#define IPCP_MODULE "utils"
#include "ipcp-logging.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

#include "utils.h"

namespace rinad {

static int string2int(const std::string& s, int& ret)
{
	char *dummy;
	const char *cstr = s.c_str();

	ret = strtol(cstr, &dummy, 10);
	if (!s.size() || *dummy != '\0') {
		ret = ~0U;
		return -1;
	}

	return 0;
}

static int string2long(const std::string& s, long& ret)
{
	char *dummy;
	const char *cstr = s.c_str();

	ret = strtol(cstr, &dummy, 10);
	if (!s.size() || *dummy != '\0') {
		ret = ~0U;
		return -1;
	}

	return 0;
}

static int string2uint(const std::string& s, unsigned int& ret)
{
	char *dummy;
	const char *cstr = s.c_str();

	ret = strtoul(cstr, &dummy, 10);
	if (!s.size() || *dummy != '\0') {
		ret = ~0U;
		return -1;
	}

	return 0;
}

static int string2ulong(const std::string& s, unsigned long& ret)
{
	char *dummy;
	const char *cstr = s.c_str();

	ret = strtoul(cstr, &dummy, 10);
	if (!s.size() || *dummy != '\0') {
		ret = ~0U;
		return -1;
	}

	return 0;
}


int SysfsHelper::get_value_as_string(std::string& file_name, std::string & value)
{
	std::ifstream sysfs_file(file_name.c_str());
	if (!sysfs_file.is_open()) {
		LOG_IPCP_DBG("Problems opening file: %s", file_name.c_str());
		return -1;
	}

        std::getline(sysfs_file, value);
	if (sysfs_file.fail()) {
		LOG_IPCP_DBG("Problems opening file: %s", file_name.c_str());
		return -1;
	}
	sysfs_file.close();

	return 0;
}

int SysfsHelper::get_value_as_int(std::string& file_name, int & value)
{
	std::string temp;
	int result = get_value_as_string(file_name, temp);
	if (result != 0)
		return result;

	result = string2int(temp, value);
	if (result != 0) {
		LOG_IPCP_ERR("Problems converting string to int: %s", temp.c_str());
		return -1;
	}

	return 0;
}

int SysfsHelper::get_value_as_long(std::string& file_name, long & value)
{
	std::string temp;
	int result = get_value_as_string(file_name, temp);
	if (result != 0)
		return result;

	result = string2long(temp, value);
	if (result != 0) {
		LOG_IPCP_ERR("Problems converting string to long: %s", temp.c_str());
		return -1;
	}

	return 0;
}

int SysfsHelper::get_value_as_uint(std::string& file_name, unsigned int & value)
{
	std::string temp;
	int result = get_value_as_string(file_name, temp);
	if (result != 0)
		return result;

	result = string2uint(temp, value);
	if (result != 0) {
		LOG_IPCP_ERR("Problems converting string to uint: %s", temp.c_str());
		return -1;
	}

	return 0;
}

int SysfsHelper::get_value_as_ulong(std::string& file_name, unsigned long & value)
{
	std::string temp;
	int result = get_value_as_string(file_name, temp);
	if (result != 0)
		return result;

	result = string2ulong(temp, value);
	if (result != 0) {
		LOG_IPCP_ERR("Problems converting string to ulong: %s", temp.c_str());
		return -1;
	}

	return 0;
}

const std::string SysfsHelper::ipcp_prefix = "/sys/rina/ipcps/";
const std::string SysfsHelper::connections = "/connections/";
const std::string SysfsHelper::rmt_ports = "/rmt/n1_ports/";
const std::string SysfsHelper::separator = "/";

int SysfsHelper::get_dtp_tx_bytes(int ipcp_id,
				  int src_cep_id,
				  unsigned long & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << connections
	   << src_cep_id << "/dtp/tx_bytes";
	file_name = ss.str();

	return get_value_as_ulong(file_name, value);
}

int SysfsHelper::get_dtp_rx_bytes(int ipcp_id,
				  int src_cep_id,
				  unsigned long & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << connections
	   << src_cep_id << "/dtp/rx_bytes";
	file_name = ss.str();

	return get_value_as_ulong(file_name, value);
}

int SysfsHelper::get_dtp_tx_pdus(int ipcp_id,
				 int src_cep_id,
				 unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << connections
	   << src_cep_id << "/dtp/tx_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_dtp_rx_pdus(int ipcp_id,
				 int src_cep_id,
				 unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << connections
	   << src_cep_id << "/dtp/rx_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_dtp_drop_pdus(int ipcp_id,
			  	   int src_cep_id,
				   unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << connections
	   << src_cep_id << "/dtp/drop_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_dtp_error_pdus(int ipcp_id,
			  	   int src_cep_id,
				   unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << connections
	   << src_cep_id << "/dtp/err_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}


int SysfsHelper::get_rmt_queued_pdus(int ipcp_id,
			       	     int port_id,
			       	     unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << rmt_ports
	   << port_id << "/queued_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_rmt_drop_pdus(int ipcp_id,
			     	   int port_id,
			       	   unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << rmt_ports
	   << port_id << "/drop_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_rmt_error_pdus(int ipcp_id,
			       	    int port_id,
			       	    unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << rmt_ports
	   << port_id << "/err_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_rmt_tx_pdus(int ipcp_id,
			       	 int port_id,
			       	 unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << rmt_ports
	   << port_id << "/tx_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_rmt_rx_pdus(int ipcp_id,
			       	 int port_id,
			       	 unsigned int & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << rmt_ports
	   << port_id << "/rx_pdus";
	file_name = ss.str();

	return get_value_as_uint(file_name, value);
}

int SysfsHelper::get_rmt_tx_bytes(int ipcp_id,
			       	  int port_id,
			       	  unsigned long & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << rmt_ports
	   << port_id << "/tx_bytes";
	file_name = ss.str();

	return get_value_as_ulong(file_name, value);
}

int SysfsHelper::get_rmt_rx_bytes(int ipcp_id,
			       	  int port_id,
			       	  unsigned long & value)
{
	std::stringstream ss;
	std::string file_name;

	ss << ipcp_prefix << ipcp_id << rmt_ports
	   << port_id << "/rx_bytes";
	file_name = ss.str();

	return get_value_as_ulong(file_name, value);
}

} //namespace rinad
