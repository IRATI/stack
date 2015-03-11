/*
 * IPC Manager - Policy management
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>

#define RINA_PREFIX "ipcm.policies"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "helpers.h"
#include "ipcm.h"

using namespace std;


namespace rinad {

void IPCManager_::ipc_process_set_policy_set_param_response_handler(
							rina::IPCEvent *e)
{
	DOWNCAST_DECL(e, rina::SetPolicySetParamResponseEvent, event);
	std::map<unsigned int, rina::IPCProcess *>::iterator mit;
	bool success = (event->result == 0);
	ostringstream ss;
	int ret = -1;

	mit = pending_set_policy_set_param_ops.find(event->sequenceNumber);
	if (mit != pending_set_policy_set_param_ops.end()) {
		pending_set_policy_set_param_ops.erase(mit);
		ss << "set-policy-set-param-op completed on IPC process "
		<< mit->second->name.toString() <<
		" [success=" << success << "]" << endl;
		FLUSH_LOG(INFO, ss);
		ret = event->result;
	} else {
		ss << "Warning: unmatched event received" << endl;
		FLUSH_LOG(WARN, ss);
	}

	concurrency.set_event_result(ret);
}

void IPCManager_::ipc_process_plugin_load_response_handler(rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::PluginLoadResponseEvent, event);
        map<unsigned int, rina::IPCProcess *>::iterator mit;
        bool success = (event->result == 0);
        ostringstream ss;
        int ret = -1;

        mit = pending_plugin_load_ops.find(event->sequenceNumber);
        if (mit != pending_plugin_load_ops.end()) {
                pending_plugin_load_ops.erase(mit);
                ss << "plugin-load-op completed on IPC process "
                       << mit->second->name.toString() <<
                        " [success=" << success << "]" << endl;
                FLUSH_LOG(INFO, ss);
                ret = event->result;
        } else {
                ss << "Warning: unmatched event received" << endl;
                FLUSH_LOG(WARN, ss);
        }

        concurrency.set_event_result(ret);
}

void IPCManager_::ipc_process_select_policy_set_response_handler(
							rina::IPCEvent *e)
{
        DOWNCAST_DECL(e, rina::SelectPolicySetResponseEvent, event);
        map<unsigned int, rina::IPCProcess *>::iterator mit;
        bool success = (event->result == 0);
        ostringstream ss;
        int ret = -1;

        mit = pending_select_policy_set_ops.find(event->sequenceNumber);
        if (mit != pending_select_policy_set_ops.end()) {
                pending_select_policy_set_ops.erase(mit);
                ss << "select-policy-set-op completed on IPC process "
                       << mit->second->name.toString() <<
                        " [success=" << success << "]" << endl;
                FLUSH_LOG(INFO, ss);
                ret = event->result;
        } else {
                ss << "Warning: unmatched event received" << endl;
                FLUSH_LOG(WARN, ss);
        }

        concurrency.set_event_result(ret);
}

} //namespace rinad
