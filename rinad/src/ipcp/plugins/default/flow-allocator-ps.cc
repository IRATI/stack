//
// Default policy set for Flow Allocator
//
//    Vincenzo Maffione <v.maffione@nextworks.it>
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

#define IPCP_MODULE "flow-allocator-ps-default"
#include "../../ipcp-logging.h"
#include <string>
#include <climits>

#include "ipcp/components.h"

namespace rinad {

class FlowAllocatorPs: public IFlowAllocatorPs {
public:
	FlowAllocatorPs(IFlowAllocator * dm);
	configs::Flow *newFlowRequest(IPCProcess * ipc_process,
			const rina::FlowRequestEvent& flowRequestEvent);
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


configs::Flow * FlowAllocatorPs::newFlowRequest(IPCProcess * ipc_process,
                                       const rina::FlowRequestEvent&
                                       event)
{
	configs::Flow* flow;
	rina::QoSCube * qosCube = NULL;

	flow = dm->createFlow();
	flow->destination_naming_info = event.remoteApplicationName;
	flow->source_naming_info = event.localApplicationName;
	flow->hop_count = 3;
	flow->max_create_flow_retries = 1;
	flow->source = true;
	flow->state = configs::Flow::ALLOCATION_IN_PROGRESS;

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
	connection->setQosId(qosCube->id_);
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
        std::list<rina::QoSCube*> qosCubes = dm->ipcp->resource_allocator_->getQoSCubes();
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

class FlowAllocatorRoundRobinPs: public IFlowAllocatorPs {
public:
	FlowAllocatorRoundRobinPs(IFlowAllocator * dm_) :
		dm(dm_), last_qos_index(0) {};
	configs::Flow *newFlowRequest(IPCProcess * ipc_process,
			const rina::FlowRequestEvent& event);
	int set_policy_set_param(const std::string& name,
			const std::string& value);
	virtual ~FlowAllocatorRoundRobinPs() {}

private:
        // Data model of the security manager component.
        IFlowAllocator * dm;
        int last_qos_index;
};

configs::Flow * FlowAllocatorRoundRobinPs::newFlowRequest(IPCProcess * ipc_process,
                                       	         const rina::FlowRequestEvent& event)
{
	configs::Flow* flow;
	rina::QoSCube * qosCube = NULL;
	std::list<rina::Connection*> connections;
        std::list<rina::QoSCube*> qosCubes = dm->ipcp->resource_allocator_->getQoSCubes();
        std::list<rina::QoSCube*>::const_iterator iterator;
        int count;
        rina::Lockable lock;
        rina::ScopedLock g(lock);

	if (*(qosCubes.begin())==NULL)
	    throw rina::Exception("No QoSCubes defined.");

	if (qosCubes.size() == 1 || last_qos_index == -1 || last_qos_index == qosCubes.size() - 1) {
		qosCube = qosCubes.front();
		last_qos_index = 0;
	} else {
		count = 0;
		for (iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
			if (count <= last_qos_index) {
				count++;
				continue;
			}

			qosCube = *iterator;
			last_qos_index = count;
			break;
		}
	}

	LOG_IPCP_DBG("Selected qos cube with name %s", qosCube->get_name().c_str());

	flow = dm->createFlow();
	flow->destination_naming_info = event.remoteApplicationName;
	flow->source_naming_info = event.localApplicationName;
	flow->hop_count = 3;
	flow->max_create_flow_retries = 1;
	flow->source = true;
	flow->state = configs::Flow::ALLOCATION_IN_PROGRESS;

	rina::Connection * connection = new rina::Connection();
	connection->portId = event.portId;
	connection->sourceAddress = ipc_process->get_address();
	connection->setQosId(qosCube->id_);
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

int FlowAllocatorRoundRobinPs::set_policy_set_param(const std::string& name,
                                            	    const std::string& value)
{
        LOG_IPCP_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

extern "C" rina::IPolicySet *
createFlowAllocatorPs(rina::ApplicationEntity * ctx)
{
        IFlowAllocator * fa = dynamic_cast<IFlowAllocator *>(ctx);

        if (!fa) {
                return NULL;
        }

        return new FlowAllocatorPs(fa);
}

extern "C" void
destroyFlowAllocatorPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

extern "C" rina::IPolicySet *
createFlowAllocatorRoundRobinPs(rina::ApplicationEntity * ctx)
{
        IFlowAllocator * fa = dynamic_cast<IFlowAllocator *>(ctx);

        if (!fa) {
                return NULL;
        }

        return new FlowAllocatorRoundRobinPs(fa);
}

extern "C" void
destroyFlowAllocatorRoundRobinPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
