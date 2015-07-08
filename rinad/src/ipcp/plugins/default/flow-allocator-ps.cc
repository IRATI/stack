//
// Default policy set for Flow Allocator
//
//    Vincenzo Maffione <v.maffione@nextworks.it>
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

#define IPCP_MODULE "flow-allocator-ps-default"
#include "../../ipcp-logging.h"
#include <string>
#include <climits>

#include "ipcp/components.h"

namespace rinad {

class FlowAllocatorPs: public IFlowAllocatorPs {
public:
	FlowAllocatorPs(IFlowAllocator * dm);
        Flow *newFlowRequest(IPCProcess * ipc_process,
                             const rina::FlowRequestEvent& event);
        int set_policy_set_param(const std::string& name,
                                 const std::string& value);
        virtual ~FlowAllocatorPs() {}

private:
        // Data model of the security manager component.
        IFlowAllocator * dm;

        rina::QoSCube * selectQoSCube(const rina::FlowSpecification& flowSpec);
};

FlowAllocatorPs::FlowAllocatorPs(IFlowAllocator * dm_) : dm(dm_)
{ }


Flow * FlowAllocatorPs::newFlowRequest(IPCProcess * ipc_process,
                                       const rina::FlowRequestEvent&
                                       event)
{
	Flow* flow;
	rina::QoSCube * qosCube = NULL;

	flow = dm->createFlow();
	flow->destination_naming_info = event.remoteApplicationName;
	flow->source_naming_info = event.localApplicationName;
	flow->hop_count = 3;
	flow->max_create_flow_retries = 1;
	flow->source = true;
	flow->state = Flow::ALLOCATION_IN_PROGRESS;

	std::list<rina::Connection*> connections;
	try {
		qosCube = selectQoSCube(event.flowSpecification);
	} catch (rina::Exception &e) {
		dm->destroyFlow(flow);
		throw e;
	}
	LOG_IPCP_DBG("Selected qos cube with name %s", qosCube->get_name().c_str());

	rina::Connection * connection = new rina::Connection();
	connection->portId = event.portId;
	connection->sourceAddress = ipc_process->get_address();
	connection->setQosId(1);
	connection->setFlowUserIpcProcessId(event.flowRequestorIpcProcessId);
        rina::DTPConfig dtpConfig = rina::DTPConfig(
                        qosCube->get_dtp_config());
	if (event.flowSpecification.maxAllowableGap < 0) {
		dtpConfig.set_max_sdu_gap(INT_MAX);
	} else {
		dtpConfig.set_max_sdu_gap(qosCube->get_max_allowable_gap());
	}
	connection->setDTPConfig(dtpConfig);
        rina::DTCPConfig dtcpConfig = rina::DTCPConfig(
                        qosCube->get_dtcp_config());
	connection->setDTCPConfig(dtcpConfig);
	connections.push_back(connection);

	flow->connections = connections;
	flow->current_connection_index = 0;
	flow->flow_specification = event.flowSpecification;

	return flow;
}

rina::QoSCube * FlowAllocatorPs::selectQoSCube(
                const rina::FlowSpecification& flowSpec)
{
        std::list<rina::QoSCube*> qosCubes = dm->getQoSCubes();
        std::list<rina::QoSCube*>::const_iterator iterator;
        rina::QoSCube* cube;

	if (*(qosCubes.begin())==NULL)
	    throw rina::Exception("No QoSCubes defined.");

	if (flowSpec.maxAllowableGap < 0) {
	        for (iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
		        cube = *iterator;
		        if (cube->get_dtp_config().is_dtcp_present()
			    && !cube->get_dtcp_config().is_rtx_control())
			        return cube;
		}
		for (iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
		        cube = *iterator;
		        if (!cube->get_dtp_config().is_dtcp_present()) {
				return cube;
			}
		}
		return *(qosCubes.begin());
	}
        //flowSpec.maxAllowableGap >=0
	for (iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
		cube = *iterator;
		if (cube->get_dtp_config().is_dtcp_present()) {
			if (cube->get_dtcp_config().is_rtx_control()) {
				return cube;
			}
		}
	}
	throw rina::Exception("Could not find a suitable QoS Cube.");
}

int FlowAllocatorPs::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" rina::IPolicySet *
createFlowAllocatorPs(rina::ApplicationEntity * ctx)
{
        IFlowAllocator * sm = dynamic_cast<IFlowAllocator *>(ctx);

        if (!sm) {
                return NULL;
        }

        return new FlowAllocatorPs(sm);
}

extern "C" void
destroyFlowAllocatorPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
