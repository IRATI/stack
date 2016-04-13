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

namespace rina {

/* CLASS ASSIGN TO DIF RESPONSE EVENT */
AssignToDIFResponseEvent::AssignToDIFResponseEvent(
                int result, unsigned int sequenceNumber):
                        BaseResponseEvent(result,
                                        ASSIGN_TO_DIF_RESPONSE_EVENT,
                                        sequenceNumber) {
}

/* CLASS SET POLICY SET PARAM RESPONSE EVENT */
SetPolicySetParamResponseEvent::SetPolicySetParamResponseEvent(
                int result, unsigned int sequenceNumber) :
			IPCEvent(IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE,
                                         sequenceNumber)
{ this->result = result; }

/* CLASS SELECT POLICY SET RESPONSE EVENT */
SelectPolicySetResponseEvent::SelectPolicySetResponseEvent(
                int result, unsigned int sequenceNumber) :
			IPCEvent(IPC_PROCESS_SELECT_POLICY_SET_RESPONSE,
                                         sequenceNumber)
{ this->result = result; }

/* CLASS PLUGIN LOAD RESPONSE EVENT */
PluginLoadResponseEvent::PluginLoadResponseEvent(
                int result, unsigned int sequenceNumber) :
			IPCEvent(IPC_PROCESS_PLUGIN_LOAD_RESPONSE,
                                         sequenceNumber)
{ this->result = result; }

/* CLASS FWD CDAP MSG REQUEST EVENT */
FwdCDAPMsgRequestEvent::FwdCDAPMsgRequestEvent(const ser_obj_t& sm,
				 int result, unsigned int sequenceNumber) :
				IPCEvent(IPC_PROCESS_FWD_CDAP_MSG,
                                         sequenceNumber)
{
        this->sermsg = sm;
	this->result = result;
}

/* CLASS FWD CDAP MSG REQUEST EVENT */
FwdCDAPMsgResponseEvent::FwdCDAPMsgResponseEvent(const ser_obj_t& sm,
                                 int result, unsigned int sequenceNumber) :
                                IPCEvent(IPC_PROCESS_FWD_CDAP_RESPONSE_MSG,
                                         sequenceNumber)
{
        this->sermsg = sm;
        this->result = result;
}

}

