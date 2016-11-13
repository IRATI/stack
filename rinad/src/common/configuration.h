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

#ifndef COMMON_CONFIGURATION_H
#define COMMON_CONFIGURATION_H

#include <librina/common.h>
#include <librina/ipc-process.h>
#include <librina/configuration.h>

namespace rinad {
namespace configs {
static const std::string NEIGH_CONT_NAME = "/difManagement/enrollment/neighbors";
/// The object that contains all the information
/// that is required to initiate an enrollment
/// request (send as the objectvalue of a CDAP M_START
/// message, as specified by the Enrollment spec)
class EnrollmentInformationRequest {
 public:
        EnrollmentInformationRequest()
                        : address_(0),
                          allowed_to_start_early_(false)
        {
        }
        ;

        /// The address of the IPC Process that requests
        ///to join a DIF
        unsigned int address_;
        std::list<rina::ApplicationProcessNamingInformation> supporting_difs_;
        bool allowed_to_start_early_;
};

/// Encapsulates all the information required to manage a Flow
class Flow {
 public:
        enum IPCPFlowState {
                EMPTY,
                ALLOCATION_IN_PROGRESS,
                ALLOCATED,
                WAITING_2_MPL_BEFORE_TEARING_DOWN,
                DEALLOCATED
        };

        Flow();
        Flow(const Flow& flow);
        ~Flow();
        rina::Connection* getActiveConnection() const;
        std::string toString();
        const std::string getKey() const;

        /// The application that requested the flow
        rina::ApplicationProcessNamingInformation source_naming_info;

        /// The destination application of the flow
        rina::ApplicationProcessNamingInformation destination_naming_info;

        /// The port-id returned to the Application process that requested the flow. This port-id is used for
        /// the life of the flow.
        unsigned int source_port_id;

        /// The port-id returned to the destination Application process. This port-id is used for
        // the life of the flow
        unsigned int destination_port_id;

        /// The address of the IPC process that is the source of this flow
        unsigned int source_address;

        /// The address of the IPC process that is the destination of this flow
        unsigned int destination_address;

        /// All the possible connections of this flow
        std::list<rina::Connection*> connections;

        /// The index of the connection that is currently Active in this flow
        unsigned int current_connection_index;

        /// The status of this flow
        IPCPFlowState state;

        /// The list of parameters from the AllocateRequest that generated this flow
        rina::FlowSpecification flow_specification;

        /// TODO this is just a placeHolder for this piece of data
        char* access_control;

        /// Maximum number of retries to create the flow before giving up.
        unsigned int max_create_flow_retries;

        /// The current number of retries
        unsigned int create_flow_retries;

        /// While the search rules that generate the forwarding table should allow for a
        /// natural termination condition, it seems wise to have the means to enforce termination.
        unsigned int hop_count;

        ///True if this IPC process is the source of the flow, false otherwise
        bool source;
};

typedef struct {
        rina::ApplicationProcessNamingInformation neighbor_name;
        rina::ApplicationProcessNamingInformation under_dif;
        rina::ApplicationProcessNamingInformation dif;
} neighbor_config_t;

/// IPCP configuration struct
typedef struct {
        rina::ApplicationProcessNamingInformation name;
        rina::DIFInformation dif_to_assign;
        std::list<rina::ApplicationProcessNamingInformation> difs_to_register;
        std::list<neighbor_config_t> neighbors;
        void addRegister(rina::ApplicationProcessNamingInformation &dif)
        {
                difs_to_register.push_back(dif);
        }
        void addNeighbor(neighbor_config_t &dif)
        {
                neighbors.push_back(dif);
        }
} ipcp_config_t;

/// IPCP (read) struct
typedef struct {
        rina::ApplicationProcessNamingInformation process;
} ipcp_t;

}  //namespace configs
}  //namespace rinad
#endif
