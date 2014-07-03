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

#include "librina/common.h"

namespace rina {

/// Represents an IPC Process with whom we're enrolled
class Neighbor {

public:
        Neighbor();
        bool operator==(const Neighbor &other) const;
        bool operator!=(const Neighbor &other) const;
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
        long get_last_heard_from_time_in_ms() const;
        void set_last_heard_from_time_in_ms(long last_heard_from_time_in_ms_);
        int get_underlying_port_id() const;
        void set_underlying_port_id(int underlying_port_id);
        unsigned int get_number_of_enrollment_attempts() const;
        void set_number_of_enrollment_attempts(
                        unsigned int number_of_enrollment_attempts);
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
        long last_heard_from_time_in_ms_;

        /// The number of times we have tried to re-enroll with the
        /// neighbor after the connectivity has been lost
        unsigned int number_of_enrollment_attempts_;
};

/**
 * Represents the value of an object stored in the RIB
 */
class RIBObjectValue{
	//TODO
};

/// Contains the data of an object in the RIB
class RIBObjectData{
public:
	RIBObjectData();
	RIBObjectData(std::string clazz, std::string name,
			long long instance, RIBObjectValue value);
	bool operator==(const RIBObjectData &other) const;
	bool operator!=(const RIBObjectData &other) const;
	const std::string& get_class() const;
	void set_class(const std::string& clazz);
	unsigned long get_instance() const;
	void set_instance(unsigned long  instance);
	const std::string& get_name() const;
	void set_name(const std::string& name);
	RIBObjectValue get_value() const;
	void set_value(RIBObjectValue value);
	const std::string& get_displayable_value() const;
	void set_displayable_value(const std::string& displayable_value);

	/** The class (type) of object */
	std::string class_;

	/** The name of the object (unique within a class)*/
	std::string name_;

	/** A synonim for clazz+name (unique within the RIB) */
	unsigned long instance_;

	/** The value of the object */
	RIBObjectValue value_;

	/**
	 * The value of the object, encoded in an string for
	 * displayable purposes
	 */
	std::string displayable_value_;
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

}

#endif

#endif
