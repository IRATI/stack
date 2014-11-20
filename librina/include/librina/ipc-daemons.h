/*
 * Common librina classes for IPC Process and IPC Manager daemons
 *
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

#ifndef LIBRINA_IPC_DAEMONS_H
#define LIBRINA_IPC_DAEMONS_H

#ifdef __cplusplus

#include "librina/rib.h"

namespace rina {

/// Represents an IPC Process with whom we're enrolled
class Neighbor {

public:
        Neighbor();
        bool operator==(const Neighbor &other) const;
        bool operator!=(const Neighbor &other) const;
#ifndef SWIG
        const ApplicationProcessNamingInformation& get_name() const;
        void set_name(const ApplicationProcessNamingInformation& name);
        const ApplicationProcessNamingInformation&
                get_supporting_dif_name() const;
        void set_supporting_dif_name(
                const ApplicationProcessNamingInformation& supporting_dif_name);
        const std::list<ApplicationProcessNamingInformation>& get_supporting_difs();
        void set_supporting_difs(
                        const std::list<ApplicationProcessNamingInformation>& supporting_difs);
        void add_supporting_dif(const ApplicationProcessNamingInformation& supporting_dif);
        unsigned int get_address() const;
        void set_address(unsigned int address);
        unsigned int get_average_rtt_in_ms() const;
        void set_average_rtt_in_ms(unsigned int average_rtt_in_ms);
        bool is_enrolled() const;
        void set_enrolled(bool enrolled);
        int get_last_heard_from_time_in_ms() const;
        void set_last_heard_from_time_in_ms(int last_heard_from_time_in_ms_);
        int get_underlying_port_id() const;
        void set_underlying_port_id(int underlying_port_id);
        unsigned int get_number_of_enrollment_attempts() const;
        void set_number_of_enrollment_attempts(
                        unsigned int number_of_enrollment_attempts);
#endif
        const std::string toString();

        /// The IPC Process name of the neighbor
        ApplicationProcessNamingInformation name_;

        /// The name of the supporting DIF used to exchange data
        ApplicationProcessNamingInformation supporting_dif_name_;

        /// The names of all the supporting DIFs of this neighbor
        std::list<ApplicationProcessNamingInformation> supporting_difs_;

        /// The address
        unsigned int address_;

        /// Tells if it is enrolled or not
        bool enrolled_;

        /// The average RTT in ms
        unsigned int average_rtt_in_ms_;

        /// The underlying portId used to communicate with this neighbor
        int underlying_port_id_;

        /// The last time a KeepAlive message was received from
        /// that neighbor, in ms
        int last_heard_from_time_in_ms_;

        /// The number of times we have tried to re-enroll with the
        /// neighbor after the connectivity has been lost
        unsigned int number_of_enrollment_attempts_;
};

/**
 * Thrown when there are problems assigning an IPC Process to a DIF
 */
class AssignToDIFException: public IPCException {
public:
        AssignToDIFException():
                IPCException("Problems assigning IPC Process to DIF"){
        }
        AssignToDIFException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems in the configuration
 */
class BadConfigurationException: public IPCException {
public:
	BadConfigurationException():
                IPCException("Problems assigning IPC Process to DIF"){
        }
	BadConfigurationException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems updating a DIF configuration
 */
class UpdateDIFConfigurationException: public IPCException {
public:
        UpdateDIFConfigurationException():
                IPCException("Problems updating DIF configuration"){
        }
        UpdateDIFConfigurationException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems instructing an IPC Process to enroll to a DIF
 */
class EnrollException: public IPCException {
public:
        EnrollException():
                IPCException("Problems causing an IPC Process to enroll to a DIF"){
        }
        EnrollException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems while modifying a policy-set-related
 * parameter of an IPC Process
 */
class SetPolicySetParamException: public IPCException {
public:
	SetPolicySetParamException():
		IPCException("Problems while modifying a policy-set related "
                                "parameter of an IPC Process"){
	}
	SetPolicySetParamException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems while selectin a policy-set
 * in an IPC Process component
 */
class SelectPolicySetException: public IPCException {
public:
	SelectPolicySetException():
		IPCException("Problems while selecting a policy-set "
                                "in an IPC Process component"){
	}
	SelectPolicySetException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems while trying to load or unload
 * plugins
 */
class PluginLoadException: public IPCException {
public:
	PluginLoadException():
		IPCException("Problems while loading or unloading "
                                "a plugin for an IPC Process") {
	}
	PluginLoadException(const std::string& description):
		IPCException(description) {
	}
};

/**
 * Event informing about the result of an assign to DIF operation
 */
class AssignToDIFResponseEvent: public BaseResponseEvent {
public:
        AssignToDIFResponseEvent(
                        int result, unsigned int sequenceNumber);
};

/**
 * An IPC process reports the result of the access of a policy-set-related
 * parameter
 */
class SetPolicySetParamResponseEvent: public IPCEvent {
public:
        int result;

	SetPolicySetParamResponseEvent(int result,
                                       unsigned int sequenceNumber);
};

/**
 * An IPC process reports the result of the selection of a policy-set
 */
class SelectPolicySetResponseEvent: public IPCEvent {
public:
        int result;

	SelectPolicySetResponseEvent(int result,
                                     unsigned int sequenceNumber);
};

/**
 * An IPC process reports the result of the loading or
 * unloading of a plugin
 */
class PluginLoadResponseEvent: public IPCEvent {
public:
        int result;

	PluginLoadResponseEvent(int result,
                                unsigned int sequenceNumber);
};

}

#endif

#endif
