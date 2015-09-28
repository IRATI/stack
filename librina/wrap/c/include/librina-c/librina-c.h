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

#ifndef LIBRINA_C_H
#define LIBRINA_C_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef	       int rina_flow_t;
struct	       rina_ipcevent;
typedef struct rina_ipcevent rina_ipcevent;

//RINA
void rina_initializePort(unsigned int localport,
			 const char  *loglevel,
			 const char  *pathtologfile);

void rina_initialize(const char *loglevel,  const char *pathtologfile);

//RinaIPCManager
unsigned int rinaIPCManager_requestApplicationRegistration(
	const char *app,
	const char *app_instance,
	const char *dif);

void rinaIPCManager_appUnregistrationResult(unsigned int seqnum, char success);

void rinaIPCManager_commitPendingRegistration(unsigned int seqnum,
					      const char  *dif);

void rinaIPCManager_withdrawPendingRegistration(unsigned int seqnum);

rina_flow_t rinaIPCManager_allocateFlowResponse(rina_ipcevent *event,
						int            result,
						char           notifySource);

void rinaIPCManager_flowDeallocated(unsigned int port_id);

void rinaIPCManager_flowDeallocationResult(int portId, char success);

unsigned int rinaIPCManager_requestFlowAllocationInDIF(
	const char *localAppName,
	const char *localAppInstance,
	const char *remoteAppName,
	const char *remoteAppInstance,
	const char *difName,
	void       *qosspec);

unsigned int rinaIPCManager_requestFlowAllocation(
	const char *localAppName,
	const char *localAppInstance,
	const char *remoteAppName,
	const char *remoteAppInstance,
	void       *qosspec);

rina_flow_t rinaIPCManager_commitPendingFlow(unsigned int sequenceNumber,
					     int          portId,
					     const char  *DIFName);

//RinaIPCEventProducer
rina_ipcevent *rinaIPCEventProducer_eventWait(void);
rina_ipcevent *rinaIPCEventProducer_eventPoll(void);

//RinaIPCEvent
int rinaIPCEvent_eventType(rina_ipcevent *e);
unsigned int rinaIPCEvent_sequenceNumber(rina_ipcevent *e);
char *rinaAllocateFlowRequestResultEvent_difName(rina_ipcevent *e);
char *rinaRegisterApplicationResponseEvent_DIFName(rina_ipcevent *e);
int rinaBaseApplicationRegistrationResponseEvent_result(rina_ipcevent *e);
int rinaAllocateFlowRequestResultEvent_portId(rina_ipcevent *e);

//RinaFlow
int rinaFlow_getPortId(rina_flow_t flow);
int rinaFlow_readSDU(rina_flow_t flow, void *sdu, int maxBytes);
void rinaFlow_writeSDU(rina_flow_t flow, void *sdu, int size);
int rinaFlow_getState(rina_flow_t flow);
char rinaFlow_isAllocated(rina_flow_t flow);

#ifdef __cplusplus
}
#endif

#endif//LIBRINA_C_H
