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

#ifndef LIBRINAPP_HPP
#define LIBRINAPP_HPP

//#include "librina-common.h"
//#include "librina-application.h"
//#include "librina-ipc-manager.h"
//#include "librina-ipc-process.h"
//#include "librina-faux-sockets.h"
//#include "librina-cdap.h"
//#include "librina-sdu-protection.h"

#include <exception>
#include <string>
using namespace std;

/**
 * Contains an application process naming information
 */
class ApplicationProcessNamingInformation {
	/**
	 * The process_name identifies an application process within the
	 * application process namespace. This value is required, it
	 * cannot be NULL. This name has global scope (it is defined by
	 * the chain of IDD databases that are linked together), and is
	 * assigned by an authority that manages the namespace that
	 * particular application name belongs to.
	 */
	string processName;

	/**
	 * The process_instance identifies a particular instance of the
	 * process. This value is optional, it may be NULL.
	 */
	string processInstance;

	/**
	 * The entity_name identifies an application entity within the
	 * application process. This value is optional, it may be NULL.
	 */
	string entityName;

	/**
	 * The entity_name identifies a particular instance of an entity
	 * within the application process. This value is optional, it
	 * may be NULL
	 */
	string entityInstance;

public:
	ApplicationProcessNamingInformation();
	ApplicationProcessNamingInformation(const string& processName,
				const string& processInstance);
	const string& getEntityInstance() const;
	void setEntityInstance(const string& entityInstance);
	const string& getEntityName() const;
	void setEntityName(const string& entityName);
	const string& getProcessInstance() const;
	void setProcessInstance(const string& processInstance);
	const string& getProcessName() const;
	void setProcessName(const string& processName);
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
 * This class encapsulates the services provided by a Flow.
 */
class Flow {
	ApplicationProcessNamingInformation sourceApplication;
	ApplicationProcessNamingInformation destinationApplication;
	FlowSpecification flowSpecification;
	int portId;
public:
	Flow(const ApplicationProcessNamingInformation& sourceApplication,
			const ApplicationProcessNamingInformation& destinationApplication,
			const FlowSpecification& flowSpecification);
	void allocate() throw (exception);
	void deallocate() throw (exception);
	int getPortId() const;
};

#endif
