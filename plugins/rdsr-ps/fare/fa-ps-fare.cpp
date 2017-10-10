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

#define IPCP_MODULE "flow-allocator-fare"

#include <string>
#include <climits>

#include "ipcp/components.h"

namespace rinad {

class FlowAllocatorPs: public IFlowAllocatorPs {
public:
	FlowAllocatorPs(IFlowAllocator * dm);

	configs::Flow *newFlowRequest(
		IPCProcess * ipc_process,
		const rina::FlowRequestEvent& flowRequestEvent);

	int set_policy_set_param(
		const std::string& name,
		const std::string& value);

	virtual ~FlowAllocatorPs() {}

private:
        // Data model of the security manager component.
        IFlowAllocator * dm;

        rina::QoSCube * selectQoSCube(
		const rina::FlowSpecification& flowSpec);
};

FlowAllocatorPs::FlowAllocatorPs(IFlowAllocator * dm_) : dm(dm_) {

}

configs::Flow * FlowAllocatorPs::newFlowRequest(
	IPCProcess * ipc_process, const rina::FlowRequestEvent& event) {

	configs::Flow* flow;
	rina::QoSCube * qosCube = NULL;

	rina::Connection * connection = NULL;
	std::list<rina::Connection*> connections;

	rina::DTPConfig dtpConfig;
	rina::DTCPConfig dtcpConfig;

	flow = dm->createFlow();
	flow->remote_naming_info = event.remoteApplicationName;
	flow->local_naming_info = event.localApplicationName;
	flow->hop_count = 3;
	flow->max_create_flow_retries = 1;
	flow->source = true;
	flow->state = configs::Flow::ALLOCATION_IN_PROGRESS;

	//
	// Try to select a qos cube which is suitable for this connection.
	//

	try {
		qosCube = selectQoSCube(event.flowSpecification);
	} catch (rina::Exception &e) {
		dm->destroyFlow(flow);
		throw e;
	}

	connection = new rina::Connection();
	connection->portId = event.portId;
	connection->sourceAddress = ipc_process->get_address();
	connection->setQosId(qosCube->id_);
	connection->setFlowUserIpcProcessId(event.flowRequestorIpcProcessId);

	//
	// DTP configuration.
	//

        dtpConfig = rina::DTPConfig(qosCube->get_dtp_config());

        // This just allow the selection of reliable/unreliable qos cube.
        //
	if (event.flowSpecification.maxAllowableGap < 0) {
		dtpConfig.set_max_sdu_gap(INT_MAX);
	} else {
		dtpConfig.set_max_sdu_gap(qosCube->get_max_allowable_gap());
	}

	connection->setDTPConfig(dtpConfig);

	//
	// DTCP configuration.
	//

        dtcpConfig = rina::DTCPConfig(qosCube->get_dtcp_config());
	connection->setDTCPConfig(dtcpConfig);

	// Configure what we consider the minimum granted bandwidth.
	//
	if(event.flowSpecification.averageBandwidth > 0) {
		connection->dtcpConfig.flow_control_config_.rate_based_config_.
			sending_rate_ = event.flowSpecification.averageBandwidth;
	}

	connections.push_back(connection);

	flow->connections = connections;
	flow->current_connection_index = 0;
	flow->flow_specification = event.flowSpecification;

	return flow;
}

rina::QoSCube * FlowAllocatorPs::selectQoSCube(
	const rina::FlowSpecification& flowSpec)
{
        std::list<rina::QoSCube*> qosCubes = dm->ipcp->resource_allocator_->getQoSCubes();
        std::list<rina::QoSCube*>::const_iterator iterator;

        rina::QoSCube* cube;

	if (*(qosCubes.begin()) == NULL) {
		throw rina::Exception("No QoSCubes defined.");
	}

	if (flowSpec.maxAllowableGap < 0) {
	        for (iterator = qosCubes.begin();
			iterator != qosCubes.end();
			++iterator) {

		        cube = *iterator;

		        if (cube->get_dtp_config().is_dtcp_present() &&
			    !cube->get_dtcp_config().is_rtx_control()) {
			        return cube;
	        	}
		}

		for (iterator = qosCubes.begin();
			iterator != qosCubes.end();
			++iterator) {

		        cube = *iterator;

		        if (!cube->get_dtp_config().is_dtcp_present()) {
				return cube;
			}
		}
		return *(qosCubes.begin());
	}

	for (iterator = qosCubes.begin();
		iterator != qosCubes.end();
		++iterator) {

		cube = *iterator;

		if (cube->get_dtp_config().is_dtcp_present()) {
			if (cube->get_dtcp_config().is_rtx_control()) {
				return cube;
			}
		}
	}

	throw rina::Exception("Could not find a suitable QoS Cube.");
}

int FlowAllocatorPs::set_policy_set_param(
	const std::string& name, const std::string& value) {

        return 0;
}

extern "C" rina::IPolicySet *
createFlowAllocatorPs(rina::ApplicationEntity * ctx) {
        IFlowAllocator * sm = dynamic_cast<IFlowAllocator *>(ctx);

        if (!sm) {
                return NULL;
        }

        return new FlowAllocatorPs(sm);
}

extern "C" void
destroyFlowAllocatorPs(rina::IPolicySet * ps) {
        if (ps) {
                delete ps;
        }
}

extern "C" int
get_factories(std::vector<struct rina::PsFactory>& factories)
{
	struct rina::PsFactory fa_factory;

	fa_factory.info.name = "fare";
	fa_factory.info.app_entity = IFlowAllocator::FLOW_ALLOCATOR_AE_NAME;
	fa_factory.create = createFlowAllocatorPs;
	fa_factory.destroy = destroyFlowAllocatorPs;
	factories.push_back(fa_factory);

	return 0;
}

}
