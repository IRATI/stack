/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * This library provides definitions common to all the other RINA
 * related libraries presented in this document. Common
 * functionalities shared among framework components (i.e. applications,
 * daemons and libraries) might be made available from this library
 * as well.
 */

#ifndef LIBRINA_COMMON_H
#define LIBRINA_COMMON_H

#ifdef __cplusplus

#include <string>
#include <vector>
#include <list>

#include "exceptions.h"
#include "patterns.h"

namespace rina {

/**
 * Returns the version number of librina
 */
std::string getVersion();

/**
 * A class that can be printed as a String
 */
class StringConvertable {
public:
	virtual std::string toString() = 0;
	virtual ~StringConvertable() {
	}
};

/**
 * Contains application naming information
 */
class ApplicationProcessNamingInformation: public StringConvertable {
	/**
	 * The process_name identifies an application process within the
	 * application process namespace. This value is required, it
	 * cannot be NULL. This name has global scope (it is defined by
	 * the chain of IDD databases that are linked together), and is
	 * assigned by an authority that manages the namespace that
	 * particular application name belongs to.
	 */
	std::string processName;

	/**
	 * The process_instance identifies a particular instance of the
	 * process. This value is optional, it may be NULL.
	 */
	std::string processInstance;

	/**
	 * The entity_name identifies an application entity within the
	 * application process. This value is optional, it may be NULL.
	 */
	std::string entityName;

	/**
	 * The entity_name identifies a particular instance of an entity
	 * within the application process. This value is optional, it
	 * may be NULL
	 */
	std::string entityInstance;

public:
	ApplicationProcessNamingInformation();
	ApplicationProcessNamingInformation(const std::string& processName,
			const std::string& processInstance);
	bool operator==(const ApplicationProcessNamingInformation &other) const;
	bool operator!=(const ApplicationProcessNamingInformation &other) const;
	bool operator>(const ApplicationProcessNamingInformation &other) const;
	bool operator<=(const ApplicationProcessNamingInformation &other) const;
	bool operator<(const ApplicationProcessNamingInformation &other) const;
	bool operator>=(const ApplicationProcessNamingInformation &other) const;
	const std::string& getEntityInstance() const;
	void setEntityInstance(const std::string& entityInstance);
	const std::string& getEntityName() const;
	void setEntityName(const std::string& entityName);
	const std::string& getProcessInstance() const;
	void setProcessInstance(const std::string& processInstance);
	const std::string& getProcessName() const;
	void setProcessName(const std::string& processName);
	std::string toString();
};

/**
 * This class defines the characteristics of a flow
 */
class FlowSpecification {
	/** Average bandwidth in bytes/s. A value of 0 means don't care. */
	unsigned int averageBandwidth;

	/** Average bandwidth in SDUs/s. A value of 0 means don't care */
	unsigned int averageSDUBandwidth;

	/** In milliseconds. A value of 0 means don't care*/
	unsigned int peakBandwidthDuration;

	/** In milliseconds. A value of 0 means don't care */
	unsigned int peakSDUBandwidthDuration;

	/** A value of 0 indicates 'do not care' */
	double undetectedBitErrorRate;

	/** Indicates if partial delivery of SDUs is allowed or not */
	bool partialDelivery;

	/** Indicates if SDUs have to be delivered in order */
	bool orderedDelivery;

	/**
	 * Indicates the maximum gap allowed among SDUs, a gap of N SDUs
	 * is considered the same as all SDUs delivered. A value of -1
	 * indicates 'Any'
	 */
	int maxAllowableGap;

	/**
	 * In milliseconds, indicates the maximum delay allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	unsigned int delay;

	/**
	 * In milliseconds, indicates the maximum jitter allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	unsigned int jitter;

	/**
	 * The maximum SDU size for the flow. May influence the choice
	 * of the DIF where the flow will be created.
	 */
	unsigned int maxSDUsize;

public:
	FlowSpecification();
	bool operator==(const FlowSpecification &other) const;
	bool operator!=(const FlowSpecification &other) const;
	unsigned int getAverageBandwidth() const;
	void setAverageBandwidth(unsigned int averageBandwidth);
	unsigned int getAverageSduBandwidth() const;
	void setAverageSduBandwidth(unsigned int averageSduBandwidth);
	unsigned int getDelay() const;
	void setDelay(unsigned int delay);
	unsigned int getJitter() const;
	void setJitter(unsigned int jitter);
	int getMaxAllowableGap() const;
	void setMaxAllowableGap(int maxAllowableGap);
	unsigned int getMaxSDUSize() const;
	void setMaxSDUSize(unsigned int maxSduSize);
	bool isOrderedDelivery() const;
	void setOrderedDelivery(bool orderedDelivery);
	bool isPartialDelivery() const;
	void setPartialDelivery(bool partialDelivery);
	unsigned int getPeakBandwidthDuration() const;
	void setPeakBandwidthDuration(unsigned int peakBandwidthDuration);
	unsigned int getPeakSduBandwidthDuration() const;
	void setPeakSduBandwidthDuration(unsigned int peakSduBandwidthDuration);
	double getUndetectedBitErrorRate() const;
	void setUndetectedBitErrorRate(double undetectedBitErrorRate);
};

/**
 * Defines the properties that a QoSCube is able to provide
 */
class QoSCube {

	/** The name of the QoS cube*/
	std::string name;

	/** The id of the QoS cube */
	int id;

	/** Average bandwidth in bytes/s. A value of 0 means don't care. */
	unsigned int averageBandwidth;

	/** Average bandwidth in SDUs/s. A value of 0 means don't care */
	unsigned int averageSDUBandwidth;

	/** In milliseconds. A value of 0 means don't care*/
	unsigned int peakBandwidthDuration;

	/** In milliseconds. A value of 0 means don't care */
	unsigned int peakSDUBandwidthDuration;

	/** A value of 0 indicates 'do not care' */
	double undetectedBitErrorRate;

	/** Indicates if partial delivery of SDUs is allowed or not */
	bool partialDelivery;

	/** Indicates if SDUs have to be delivered in order */
	bool orderedDelivery;

	/**
	 * Indicates the maximum gap allowed among SDUs, a gap of N SDUs
	 * is considered the same as all SDUs delivered. A value of -1
	 * indicates 'Any'
	 */
	int maxAllowableGap;

	/**
	 * In milliseconds, indicates the maximum delay allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	unsigned int delay;

	/**
	 * In milliseconds, indicates the maximum jitter allowed in this
	 * flow. A value of 0 indicates 'do not care'
	 */
	unsigned int jitter;
public:
	QoSCube();
	QoSCube(const std::string& name, int id);
	bool operator==(const QoSCube &other) const;
	bool operator!=(const QoSCube &other) const;
	int getId() const;
	const std::string& getName() const;
	unsigned int getAverageBandwidth() const;
	void setAverageBandwidth(unsigned int averageBandwidth);
	unsigned int getAverageSduBandwidth() const;
	void setAverageSduBandwidth(unsigned int averageSduBandwidth);
	unsigned int getDelay() const;
	void setDelay(unsigned int delay);
	unsigned int getJitter() const;
	void setJitter(unsigned int jitter);
	int getMaxAllowableGap() const;
	void setMaxAllowableGap(int maxAllowableGap);
	bool isOrderedDelivery() const;
	void setOrderedDelivery(bool orderedDelivery);
	bool isPartialDelivery() const;
	void setPartialDelivery(bool partialDelivery);
	unsigned int getPeakBandwidthDuration() const;
	void setPeakBandwidthDuration(unsigned int peakBandwidthDuration);
	unsigned int getPeakSduBandwidthDuration() const;
	void setPeakSduBandwidthDuration(unsigned int peakSduBandwidthDuration);
	double getUndetectedBitErrorRate() const;
	void setUndetectedBitErrorRate(double undetectedBitErrorRate);
};

/**
 * This class contains the properties of a single DIF
 */
class DIFProperties {
	/** The name of the DIF */
	ApplicationProcessNamingInformation DIFName;

	/**
	 * The maximum SDU size this DIF can handle (writes with bigger
	 * SDUs will return an error, and read will never return an SDUs
	 * bigger than this size
	 */
	unsigned int maxSDUSize;

	/**
	 * The different QoS cubes supported by the DIF
	 */
	std::list<QoSCube> qosCubes;
public:
	DIFProperties();
	DIFProperties(const ApplicationProcessNamingInformation& DIFName,
			int maxSDUSize);
	const ApplicationProcessNamingInformation& getDifName() const;
	unsigned int getMaxSduSize() const;
	const std::list<QoSCube>& getQoSCubes() const;
	void addQoSCube(const QoSCube& qosCube);
	void removeQoSCube(const QoSCube& qosCube);
};

/**
 * Enum type that identifies the different types of events
 */
enum IPCEventType {
	FLOW_ALLOCATION_REQUESTED_EVENT,
	FLOW_DEALLOCATION_REQUESTED_EVENT,
	APPLICATION_UNREGISTERED_EVENT,
	FLOW_DEALLOCATED_EVENT,
	FLOW_ALLOCATION_REQUEST_EVENT,
	APPLICATION_REGISTRATION_REQUEST_EVENT,
	APPLICATION_UNREGISTRATION_REQUEST_EVENT,
	ASSIGN_TO_DIF_REQUEST_EVENT
};

/**
 * Base class for IPC Events
 */
class IPCEvent {
	/** The type of event */
	IPCEventType eventType;

	/**
	 * If the event is a request, this is the number to relate it
	 * witht the response
	 */
	unsigned int sequenceNumber;

public:
	virtual ~IPCEvent(){}

	IPCEvent(IPCEventType eventType, unsigned int sequenceNumber) {
		this->eventType = eventType;
		this->sequenceNumber = sequenceNumber;
	}

	IPCEventType getType() const {
		return eventType;
	}

	unsigned int getSequenceNumber() const{
		return sequenceNumber;
	}
};

/**
 * Event informing about an incoming flow request from another application
 */
class FlowRequestEvent: public IPCEvent {
	/** The port-id that locally identifies the flow */
	int portId;

	/** The name of the DIF that is providing this flow */
	ApplicationProcessNamingInformation DIFName;

	/** The application that requested the flow */
	ApplicationProcessNamingInformation sourceApplicationName;

	/** The application targeted by the flow */
	ApplicationProcessNamingInformation destinationApplicationName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

public:
	FlowRequestEvent(const FlowSpecification& flowSpecification,
			const ApplicationProcessNamingInformation& sourceApplicationName,
			const ApplicationProcessNamingInformation& destApplicationName,
			unsigned int sequenceNumber);
	FlowRequestEvent(int portId,
			const FlowSpecification& flowSpecification,
			const ApplicationProcessNamingInformation& sourceApplicationName,
			const ApplicationProcessNamingInformation& destApplicationName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber);
	int getPortId() const;
	const FlowSpecification& getFlowSpecification() const;
	void setPortId(int portId);
	void setDIFName(const ApplicationProcessNamingInformation& difName);
	const ApplicationProcessNamingInformation& getDIFName() const;
	const ApplicationProcessNamingInformation& getSourceApplicationName() const;
	const ApplicationProcessNamingInformation& getDestApplicationName() const;
};

/**
 * Event informing that an application has requested the
 * registration to a DIF
 */
class ApplicationRegistrationRequestEvent: public IPCEvent {
	/** The application that wants to register */
	ApplicationProcessNamingInformation applicationName;

	/** The DIF to which the application wants to register */
	ApplicationProcessNamingInformation DIFName;

public:
	ApplicationRegistrationRequestEvent(
			const ApplicationProcessNamingInformation& appName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
};

/**
 * Event informing that an application has requested the
 * unregistration from a DIF
 */
class ApplicationUnregistrationRequestEvent: public IPCEvent {
	/** The application that wants to unregister */
	ApplicationProcessNamingInformation applicationName;

	/** The DIF to which the application wants to cancel the registration */
	ApplicationProcessNamingInformation DIFName;

public:
	ApplicationUnregistrationRequestEvent(
			const ApplicationProcessNamingInformation& appName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
};

/**
 * Stores IPC Events that have happened, ready to be consumed and
 * processed by client classes.
 */
class IPCEventProducer {
public:
	/** Retrieves the next available event, if any */
	IPCEvent * eventPoll();

	/** Blocks until there is an event available */
	IPCEvent * eventWait();
};

/**
 * Make IPCManager singleton
 */
extern Singleton<IPCEventProducer> ipcEventProducer;

/**
 * Base class for all RINA exceptions
 */
class IPCException: public Exception {
public:
	IPCException(const std::string& description);
	static const std::string operation_not_implemented_error;
};

/**
 * Represents a policy. This is a generic placeholder which should be defined
 * during the second prototype activities
 */
class Policy {

};

/**
 * Enum type that identifies the different types of DIFs
 */
enum DIFType {
	DIF_TYPE_NORMAL, DIF_TYPE_SHIM_IP, DIF_TYPE_SHIM_ETHERNET
};

/**
 * Contains the data about a DIF Configuration
 */
class DIFConfiguration {

	/** The type of DIF */
	DIFType difType;

	/** The name of the DIF */
	ApplicationProcessNamingInformation difName;

	/** The QoS cubes supported by the DIF */
	std::vector<QoSCube> qosCubes;

	/** The policies of the DIF */
	std::vector<Policy> policies;

public:
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	DIFType getDifType() const;
	void setDifType(DIFType difType);
	const std::vector<Policy>& getPolicies();
	void setPolicies(const std::vector<Policy>& policies);
	const std::vector<QoSCube>& getQosCubes() const;
	void setQosCubes(const std::vector<QoSCube>& qosCubes);
};

/**
 * Encapsulates a flow request
 */
class FlowRequest {
	/** The port-id that locally identifies the flow */
	int portId;

	/** The application that requested the flow */
	ApplicationProcessNamingInformation sourceApplicationName;

	/** The application targeted by the flow */
	ApplicationProcessNamingInformation destinationApplicationName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

public:
	const ApplicationProcessNamingInformation&
			getDestinationApplicationName() const;
	void setDestinationApplicationName(
			const ApplicationProcessNamingInformation&
				destinationApplicationName);
	const FlowSpecification& getFlowSpecification() const;
	void setFlowSpecification(const FlowSpecification& flowSpecification);
	int getPortId() const;
	void setPortId(int portId);
	const ApplicationProcessNamingInformation& getSourceApplicationName() const;
	void setSourceApplicationName(
			const ApplicationProcessNamingInformation& sourceApplicationName);
};

}
#endif

#endif
