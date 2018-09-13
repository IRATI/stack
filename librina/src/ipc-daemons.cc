//
// Common librina classes for IPC Process and IPC Manager daemons
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <sstream>

#define RINA_PREFIX "librina.ipc-daemons"

#include "librina/logs.h"
#include "librina/ipc-daemons.h"
#include "core.h"

namespace rina {

/* CLASS ASSIGN TO DIF RESPONSE EVENT */
AssignToDIFResponseEvent::AssignToDIFResponseEvent(
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
                        BaseResponseEvent(result,
                                        ASSIGN_TO_DIF_RESPONSE_EVENT,
                                        sequenceNumber, ctrl_port, ipcp_id) {
}

/* CLASS SET POLICY SET PARAM RESPONSE EVENT */
SetPolicySetParamResponseEvent::SetPolicySetParamResponseEvent(
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id) :
			IPCEvent(IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE,
                                         sequenceNumber, ctrl_port, ipcp_id)
{ this->result = result; }

/* CLASS SELECT POLICY SET RESPONSE EVENT */
SelectPolicySetResponseEvent::SelectPolicySetResponseEvent(
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id) :
			IPCEvent(IPC_PROCESS_SELECT_POLICY_SET_RESPONSE,
                                         sequenceNumber, ctrl_port, ipcp_id)
{ this->result = result; }

/* CLASS PLUGIN LOAD RESPONSE EVENT */
PluginLoadResponseEvent::PluginLoadResponseEvent(
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id) :
			IPCEvent(IPC_PROCESS_PLUGIN_LOAD_RESPONSE,
                                         sequenceNumber, ctrl_port, ipcp_id)
{ this->result = result; }

/* CLASS FWD CDAP MSG REQUEST EVENT */
FwdCDAPMsgRequestEvent::FwdCDAPMsgRequestEvent(const ser_obj_t& sm,
				 int result, unsigned int sequenceNumber,
				 unsigned int ctrl_port, unsigned short ipcp_id) :
				IPCEvent(IPC_PROCESS_FWD_CDAP_MSG,
                                         sequenceNumber, ctrl_port, ipcp_id)
{
        this->sermsg = sm;
	this->result = result;
}

/* CLASS FWD CDAP MSG REQUEST EVENT */
FwdCDAPMsgResponseEvent::FwdCDAPMsgResponseEvent(const ser_obj_t& sm,
                                 int result, unsigned int sequenceNumber,
				 unsigned int ctrl_port, unsigned short ipcp_id) :
                                IPCEvent(IPC_PROCESS_FWD_CDAP_RESPONSE_MSG,
                                         sequenceNumber, ctrl_port, ipcp_id)
{
        this->sermsg = sm;
        this->result = result;
}

bool BaseStationInfo::operator==(const BaseStationInfo &other) const
{
	return other.ipcp_address == ipcp_address &&
			other.signal_strength == signal_strength;
}

bool BaseStationInfo::operator!=(const BaseStationInfo &other) const
{
	return !(*this == other);
}

void BaseStationInfo::from_c_bs_info(BaseStationInfo & bs,
			   	     struct bs_info_entry * bsi)
{
	if (!bsi)
		return;

	bs.ipcp_address = bsi->ipcp_addr;
	bs.signal_strength = bsi->signal_strength;
}

struct bs_info_entry * BaseStationInfo::to_c_bs_info() const
{
	struct bs_info_entry * result;

	result = new bs_info_entry();
	INIT_LIST_HEAD(&result->next);
	result->ipcp_addr = stringToCharArray(ipcp_address);
	result->signal_strength = signal_strength;

	return result;
}

std::string BaseStationInfo::toString() const
{
	std::stringstream ss;

	ss << "IPCP address: " << ipcp_address << std::endl;
	ss << "Signal strength: " << signal_strength << std::endl;

	return ss.str();
}

bool MediaDIFInfo::operator==(const MediaDIFInfo &other) const
{
	return other.available_bs_ipcps.size() == available_bs_ipcps.size() &&
			other.dif_name == dif_name &&
			other.security_policies == security_policies;
}

bool MediaDIFInfo::operator!=(const MediaDIFInfo &other) const
{
	return !(*this == other);
}

void MediaDIFInfo::from_c_media_dif_info(MediaDIFInfo & md,
				         struct media_dif_info * mdi)
{
	struct bs_info_entry * pos;

	if (!mdi)
		return;

	md.dif_name = mdi->dif_name;
	md.security_policies = mdi->sec_policies;

        list_for_each_entry(pos, &(mdi->available_bs_ipcps), next) {
        	BaseStationInfo bs;
        	BaseStationInfo::from_c_bs_info(bs, pos);
        	md.available_bs_ipcps.push_back(bs);
        }
}

struct media_dif_info * MediaDIFInfo::to_c_media_dif_info() const
{
	struct media_dif_info * result;
	struct bs_info_entry * pos;
	std::list<BaseStationInfo>::const_iterator it;

	result = new media_dif_info();
	INIT_LIST_HEAD(&result->available_bs_ipcps);
	result->dif_name = stringToCharArray(dif_name);
	result->sec_policies = stringToCharArray(security_policies);

	for (it = available_bs_ipcps.begin();
			it != available_bs_ipcps.end(); ++it) {
		pos = it->to_c_bs_info();
		list_add_tail(&pos->next, &result->available_bs_ipcps);
	}

	return result;
}

std::string MediaDIFInfo::toString() const
{
	std::stringstream ss;
	std::list<BaseStationInfo>::const_iterator it;

	ss << "DIF Name: " << dif_name << std::endl;
	ss << "Security policies: " << security_policies << std::endl;
	ss << "Available BS IPCPS: " << std::endl;
	for (it = available_bs_ipcps.begin();
			it != available_bs_ipcps.end(); ++it) {
		ss << it->toString();
	}

	return ss.str();
}

bool MediaReport::operator==(const MediaReport &other) const
{
	return other.available_difs.size() == available_difs.size() &&
			other.bs_ipcp_address == bs_ipcp_address &&
			other.current_dif_name == current_dif_name &&
			other.ipcp_id == ipcp_id;
}

bool MediaReport::operator!=(const MediaReport &other) const
{
	return !(*this == other);
}

void MediaReport::from_c_media_report(MediaReport &mr,
				      struct media_report * mre)
{
	struct media_info_entry * pos;

	if (!mre)
		return;

	mr.bs_ipcp_address = mre->bs_ipcp_addr;
	mr.current_dif_name = mre->dif_name;
	mr.ipcp_id = mre->ipcp_id;

	list_for_each_entry(pos, &(mre->available_difs), next) {
		MediaDIFInfo med;
		MediaDIFInfo::from_c_media_dif_info(med, pos->entry);
		mr.available_difs[pos->dif_name] = med;
	}
}

struct media_report * MediaReport::to_c_media_report() const
{
	struct media_report * result;
	struct media_info_entry * pos;
	std::map<std::string, MediaDIFInfo>::const_iterator it;

	result = new media_report();
	INIT_LIST_HEAD(&result->available_difs);
	result->bs_ipcp_addr = stringToCharArray(bs_ipcp_address);
	result->dif_name = stringToCharArray(current_dif_name);
	result->ipcp_id = ipcp_id;

	for(it = available_difs.begin(); it != available_difs.end(); ++it) {
		pos = new media_info_entry();
		INIT_LIST_HEAD(&pos->next);
		pos->dif_name = stringToCharArray(it->first);
		pos->entry = it->second.to_c_media_dif_info();
		list_add_tail(&pos->next, &result->available_difs);
	}

	return result;
}

std::string MediaReport::toString() const
{
	std::stringstream ss;
	std::map<std::string, MediaDIFInfo>::const_iterator it;

	ss << "IPCP id" << ipcp_id << std::endl;
	ss << "DIF Name: " << current_dif_name << std::endl;
	ss << "BS IPCP address: " << bs_ipcp_address << std::endl;
	ss << "Available Media DIFs: " << std::endl;
	for (it = available_difs.begin();
			it != available_difs.end(); ++it) {
		ss << it->second.toString();
	}

	return ss.str();
}

}

