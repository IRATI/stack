/*
 *  C wrapper for librina
 *
 *    Addy Bombeke      <addy.bombeke@ugent.be>
 *    Sander Vrijders   <sander.vrijders@intec.ugent.be>
 *    Dimitri Staessens <dimitri.staessens@intec.ugent.be>
 *
 *    Copyright (C) 2015
 *
 *  This software was written as part of the MSc thesis in electrical
 *  engineering,
 *
 *  "Comparing RINA to TCP/IP for latency-constrained applications",
 *
 *  at Ghent University, Academic Year 2014-2015
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <string>
#include <librina/librina.h>
#include "librina-c/librina-c.h"

using namespace std;
using namespace rina;

extern "C"
{

unsigned int rinaIPCManager_requestApplicationRegistration(
	const char *app,
	const char *app_instance,
	const char *dif)
{
	ApplicationRegistrationInformation ari;
	ari.ipcProcessId = 0; //application, not IPC process
	ari.appName = ApplicationProcessNamingInformation(app,
							  app_instance);
	if(dif) {
		ari.applicationRegistrationType =
			static_cast<rina::ApplicationRegistrationType>
			(0);  /* APPLICATION_REGISTRATION_SINGLE_DIF */
		ari.difName = ApplicationProcessNamingInformation(
			dif,
			string());
	}
	else {
		ari.applicationRegistrationType =
			static_cast<rina::ApplicationRegistrationType>
			(1);  /* APPLICATION_REGISTRATION_ANY_DIF */
	}
	return ipcManager->requestApplicationRegistration(ari);
}

void rinaIPCManager_appUnregistrationResult(unsigned int seqnum,
					    char         success)
{
	ipcManager->appUnregistrationResult(seqnum, success);
}

void rinaIPCManager_commitPendingRegistration(unsigned int seqnum,
					      const char  *dif)
{
	if(dif)
		ipcManager->commitPendingRegistration(
			seqnum,
			ApplicationProcessNamingInformation(
				dif,
				string()));
	else
		ipcManager->commitPendingRegistration(
			seqnum,
			ApplicationProcessNamingInformation(
				string(),
				string()));
}
void rinaIPCManager_withdrawPendingRegistration(unsigned int seqnum)
{
	ipcManager->withdrawPendingFlow(seqnum);
}

rina_flow_t rinaIPCManager_allocateFlowResponse(rina_ipcevent *event,
						int            result,
						char           notifySource)
{
	return (ipcManager->allocateFlowResponse(
			*reinterpret_cast<FlowRequestEvent *>(event),
			result, notifySource)).portId;
}

void rinaIPCManager_flowDeallocated(unsigned int port_id)
{
	ipcManager->flowDeallocated(port_id);
}

void rinaIPCManager_flowDeallocationResult(int portId, char success)
{
	ipcManager->flowDeallocationResult(portId, success);
}

unsigned int rinaIPCManager_requestFlowAllocationInDIF(
	const char *localAppName,
	const char *localAppInstance,
	const char *remoteAppName,
	const char *remoteAppInstance,
	const char *difName,
	void       *qosspec)
{
	return ipcManager->requestFlowAllocationInDIF(
		ApplicationProcessNamingInformation(
			localAppName,
			localAppInstance),
		ApplicationProcessNamingInformation(
			remoteAppName,
			remoteAppInstance),
		ApplicationProcessNamingInformation(
			difName,
			string()),
		FlowSpecification());
}

unsigned int rinaIPCManager_requestFlowAllocation(
	const char *localAppName,
	const char *localAppInstance,
	const char *remoteAppName,
	const char *remoteAppInstance,
	void       *qosspec)
{
	return ipcManager->requestFlowAllocation(
		ApplicationProcessNamingInformation(
			localAppName,
			localAppInstance),
		ApplicationProcessNamingInformation(
			remoteAppName,
			remoteAppInstance),
		FlowSpecification());
}

rina_flow_t rinaIPCManager_commitPendingFlow(unsigned int sequenceNumber,
					     int          portId,
					     const char  *DIFName)
{
	return ipcManager->commitPendingFlow(
                  sequenceNumber,
                  portId,
                  ApplicationProcessNamingInformation(
                          DIFName,
                          string())).portId;;
}
}
