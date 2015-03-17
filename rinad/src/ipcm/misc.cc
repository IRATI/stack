/*
 * IPC Manager - Miscellaneous command handlers
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

#define RINA_PREFIX "ipcm.misc"
#include <librina/logs.h>

#include "rina-configuration.h"
#include "misc.h"

using namespace std;


namespace rinad {

void IPCManager_::query_rib_response_event_handler(rina::QueryRIBResponseEvent *e)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	list<rina::RIBObjectData>::iterator lit;
	RIBqTransState* trans =
		get_transaction_state<RIBqTransState>(e->sequenceNumber);

	if(!trans){
		ss  << ": Warning: IPC process query RIB "
			"response received, but no corresponding pending "
			"request" << endl;
		FLUSH_LOG(WARN, ss);
		//XXX: invoke the callback
		return;
	}

	if(e->result < 0){
		ss  << ": Error: Query RIB operation of "
			"process " << ipcp->get_name().toString() << " failed"
			<< endl;
		FLUSH_LOG(ERR, ss);

		if(trans->callee){
			//XXX: invoke the callback
			remove_transaction_state(trans->tid);
			delete trans;
		}
		return;
	}

	ipcp = lookup_ipcp_by_id(trans->ipcp_id);
	if(!ipcp){
		ss << "Could not complete IPCP query RIB. Invalid IPCP id "<< ipcp->get_id();
		FLUSH_LOG(ERR, ss);
		if(trans->callee){
			//XXX: invoke the callback
			remove_transaction_state(trans->tid);
			delete trans;
		}
		return;
	}


	ss << "Query RIB operation completed for IPC "
		<< "process " << ipcp->get_name().toString() << endl;
	FLUSH_LOG(INFO, ss);

	for (lit = e->ribObjects.begin(); lit != e->ribObjects.end();
			++lit) {
		ss << "Name: " << lit->name_ <<
			"; Class: "<< lit->class_;
		ss << "; Instance: "<< lit->instance_ << endl;
		ss << "Value: " << lit->displayable_value_ <<endl;
		ss << "" << endl;
	}

	//Set query RIB response
	trans->result = ss.str();

	//Mark as completed
	trans->completed(0);

	//If there was a calle, invoke the callback and remove it. Otherwise
	//transaction is fully complete and originator will clean up the state
	if(trans->callee){
		//Invoke callback
		//FIXME

		//Remove the transaction
		remove_transaction_state(trans->tid);
		delete trans;
	}
}

void IPCManager_::timer_expired_event_handler(rina::IPCEvent *event)
{
	(void) event;  // Stop compiler barfs
}

} //namespace rinad
