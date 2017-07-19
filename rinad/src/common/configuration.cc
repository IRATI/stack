/*
 * Common MAD-IPCP objects
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include "configuration.h"
namespace rinad {
namespace configs {
//	CLASS Flow
Flow::Flow()
{
        local_port_id = 0;
        remote_port_id = 0;
        local_address = 0;
        remote_address = 0;
        current_connection_index = 0;
        max_create_flow_retries = 0;
        create_flow_retries = 0;
        hop_count = 0;
        source = false;
        state = EMPTY;
        access_control = 0;
        internal = false;
}

Flow::Flow(const Flow& flow)
{
        local_naming_info = flow.local_naming_info;
        remote_naming_info = flow.remote_naming_info;
        flow_specification = flow.flow_specification;
        local_port_id = flow.local_port_id;
        remote_port_id = flow.remote_port_id;
        local_address = flow.local_address;
        remote_address = flow.remote_address;
        current_connection_index = flow.current_connection_index;
        max_create_flow_retries = flow.max_create_flow_retries;
        create_flow_retries = flow.create_flow_retries;
        hop_count = flow.hop_count;
        source = flow.source;
        state = flow.state;
        access_control = 0;
        internal = flow.internal;

        std::list<rina::Connection*>::const_iterator it;
        rina::Connection * current = 0;
        for (it = flow.connections.begin(); it != flow.connections.end(); ++it)
        {
                current = *it;
                connections.push_back(new rina::Connection(*current));
        }
}

Flow::~Flow()
{
        std::list<rina::Connection*>::iterator iterator;
        for (iterator = connections.begin(); iterator != connections.end();
                        ++iterator)
        {
                if (*iterator)
                {
                        delete *iterator;
                        *iterator = 0;
                }
        }
        connections.clear();
}

rina::Connection* Flow::getActiveConnection() const
{
        rina::Connection result;
        std::list<rina::Connection*>::const_iterator iterator;

        unsigned int i = 0;
        for (iterator = connections.begin(); iterator != connections.end();
                        ++iterator)
        {
                if (i == current_connection_index)
                {
                        return *iterator;
                } else
                {
                        i++;
                }
        }

        throw rina::Exception("No active connection is currently defined");
}

std::string Flow::toString()
{
        std::stringstream ss;
        ss << "* State: " << state << std::endl;
        ss << "* Is this IPC Process the requestor of the flow? " << source
           << std::endl;
        ss << "* Max create flow retries: " << max_create_flow_retries
           << std::endl;
        ss << "* Hop count: " << hop_count << std::endl;
        ss << "* Local AP Naming Info: " << local_naming_info.toString()
           << std::endl;
        ;
        ss << "* Local address: " << local_address << std::endl;
        ss << "* Local port id: " << local_port_id << std::endl;
        ss << "* Remote AP Naming Info: "
           << remote_naming_info.toString() << std::endl;
        ss << "* Remote address: " << remote_address << std::endl;
        ss << "* Remote port id: " << remote_port_id << std::endl;
        if (connections.size() > 0)
        {
                ss << "* Connection ids of the connection supporting this flow: +\n";
                for (std::list<rina::Connection*>::const_iterator iterator =
                                connections.begin(), end = connections.end();
                                iterator != end; ++iterator)
                {
                        ss << "Src CEP-id " << (*iterator)->getSourceCepId()
                           << "; Dest CEP-id " << (*iterator)->getDestCepId()
                           << "; Qos-id " << (*iterator)->getQosId()
                           << std::endl;
                }
        }
        ss << "* Index of the current active connection for this flow: "
           << current_connection_index << std::endl;
        return ss.str();
}

const std::string Flow::getKey() const
{
        std::stringstream ss;
        ss << local_address << "-" << local_port_id;
        return ss.str();
}

rina::FlowInformation Flow::to_flow_information(const std::string dif_name,
					        int port_id)
{
	rina::FlowInformation flow_info;

	flow_info.difName.processName = dif_name;
	flow_info.flowSpecification = flow_specification;
	flow_info.localAppName = local_naming_info;
	flow_info.remoteAppName = remote_naming_info;
	flow_info.portId = port_id;

	return flow_info;
}

}  //namespace configs
}  //namespace rinad
