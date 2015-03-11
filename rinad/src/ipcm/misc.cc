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
#include "helpers.h"
#include "ipcm.h"

using namespace std;


namespace rinad {

void IPCManager_::query_rib_response_event_handler(rina::IPCEvent *e)
{
	DOWNCAST_DECL(e, rina::QueryRIBResponseEvent, event);
	ostringstream ss;
	map<unsigned int, rina::IPCProcess *>::iterator mit;
	rina::IPCProcess *ipcp = NULL;
	bool success = (event->result == 0);
	int ret = -1;

	ss << "Query RIB response event arrived" << endl;
	FLUSH_LOG(INFO, ss);

	mit = IPCManager->pending_ipcp_query_rib_responses.find(event->sequenceNumber);
	if (mit == IPCManager->pending_ipcp_query_rib_responses.end()) {
		ss  << ": Warning: IPC process query RIB "
			"response received, but no corresponding pending "
			"request" << endl;
		FLUSH_LOG(WARN, ss);
	} else {
		ipcp = mit->second;
		if (success) {
			std::stringstream ss;
			list<rina::RIBObjectData>::iterator lit;

			ss << "Query RIB operation completed for IPC "
				<< "process " << ipcp->name.toString() << endl;
			FLUSH_LOG(INFO, ss);
			for (lit = event->ribObjects.begin(); lit != event->ribObjects.end();
					++lit) {
				ss << "Name: " << lit->name_ <<
					"; Class: "<< lit->class_;
				ss << "; Instance: "<< lit->instance_ << endl;
				ss << "Value: " << lit->displayable_value_ <<endl;
				ss << "" << endl;
			}
			IPCManager->query_rib_responses[event->sequenceNumber] = ss.str();
			ret = 0;
		} else {
			ss  << ": Error: Query RIB operation of "
				"process " << ipcp->name.toString() << " failed"
				<< endl;
			FLUSH_LOG(ERR, ss);
		}

		IPCManager->pending_ipcp_query_rib_responses.erase(mit);
	}

	IPCManager->concurrency.set_event_result(ret);
}

void IPCManager_::timer_expired_event_handler(rina::IPCEvent *event)
{
	(void) event;  // Stop compiler barfs
}

} //namespace rinad
