/*
 * IPC Manager - Policy management
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <debug.h>

#define RINA_PREFIX "ipcm.policies"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "ipcm.h"
#include "ipcp-handlers.h"

using namespace std;


namespace rinad {

void IPCManager_::ipc_process_set_policy_set_param_response_handler(
				rina::SetPolicySetParamResponseEvent *event)
{
	bool success = (event->result == 0);
	ostringstream ss;

	IPCPTransState* trans = get_transaction_state<IPCPTransState>(event->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown policy set param response received: "<<event->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		assert(0);
		return;
	}

	IPCMIPCProcess* ipcp = lookup_ipcp_by_id(trans->ipcp_id);
	if(!ipcp){
		ss << "Could not complete policy set param. Invalid IPCP id "<< ipcp->get_id();
		FLUSH_LOG(ERR, ss);

		trans->completed(IPCM_FAILURE);
		remove_transaction_state(trans->tid);
		return;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	ss << "set-policy-set-param-op completed on IPC process "
	<< ipcp->get_name().toString() << endl;
	FLUSH_LOG(INFO, ss);

	//Mark as completed
	trans->completed(success ? IPCM_SUCCESS : IPCM_FAILURE);
	remove_transaction_state(trans->tid);
}

void IPCManager_::ipc_process_plugin_load_response_handler(rina::PluginLoadResponseEvent *event)
{
	bool success = (event->result == 0);
	ostringstream ss;
        int ret = -1;

        IPCPpluginTransState* trans = get_transaction_state<IPCPpluginTransState>(event->sequenceNumber);

        if(!trans){
        	ss << ": Warning: unknown plugin load response received: "
        	   << event->sequenceNumber << endl;
        	FLUSH_LOG(WARN, ss);
        	assert(0);
        	return;
        }

        IPCMIPCProcess* ipcp = lookup_ipcp_by_id(trans->ipcp_id);
        if(!ipcp){
        	ss << "Could not complete policy set param. Invalid IPCP id "
        	   << ipcp->get_id();
        	FLUSH_LOG(ERR, ss);

        	trans->completed(IPCM_FAILURE);
        	remove_transaction_state(trans->tid);
        	return;
        }

        //Auto release the read lock
        rina::ReadScopedLock readlock(ipcp->rwlock, false);

        ss << "plugin-load-op [plugin=" << trans->plugin_name <<
	      ", load=" << trans->load << "] completed on IPC process "
	       << ipcp->get_name().toString()
	       << " [success=" << success << "]" << endl;
        FLUSH_LOG(INFO, ss);

	if (success) {
		catalog.plugin_loaded(trans->plugin_name, trans->ipcp_id,
				      trans->load);
	}

	//Mark as completed
	trans->completed(success ? IPCM_SUCCESS : IPCM_FAILURE);
        remove_transaction_state(trans->tid);
}

void IPCManager_::ipc_process_select_policy_set_response_handler(
					rina::SelectPolicySetResponseEvent *event)
{
	bool success = (event->result == 0);
	ostringstream ss;
        int ret = -1;

	IPCPSelectPsTransState* trans = get_transaction_state<IPCPSelectPsTransState>(event->sequenceNumber);

	if(!trans){
		ss << ": Warning: unknown select policy response received: "
			<<event->sequenceNumber<<endl;
		FLUSH_LOG(WARN, ss);
		return;
	}

	IPCMIPCProcess* ipcp = lookup_ipcp_by_id(trans->ipcp_id);
	if(!ipcp){
		ss << "Could not complete policy set param. Invalid IPCP id "<< ipcp->get_id();
		FLUSH_LOG(ERR, ss);

		trans->completed(IPCM_FAILURE);
		remove_transaction_state(trans->tid);
		return;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	ss << "select-policy-set-op completed on IPC process "
	       << ipcp->get_name().toString() <<
		" [success=" << success << "]" << endl;
	FLUSH_LOG(INFO, ss);

	if (success) {
		catalog.policy_set_selected(trans->ps_info, trans->id);
	}

	//Mark as completed
	trans->completed(success ? IPCM_SUCCESS : IPCM_FAILURE);
	remove_transaction_state(trans->tid);
}

} //namespace rinad
