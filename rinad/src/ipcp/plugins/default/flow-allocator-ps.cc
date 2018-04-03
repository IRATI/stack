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
	flow->remote_naming_info = event.remoteApplicationName;
	flow->local_naming_info = event.localApplicationName;
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
	connection->sourceAddress = ipc_process->get_active_address();
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

configs::Flow *
FlowAllocatorRoundRobinPs::newFlowRequest(IPCProcess * ipc_process,
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
	flow->remote_naming_info = event.remoteApplicationName;
	flow->local_naming_info = event.localApplicationName;
	flow->hop_count = 3;
	flow->max_create_flow_retries = 1;
	flow->source = true;
	flow->state = configs::Flow::ALLOCATION_IN_PROGRESS;

	rina::Connection * connection = new rina::Connection();
	connection->portId = event.portId;
	connection->sourceAddress = ipc_process->get_active_address();
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

class FlowAllocatorQTAPs: public IFlowAllocatorPs {
public:
	FlowAllocatorQTAPs(IFlowAllocator * dm_) :
		dm(dm_) {};
	configs::Flow *newFlowRequest(IPCProcess * ipc_process,
			const rina::FlowRequestEvent& event);
	int set_policy_set_param(const std::string& name,
			const std::string& value);
	virtual ~FlowAllocatorQTAPs() {}

private:
        // Data model of the security manager component.
        IFlowAllocator * dm;
};

// TODO: improve. Loss and delay CDF approximated by a single value only
// (maximum value)
configs::Flow *
FlowAllocatorQTAPs::newFlowRequest(IPCProcess * ipc_process,
				   const rina::FlowRequestEvent& event)
{
	configs::Flow* flow;
	rina::QoSCube * qos_cube = NULL;
	std::list<rina::Connection*> connections;
        std::list<rina::QoSCube*> qosCubes = dm->ipcp->resource_allocator_->getQoSCubes();
        std::list<rina::QoSCube*>::const_iterator iterator;
        const rina::FlowSpecification& flowSpec = event.flowSpecification;
        rina::Lockable lock;
        rina::ScopedLock g(lock);
        unsigned short qos_cube_delay = 0;
        unsigned short iterator_delay = 0;
        unsigned short flow_spec_delay = 0;

        if (*(qosCubes.begin())==NULL)
	    throw rina::Exception("No QoSCubes defined.");

        // A value of 0 means user does not care about delay
        if (flowSpec.delay == 0)
        	flow_spec_delay = USHRT_MAX;
        else
        	flow_spec_delay = flowSpec.delay;

        for (iterator = qosCubes.begin(); iterator != qosCubes.end(); ++iterator) {
        	// A value of 0 means the QoS cube does not guarantee max delay
        	if ((*iterator)->delay_ == 0) {
        		iterator_delay = USHRT_MAX;
        	} else {
        		iterator_delay = (*iterator)->delay_;
        	}

        	if (flow_spec_delay >= iterator_delay && flowSpec.loss >= (*iterator)->loss) {
        		if (qos_cube && qos_cube->delay_ == 0) {
        			qos_cube_delay = USHRT_MAX;
        		} else if (qos_cube) {
        			qos_cube_delay = qos_cube->delay_;
        		}

        		if (!qos_cube) {
        			qos_cube = *iterator;
        		} else if (qos_cube_delay < iterator_delay) {
        			qos_cube = *iterator;
        		} else if (qos_cube->loss < (*iterator)->loss) {
        			qos_cube = *iterator;
        		}
        	}
        }

	if (!qos_cube) {
		LOG_ERR("Could not find any QoS cube with max delay < %u ms and max loss < %u/10000",
				flowSpec.delay, flowSpec.loss);
		throw rina::Exception("No matching QoS found");
	}

	LOG_IPCP_INFO("Selected QoS cube with name %s", qos_cube->get_name().c_str());

	flow = dm->createFlow();
	flow->remote_naming_info = event.remoteApplicationName;
	flow->local_naming_info = event.localApplicationName;
	flow->hop_count = 3;
	flow->max_create_flow_retries = 1;
	flow->source = true;
	flow->state = configs::Flow::ALLOCATION_IN_PROGRESS;

	rina::Connection * connection = new rina::Connection();
	connection->portId = event.portId;
	connection->sourceAddress = ipc_process->get_address();
	connection->setQosId(qos_cube->id_);
	connection->setFlowUserIpcProcessId(event.flowRequestorIpcProcessId);
        rina::DTPConfig dtpConfig = rina::DTPConfig(qos_cube->dtp_config_);
	if (event.flowSpecification.maxAllowableGap < 0) {
		dtpConfig.set_max_sdu_gap(INT_MAX);
	} else {
		dtpConfig.set_max_sdu_gap(qos_cube->max_allowable_gap_);
	}
	connection->setDTPConfig(dtpConfig);
        rina::DTCPConfig dtcpConfig = rina::DTCPConfig(qos_cube->dtcp_config_);
	connection->setDTCPConfig(dtcpConfig);
	connections.push_back(connection);

	flow->connections = connections;
	flow->current_connection_index = 0;
	flow->flow_specification = flowSpec;

	return flow;
}

int FlowAllocatorQTAPs::set_policy_set_param(const std::string& name,
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

extern "C" rina::IPolicySet *
createFlowAllocatorQTAPs(rina::ApplicationEntity * ctx)
{
        IFlowAllocator * fa = dynamic_cast<IFlowAllocator *>(ctx);

        if (!fa) {
                return NULL;
        }

        return new FlowAllocatorQTAPs(fa);
}

extern "C" void
destroyFlowAllocatorQTAPs(rina::IPolicySet * ps)
{
        if (ps) {
                delete ps;
        }
}

}
