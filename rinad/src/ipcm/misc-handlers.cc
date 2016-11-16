/*
 * IPC Manager - Miscellaneous command handlers
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#define RINA_PREFIX "ipcm.misc"
#include <librina/logs.h>
#include <debug.h>

#include "rina-configuration.h"
#include "misc-handlers.h"

using namespace std;


namespace rinad {

void IPCManager_::query_rib_response_event_handler(rina::QueryRIBResponseEvent *e)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	list<rina::rib::RIBObjectData>::iterator lit;
	RIBqTransState* trans =
		get_transaction_state<RIBqTransState>(e->sequenceNumber);

	if(!trans){
		ss  << ": Warning: IPC process query RIB "
			"response received, but no corresponding pending "
			"request" << endl;
		FLUSH_LOG(WARN, ss);
		assert(0);
		return;
	}

	if(e->result < 0){
		ss  << ": Error: Query RIB operation of failed" << endl;
		trans->completed(IPCM_FAILURE);
		remove_transaction_state(trans->tid);
		return;
	}

	ipcp = lookup_ipcp_by_id(trans->ipcp_id);
	if(!ipcp){
		ss << "Could not complete IPCP query RIB. Invalid IPCP id "<< ipcp->get_id();
		FLUSH_LOG(ERR, ss);

		trans->completed(IPCM_FAILURE);
		remove_transaction_state(trans->tid);
		return;
	}

	//Auto release the read lock
	rina::ReadScopedLock readlock(ipcp->rwlock, false);

	ss << "Query RIB operation completed for IPC "
		<< "process " << ipcp->get_name().toString() << endl;
	FLUSH_LOG(DBG, ss);

	for (lit = e->ribObjects.begin(); lit != e->ribObjects.end();
			++lit) {
		ss << "Name: " << lit->name_ <<
			"; Class: "<< lit->class_;
		ss << "; Instance: "<< lit->instance_ << endl;
		ss << "Value: " << lit->displayable_value_ <<endl;
		ss << "" << endl;
	}

	//Set query RIB response
	QueryRIBPromise* promise = trans->get_promise<QueryRIBPromise>();

	if(!promise){
		assert(0);
		trans->completed(IPCM_FAILURE);
		remove_transaction_state(trans->tid);
		return;
	}

	//Mark as completed
	promise->serialized_rib = ss.str();
	trans->completed(IPCM_SUCCESS);
	remove_transaction_state(trans->tid);

	return;
}

} //namespace rinad
