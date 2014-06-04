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

static std::string NORMAL_IPC_PROCESS= "normal-ipc";

/**
 * Returns the version number of librina
 */
std::string getVersion();

/**
 * Contains application naming information
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
	ApplicationProcessNamingInformation & operator=(
			const ApplicationProcessNamingInformation & other);
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
	std::string getProcessNamePlusInstance();
	std::string getEncodedString();
	const std::string toString();
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
	const std::string toString();
};

/**
 * Contains the information of an allocated flow
 */
class FlowInformation {

	/** The local application name */
	ApplicationProcessNamingInformation localAppName;

	/** The remote application name */
	ApplicationProcessNamingInformation remoteAppName;

	/** The flow characteristics */
	FlowSpecification flowSpecification;

	/** The portId of the flow */
	int portId;

	/** The name of the DIF where the flow has been allocated */
	ApplicationProcessNamingInformation difName;

public:
	bool operator==(const FlowInformation &other) const;
	bool operator!=(const FlowInformation &other) const;
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	const FlowSpecification& getFlowSpecification() const;
	void setFlowSpecification(const FlowSpecification& flowSpecification);
	const ApplicationProcessNamingInformation& getLocalAppName() const;
	void setLocalAppName(
			const ApplicationProcessNamingInformation& localAppName);
	int getPortId() const;
	void setPortId(int portId);
	const ApplicationProcessNamingInformation& getRemoteAppName() const;
	void setRemoteAppName(
			const ApplicationProcessNamingInformation& remoteAppName);
	const std::string toString();
};

/**
 * A parameter of the policy
 */
class PolicyParameter {
        /** the name of the parameter */
        std::string name;

        /** the value of the parameter */
        std::string value;

public:
        PolicyParameter();
        PolicyParameter(const std::string& name, const std::string& value);
        bool operator==(const PolicyParameter &other) const;
        bool operator!=(const PolicyParameter &other) const;
        const std::string& getName() const;
        void setName(const std::string& name);
        const std::string& getValue() const;
        void setValue(const std::string& value);
};

/**
 * Configuration of a policy (name/version/parameters)
 */
class PolicyConfig {

        /** the name of policy */
        std::string name;

        /** the version of the policy */
        std::string version;

        /** optional name/value parameters to configure the policy */
        std::list<PolicyParameter> parameters;

public:
        PolicyConfig();
        PolicyConfig(const std::string& name, const std::string& version);
        bool operator==(const PolicyConfig &other) const;
        bool operator!=(const PolicyConfig &other) const;
        const std::string& getName() const;
        void setName(const std::string& name);
        //const std::list<PolicyParameter>& getParameters() const;
        //void setParameters(const std::list<PolicyParameter>& parameters);
        //void addParameter(const PolicyParameter& paremeter);
        const std::string& getVersion() const;
        void setVersion(const std::string& version);
};

/**
 * The DTCP window based flow control configuration
 */
class DTCPWindowBasedFlowControlConfig {

        /**
         * Integer that the number PDUs that can be put on the
         * ClosedWindowQueue before something must be done.
         */
        int maxclosedwindowqueuelength;

        /** initial sequence number to get right window edge. */
        int initialcredit;

        /**
         * Invoked when a Transfer PDU is received to give the receiving PM an
         * opportunity to update the flow control allocations.
         */
        PolicyConfig rcvrflowcontrolpolicy;

        /**
         * This policy is used when there are conditions that warrant sending
         * fewer PDUs than allowed by the sliding window flow control, e.g.
         * the ECN bit is set.
         */
        PolicyConfig txControlPolicy;

public:
        DTCPWindowBasedFlowControlConfig();
        int getInitialcredit() const;
        void setInitialcredit(int initialcredit);
        int getMaxclosedwindowqueuelength() const;
        void setMaxclosedwindowqueuelength(int maxclosedwindowqueuelength);
        const PolicyConfig& getRcvrflowcontrolpolicy() const;
        void setRcvrflowcontrolpolicy(
                        const PolicyConfig& rcvrflowcontrolpolicy);
        const PolicyConfig& getTxControlPolicy() const;
        void setTxControlPolicy(const PolicyConfig& txControlPolicy);
        const std::string toString();
};

/**
 * The DTCP rate-basd flow control configuration
 */
class DTCPRateBasedFlowControlConfig {

        /**
         * the number of PDUs that may be sent in a TimePeriod. Used with
         * rate-based flow control.
         */
        int sendingrate;

        /**
         * length of time in microseconds for pacing rate-based flow control.
         */
        int timeperiod;

        /** used to momentarily lower the send rate below the rate allowed */
        PolicyConfig norateslowdownpolicy;

        /**
         * Allows rate-based flow control to exceed its nominal rate.
         * Presumably this would be for short periods and policies should
         * enforce this.  Like all policies, if this returns True it creates
         * the default action which is no override.
         */
        PolicyConfig nooverridedefaultpeakpolicy;

        /**
         * Allows an alternate action when using rate-based flow control and
         * the number of free buffers is getting low.
         */
        PolicyConfig ratereductionpolicy;

public:
        DTCPRateBasedFlowControlConfig();
        const PolicyConfig& getNooverridedefaultpeakpolicy() const;
        void setNooverridedefaultpeakpolicy(
                        const PolicyConfig& nooverridedefaultpeakpolicy);
        const PolicyConfig& getNorateslowdownpolicy() const;
        void setNorateslowdownpolicy(
                        const PolicyConfig& norateslowdownpolicy);
        const PolicyConfig& getRatereductionpolicy() const;
        void setRatereductionpolicy(const PolicyConfig& ratereductionpolicy);
        int getSendingrate() const;
        void setSendingrate(int sendingrate);
        int getTimeperiod() const;
        void setTimeperiod(int timeperiod);
        const std::string toString();
};

/**
 * The flow control configuration of a DTCP instance
 */
class DTCPFlowControlConfig {

        /** indicates whether window-based flow control is in use */
        bool windowbased;

        /** the window-based flow control configuration */
        DTCPWindowBasedFlowControlConfig windowbasedconfig;

        /** indicates whether rate-based flow control is in use */
        bool ratebased;

        /** the rate-based flow control configuration */
        DTCPRateBasedFlowControlConfig ratebasedconfig;

        /**
         * The number of free bytes below which flow control should slow or
         * block the user from doing any more Writes.
         */
        int sentbytesthreshold;

        /**
         * The percent of free bytes below, which flow control should slow or
         * block the user from doing any more Writes.
         */
        int sentbytespercentthreshold;

        /**
         * The number of free buffers below which flow control should slow or
         * block the user from doing any more Writes.
         */
        int sentbuffersthreshold;

        /**
         * The number of free bytes below which flow control does not move or
         * decreases the amount the Right Window Edge is moved.
         */
        int rcvbytesthreshold;

        /**
         * The number of free buffers at which flow control does not advance
         * or decreases the amount the Right Window Edge is moved.
         */
        int rcvbytespercentthreshold;

        /**
         * The percent of free buffers below which flow control should not
         * advance or decreases the amount the Right Window Edge is moved.
         */
        int rcvbuffersthreshold;

        /**
         * Used with flow control to determine the action to be taken when the
         * receiver has not extended more credit to allow the sender to send more
         * PDUs. Typically, the action will be to queue the PDUs until credit is
         * extended. This action is taken by DTCP, not DTP.
         */
        PolicyConfig closedwindowpolicy;

        /**
         * Determines what action to take if the receiver receives PDUs but the
         * credit or rate has been exceeded
         */
        PolicyConfig flowcontroloverrunpolicy;

        /**
         * Invoked when both Credit and Rate based flow control are in use and
         * they disagree on whether the PM can send or receive data. If it
         * returns True, then the PM can send or receive; if False, it cannot.
         */
        PolicyConfig reconcileflowcontrolpolicy;

        /**
         * Allows some discretion in when to send a Flow Control PDU when there
         * is no Retransmission Control.
         */
        PolicyConfig receivingflowcontrolpolicy;

public:
        DTCPFlowControlConfig();
        const PolicyConfig& getClosedwindowpolicy() const;
        void setClosedwindowpolicy(const PolicyConfig& closedwindowpolicy);
        const PolicyConfig& getFlowcontroloverrunpolicy() const;
        void setFlowcontroloverrunpolicy(
                        const PolicyConfig& flowcontroloverrunpolicy);
        bool isRatebased() const;
        void setRatebased(bool ratebased);
        const DTCPRateBasedFlowControlConfig& getRatebasedconfig() const;
        void setRatebasedconfig(
                        const DTCPRateBasedFlowControlConfig& ratebasedconfig);
        int getRcvbuffersthreshold() const;
        void setRcvbuffersthreshold(int rcvbuffersthreshold);
        int getRcvbytespercentthreshold() const;
        void setRcvbytespercentthreshold(int rcvbytespercentthreshold);
        int getRcvbytesthreshold() const;
        void setRcvbytesthreshold(int rcvbytesthreshold);
        const PolicyConfig& getReconcileflowcontrolpolicy() const;
        void setReconcileflowcontrolpolicy(
                        const PolicyConfig& reconcileflowcontrolpolicy);
        int getSentbuffersthreshold() const;
        void setSentbuffersthreshold(int sentbuffersthreshold);
        int getSentbytespercentthreshold() const;
        void setSentbytespercentthreshold(int sentbytespercentthreshold);
        int getSentbytesthreshold() const;
        void setSentbytesthreshold(int sentbytesthreshold);
        bool isWindowbased() const;
        void setWindowbased(bool windowbased);
        const DTCPWindowBasedFlowControlConfig& getWindowbasedconfig() const;
        void setWindowbasedconfig(
                        const DTCPWindowBasedFlowControlConfig&
                        windowbasedconfig);
        const PolicyConfig& getReceivingflowcontrolpolicy() const;
        void setReceivingflowcontrolpolicy(
                        const PolicyConfig& receivingflowcontrolpolicy);
        const std::string toString();
};

/**
 * The configuration of the retransmission control functions of a
 * DTCP instance
 */
class DTCPRtxControlConfig{

        /**
         * the number of times the retransmission of a PDU will be attempted
         * before some other action must be taken.
         */
        int datarxmsnmax;

        /**
         * Executed by the sender when a Retransmission Timer Expires. If this
         * policy returns True, then all PDUs with sequence number less than
         * or equal to the sequence number of the PDU associated with this
         * timeout are retransmitted; otherwise the procedure must determine
         * what action to take. This policy must be executed in less than the
         * maximum time to Ack
         */
        PolicyConfig rtxtimerexpirypolicy;

        /**
         * Executed by the sender and provides the Sender with some discretion
         * on when PDUs may be deleted from the ReTransmissionQ. This is useful
         * for multicast and similar situations where one might want to delay
         * discarding PDUs from the retransmission queue.
         */
        PolicyConfig senderackpolicy;

        /**
         *  Executed by the Sender and provides the Sender with some discretion
         *  on when PDUs may be deleted from the ReTransmissionQ. This policy
         *  is used in conjunction with the selective acknowledgement aspects
         *  of the mechanism and may be useful for multicast and similar
         *  situations where there may be a requirement to delay discarding PDUs
         *  from the retransmission queue
         */
        PolicyConfig recvingacklistpolicy;

        /**
         * Executed by the receiver of the PDU and provides some discretion in
         * the action taken.  The default action is to either Ack immediately
         * or to start the A-Timer and Ack the LeftWindowEdge when it expires.
         */
        PolicyConfig rcvrackpolicy;

        /**
         * This policy allows an alternate action when the A-Timer expires when
         * DTCP is present.
         */
        PolicyConfig sendingackpolicy;

        /** Allows an alternate action when a Control Ack PDU is received. */
        PolicyConfig rcvrcontrolackpolicy;

public:
        DTCPRtxControlConfig();
        int getDatarxmsnmax() const;
        void setDatarxmsnmax(int datarxmsnmax);
        const PolicyConfig& getRcvrackpolicy() const;
        void setRcvrackpolicy(const PolicyConfig& rcvrackpolicy);
        const PolicyConfig& getRcvrcontrolackpolicy() const;
        void setRcvrcontrolackpolicy(
                        const PolicyConfig& rcvrcontrolackpolicy);
        const PolicyConfig& getRecvingacklistpolicy() const;
        void setRecvingacklistpolicy(
                        const PolicyConfig& recvingacklistpolicy);
        void setRtxtimerexpirypolicy(
                        const PolicyConfig& rtxtimerexpirypolicy);
        const PolicyConfig& getRtxtimerexpirypolicy() const;
        const PolicyConfig& getSenderackpolicy() const;
        void setSenderackpolicy(const PolicyConfig& senderackpolicy);
        const PolicyConfig& getSendingackpolicy() const;
        void setSendingackpolicy(const PolicyConfig& sendingackpolicy);
        const std::string toString();
};

/**
 * Configuration of the DTCP instance, including policies and its parameters
 */
class DTCPConfig {

        /** True if flow control is required */
        bool flowcontrol;

        /** the flow control configuration of a DTCP instance */
        DTCPFlowControlConfig flowcontrolconfig;

        /** True if rtx control is required */
        bool rtxcontrol;

        /** the rtx control configuration of a DTCP instance */
        DTCPRtxControlConfig rtxcontrolconfig;

        /**
         * should be approximately 2Δt. This must be bounded. A DIF
         * specification may want to specify a maximum value.
         */
        int initialsenderinactivitytime;

        /**
         * should be approximately 3Δt. This must be bounded. A DIF
         * specification may want to specify a maximum value.
         */
        int initialrecvrinactivitytime;

        /**
         * used when DTCP is in use. If no PDUs arrive in this time period,
         * the receiver should expect a DRF in the next Transfer PDU. If not,
         * something is very wrong. The timeout value should generally be set
         * to 3(MPL+R+A).
         */
        PolicyConfig rcvrtimerinactivitypolicy;

        /**
         * used when DTCP is in use. This timer is used to detect long periods
         * of no traffic, indicating that a DRF should be sent. If not,
         * something is very wrong. The timeout value should generally be set
         * to 2(MPL+R+A).
         */
        PolicyConfig sendertimerinactiviypolicy;

        /**
         * This policy determines what action to take when the PM detects that
         * a control PDU (Ack or Flow Control) may have been lost.  If this
         * procedure returns True, then the PM will send a Control Ack and an
         * empty Transfer PDU.  If it returns False, then any action is determined
         * by the policy
         */
        PolicyConfig lostcontrolpdupolicy;

        /**
         * Executed by the sender to estimate the duration of the retx timer.
         * This policy will be based on an estimate of round-trip time and the
         * Ack or Ack List policy in use
         */
        PolicyConfig rttestimatorpolicy;

public:
        DTCPConfig();
        bool isFlowcontrol() const;
        void setFlowcontrol(bool flowcontrol);
        const DTCPFlowControlConfig& getFlowcontrolconfig() const;
        void setFlowcontrolconfig(
                        const DTCPFlowControlConfig& flowcontrolconfig);
        int getInitialrecvrinactivitytime() const;
        void setInitialrecvrinactivitytime(int initialrecvrinactivitytime);
        int getInitialsenderinactivitytime() const;
        void setInitialsenderinactivitytime(int initialsenderinactivitytime);
        const PolicyConfig& getLostcontrolpdupolicy() const;
        void setLostcontrolpdupolicy(
                        const PolicyConfig& lostcontrolpdupolicy);
        const PolicyConfig& getRcvrtimerinactivitypolicy() const;
        void setRcvrtimerinactivitypolicy(
                        const PolicyConfig& rcvrtimerinactivitypolicy);
        bool isRtxcontrol() const;
        void setRtxcontrol(bool rtxcontrol);
        const DTCPRtxControlConfig& getRtxcontrolconfig() const;
        void setRtxcontrolconfig(const DTCPRtxControlConfig& rtxcontrolconfig);
        const PolicyConfig& getSendertimerinactiviypolicy() const;
        void setSendertimerinactiviypolicy(
                        const PolicyConfig& sendertimerinactiviypolicy);
        const PolicyConfig& getRttestimatorpolicy() const;
        void setRttestimatorpolicy(const PolicyConfig& rttestimatorpolicy);
        const std::string toString();
};

/**
 * This class defines the policies paramenters for an EFCP connection
 */
class ConnectionPolicies {
        /** Indicates if DTCP is required */
        bool DTCPpresent;

        /** The configuration of the DTCP instance */
        DTCPConfig dtcpConfiguration;

        /**
         * This policy allows some discretion in selecting the initial sequence
         * number, when DRF is going to be sent.
         */
        PolicyConfig initialseqnumpolicy;

        /**
         * When the sequence number is increasing beyond this value, the
         * sequence number space is close to rolling over, a new connection
         * should be instantiated and bound to the same port-ids, so that new
         * PDUs can be sent on the new connection.
         */
        int seqnumrolloverthreshold;

        /**
         * indicates the maximum time that a receiver will wait before sending
         * an Ack. Some DIFs may wish to set a maximum value for the DIF.
         */
        int initialATimer;

        /**
         * True if partial delivery of an SDU is allowed, false otherwise
         */
        bool partialDelivery;

        /**
         * True if incomplete delivery is allowed (one fragment of SDU
         * delivered is the same as all the SDU delivered), false otherwise
         */
        bool incompleteDelivery;

        /**
         * True if in order delivery of SDUs is mandatory, false otherwise
         */
        bool inOrderDelivery;

        /**
         * The maximum gap of SDUs allowed
         */
        unsigned int maxSDUGap;

public:
        ConnectionPolicies();
        const DTCPConfig& getDtcpConfiguration() const;
        void setDtcpConfiguration(const DTCPConfig& dtcpConfiguration);
        bool isDtcpPresent() const;
        void setDtcpPresent(bool dtcPpresent);
        const PolicyConfig& getInitialseqnumpolicy() const;
        void setInitialseqnumpolicy(const PolicyConfig& initialseqnumpolicy);
        int getSeqnumrolloverthreshold() const;
        void setSeqnumrolloverthreshold(int seqnumrolloverthreshold);
        int getInitialATimer() const;
        void setInitialATimer(int initialATimer);
        bool isInOrderDelivery() const;
        void setInOrderDelivery(bool inOrderDelivery);
        unsigned int getMaxSduGap() const;
        void setMaxSduGap(unsigned int maxSduGap);
        bool isPartialDelivery() const;
        void setPartialDelivery(bool partialDelivery);
        bool isIncompleteDelivery() const;
        void setIncompleteDelivery(bool incompleteDelivery);
        const std::string toString();
};

/**
 * Defines the properties that a QoSCube is able to provide
 */
class QoSCube {

	/** The name of the QoS cube*/
	std::string name;

	/** The id of the QoS cube */
	int id;

        /**
         * The EFCP policies associated to this QoS Cube
         */
        ConnectionPolicies efcpPolicies;

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
	void setId(int id);
	int getId() const;
	const std::string& getName() const;
	void setName(const std::string& name);
        const ConnectionPolicies& getEfcpPolicies() const;
        void setEfcpPolicies(const ConnectionPolicies& efcpPolicies);
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
	const std::string toString();
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
	ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
	ALLOCATE_FLOW_RESPONSE_EVENT,
	FLOW_DEALLOCATION_REQUESTED_EVENT,
	DEALLOCATE_FLOW_RESPONSE_EVENT,
	APPLICATION_UNREGISTERED_EVENT,
	FLOW_DEALLOCATED_EVENT,
	APPLICATION_REGISTRATION_REQUEST_EVENT,
	REGISTER_APPLICATION_RESPONSE_EVENT,
	APPLICATION_UNREGISTRATION_REQUEST_EVENT,
	UNREGISTER_APPLICATION_RESPONSE_EVENT,
	APPLICATION_REGISTRATION_CANCELED_EVENT,
	ASSIGN_TO_DIF_REQUEST_EVENT,
	ASSIGN_TO_DIF_RESPONSE_EVENT,
	UPDATE_DIF_CONFIG_REQUEST_EVENT,
	UPDATE_DIF_CONFIG_RESPONSE_EVENT,
	ENROLL_TO_DIF_REQUEST_EVENT,
	ENROLL_TO_DIF_RESPONSE_EVENT,
	NEIGHBORS_MODIFIED_NOTIFICAITON_EVENT,
	IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
	IPC_PROCESS_QUERY_RIB,
	GET_DIF_PROPERTIES,
	GET_DIF_PROPERTIES_RESPONSE_EVENT,
	OS_PROCESS_FINALIZED,
	IPCM_REGISTER_APP_RESPONSE_EVENT,
	IPCM_UNREGISTER_APP_RESPONSE_EVENT,
	IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT,
	IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
	QUERY_RIB_RESPONSE_EVENT,
	IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
	TIMER_EXPIRED_EVENT,
	IPC_PROCESS_CREATE_CONNECTION_RESPONSE,
	IPC_PROCESS_UPDATE_CONNECTION_RESPONSE,
	IPC_PROCESS_CREATE_CONNECTION_RESULT,
	IPC_PROCESS_DESTROY_CONNECTION_RESULT,
	IPC_PROCESS_DUMP_FT_RESPONSE
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

class BaseResponseEvent: public IPCEvent {
        /** The result of the operation */
        int result;

public:
        BaseResponseEvent(
                        int result,
                        IPCEventType eventType,
                        unsigned int sequenceNumber);
        int getResult() const;
};

/**
 * Event informing about an incoming flow request
 */
class FlowRequestEvent: public IPCEvent {
	/** The port-id that locally identifies the flow */
	int portId;

	/** The name of the DIF that is providing this flow */
	ApplicationProcessNamingInformation DIFName;

	/** The local application name*/
	ApplicationProcessNamingInformation localApplicationName;

	/** The remote application name*/
	ApplicationProcessNamingInformation remoteApplicationName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

	/** True if it is a local application, false if it is a remote one */
	bool localRequest;

	/** 0 if it is an application, or the ID of the IPC Process otherwise */
	int flowRequestorIpcProcessId;

	/** the ID of the IPC Process that will provide the flow*/
	unsigned short ipcProcessId;

public:
	FlowRequestEvent(const FlowSpecification& flowSpecification,
			bool localRequest,
			const ApplicationProcessNamingInformation& localApplicationName,
			const ApplicationProcessNamingInformation& remoteApplicationName,
			int flowRequestorIpcProcessId,
			unsigned int sequenceNumber);
	FlowRequestEvent(int portId,
			const FlowSpecification& flowSpecification,
			bool localRequest,
			const ApplicationProcessNamingInformation& localApplicationName,
			const ApplicationProcessNamingInformation& remoteApplicationName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned short ipcProcessId,
			unsigned int sequenceNumber);
	int getPortId() const;
	bool isLocalRequest() const;
	const FlowSpecification& getFlowSpecification() const;
	void setPortId(int portId);
	void setDIFName(const ApplicationProcessNamingInformation& difName);
	const ApplicationProcessNamingInformation& getDIFName() const;
	const ApplicationProcessNamingInformation& getLocalApplicationName() const;
	const ApplicationProcessNamingInformation& getRemoteApplicationName() const;
	int getFlowRequestorIPCProcessId() const;
	unsigned short getIPCProcessId() const;
};

/**
 * Event informing the IPC Process about a flow deallocation request
 */
class FlowDeallocateRequestEvent: public IPCEvent {
	/** The port-id that locally identifies the flow */
	int portId;

	/** The application that requested the flow deallocation*/
	ApplicationProcessNamingInformation applicationName;

public:
	FlowDeallocateRequestEvent(int portId,
			const ApplicationProcessNamingInformation& appName,
			unsigned int sequenceNumber);
	FlowDeallocateRequestEvent(int portId,
				unsigned int sequenceNumber);
	int getPortId() const;
	const ApplicationProcessNamingInformation& getApplicationName() const;
};

/**
 * Event informing that a flow has been deallocated by an IPC Process, without
 * the application having requested it
 */
class FlowDeallocatedEvent: public IPCEvent {
	/** The port id of the deallocated flow */
	int portId;

	/** An error code indicating why the flow was deallocated */
	int code;

public:
	FlowDeallocatedEvent(int portId, int code);
	int getPortId() const;
	int getCode() const;
	const ApplicationProcessNamingInformation getDIFName() const;
};

/**
 * Identifies the types of application registrations
 * APPLICATION_REGISTRATION_SINGLE_DIF - registers the application in a single
 * DIF, specified by the application
 * APPLICATION_REGISTRATION_ANY_DIF - registers the application in any of the
 * DIFs available to the application, chosen by the IPC Manager
 */
enum ApplicationRegistrationType {
	APPLICATION_REGISTRATION_SINGLE_DIF,
	APPLICATION_REGISTRATION_ANY_DIF
};

/**
 * Contains information about the registration of an application
 */
class ApplicationRegistrationInformation {

        /** The name of the application being registered */
        ApplicationProcessNamingInformation appName;

        /**
         * The id of the IPC process being registered (0 if it is
         * an application
         */
        unsigned short ipcProcessId;

	/** The type of registration requested */
	ApplicationRegistrationType applicationRegistrationType;

	/** Optional DIF name where the application wants to register */
	ApplicationProcessNamingInformation difName;

public:
	ApplicationRegistrationInformation();
	ApplicationRegistrationInformation(
		ApplicationRegistrationType applicationRegistrationType);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	void setApplicationName(
	                const ApplicationProcessNamingInformation& appName);
	ApplicationRegistrationType getRegistrationType() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
	void setDIFName(const ApplicationProcessNamingInformation& difName);
        unsigned short getIpcProcessId() const;
        void setIpcProcessId(unsigned short ipcProcessId);
        const std::string toString();
};

/**
 * Event informing that an application has requested the
 * registration to a DIF
 */
class ApplicationRegistrationRequestEvent: public IPCEvent {

	/** The application registration information*/
	ApplicationRegistrationInformation applicationRegistrationInformation;

public:
	ApplicationRegistrationRequestEvent(
		const ApplicationRegistrationInformation&
		applicationRegistrationInformation, unsigned int sequenceNumber);
	const ApplicationRegistrationInformation&
		getApplicationRegistrationInformation() const;
};

class BaseApplicationRegistrationEvent: public IPCEvent {
        /** The application that wants to unregister */
        ApplicationProcessNamingInformation applicationName;

        /** The DIF to which the application wants to cancel the registration */
        ApplicationProcessNamingInformation DIFName;

public:
        BaseApplicationRegistrationEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& DIFName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber);
        BaseApplicationRegistrationEvent(
                        const ApplicationProcessNamingInformation& appName,
                        IPCEventType eventType,
                        unsigned int sequenceNumber);
        const ApplicationProcessNamingInformation& getApplicationName() const;
        const ApplicationProcessNamingInformation& getDIFName() const;
};

/**
 * Event informing that an application has requested the
 * unregistration from a DIF
 */
class ApplicationUnregistrationRequestEvent:
                public BaseApplicationRegistrationEvent {
public:
	ApplicationUnregistrationRequestEvent(
			const ApplicationProcessNamingInformation& appName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber);
};

class BaseApplicationRegistrationResponseEvent:
                public BaseApplicationRegistrationEvent {
        /** The result of the operation */
        int result;

public:
        BaseApplicationRegistrationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& DIFName,
                        int result,
                        IPCEventType eventType,
                        unsigned int sequenceNumber);
        BaseApplicationRegistrationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int result,
                        IPCEventType eventType,
                        unsigned int sequenceNumber);
        int getResult() const;
};

/**
 * Event informing about the result of an application registration request
 */
class RegisterApplicationResponseEvent:
                public BaseApplicationRegistrationResponseEvent {
public:
        RegisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& difName,
                        int result, unsigned int sequenceNumber);
};

/**
 * Event informing about the result of an application unregistration request
 */
class UnregisterApplicationResponseEvent:
                public BaseApplicationRegistrationResponseEvent {
public:
        UnregisterApplicationResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int result, unsigned int sequenceNumber);
};

/**
 * Event informing about the application decision regarding the
 * acceptance/denial of a flow request
 */
class AllocateFlowResponseEvent: public BaseResponseEvent {
        /**
         * If the flow was denied, this field controls wether the application
         * wants the IPC Process to reply to the source or not
         */
        bool notifySource;

        /** 0 if it is an application, or the ID of the IPC Process otherwise */
        int flowAcceptorIpcProcessId;

public:
        AllocateFlowResponseEvent(
                        int result,
                        bool notifysource,
                        int flowAcceptorIpcProcessId,
                        unsigned int sequenceNumber);
        bool isNotifySource() const;
        int getFlowAcceptorIpcProcessId() const;
};

/**
 * Event informing that an OS process (an application or an
 * IPC Process daemon) has finalized
 */
class OSProcessFinalizedEvent: public IPCEvent {
	/**
	 * The naming information of the application that has
	 * finalized
	 */
	ApplicationProcessNamingInformation applicationName;

	/**
	 * If this id is greater than 0, it means that the process
	 * that finalized was an IPC Process Daemon. Otherwise it is an
	 * application process.
	 */
	unsigned int ipcProcessId;

public:
	OSProcessFinalizedEvent(const ApplicationProcessNamingInformation& appName,
			unsigned int ipcProcessId, unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	unsigned int getIPCProcessId() const;
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

	/**
	 * Blocks until there is an event available, no more than the
	 * time specified
	 */
	IPCEvent * eventTimedWait(int seconds, int nanoseconds);
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
 * Represents a parameter that has a name and value
 */
class Parameter {
        std::string name;
        std::string value;

public:
        Parameter();
        Parameter(const std::string & name, const std::string & value);
        bool operator==(const Parameter &other) const;
        bool operator!=(const Parameter &other) const;
        const std::string& getName() const;
        void setName(const std::string& name);
        const std::string& getValue() const;
        void setValue(const std::string& value);
};

/**
 * Contains the values of the constants for the Error and Flow Control
 * Protocol (EFCP)
 */
class DataTransferConstants {

        /** The length of QoS-id field in the DTP PCI, in bytes */
        unsigned short qosIdLenght;

        /** The length of the Port-id field in the DTP PCI, in bytes */
        unsigned short portIdLength;

        /** The length of the CEP-id field in the DTP PCI, in bytes */
        unsigned short cepIdLength;

        /** The length of the sequence number field in the DTP PCI, in bytes */
        unsigned short sequenceNumberLength;

        /** The length of the address field in the DTP PCI, in bytes */
        unsigned short addressLength;

        /** The length of the length field in the DTP PCI, in bytes */
        unsigned short lengthLength;

        /** The maximum length allowed for a PDU in this DIF, in bytes */
        unsigned int maxPDUSize;

        /**
         * True if the PDUs in this DIF have CRC, TTL, and/or encryption. Since
         * headers are encrypted, not just user data, if any flow uses encryption,
         * all flows within the same DIF must do so and the same encryption
         * algorithm must be used for every PDU; we cannot identify which flow
         * owns a particular PDU until it has been decrypted.
         */
        bool DIFIntegrity;

        /**
         * The maximum PDU lifetime in this DIF, in milliseconds. This is MPL
         * in delta-T
         */
        unsigned int maxPDULifetime;

public:
        DataTransferConstants();
        unsigned short getAddressLength() const;
        void setAddressLength(unsigned short addressLength);
        unsigned short getCepIdLength() const;
        void setCepIdLength(unsigned short cepIdLength);
        bool isDifIntegrity() const;
        void setDifIntegrity(bool difIntegrity);
        unsigned short getLengthLength() const;
        void setLengthLength(unsigned short lengthLength);
        unsigned int getMaxPduLifetime() const;
        void setMaxPduLifetime(unsigned int maxPduLifetime);
        unsigned int getMaxPduSize() const;
        void setMaxPduSize(unsigned int maxPduSize);
        unsigned short getPortIdLength() const;
        void setPortIdLength(unsigned short portIdLength);
        unsigned short getQosIdLenght() const;
        void setQosIdLenght(unsigned short qosIdLenght);
        unsigned short getSequenceNumberLength() const;
        void setSequenceNumberLength(unsigned short sequenceNumberLength);
        bool isInitialized();
        const std::string toString();
};

/**
 * Contains the configuration of the Error and Flow Control Protocol for a
 * particular DIF
 */
class EFCPConfiguration {
public:
        EFCPConfiguration();
        const DataTransferConstants& getDataTransferConstants() const;
        void setDataTransferConstants(
                        const DataTransferConstants& dataTransferConstants);
        const std::list<QoSCube>& getQosCubes() const;
        void setQosCubes(const std::list<QoSCube>& qosCubes);
        void addQoSCube(const QoSCube& qosCube);
        const PolicyConfig& getUnknownFlowPolicy() const;
        void setUnknownFlowPolicy(const PolicyConfig& unknownFlowPolicy);

private:
        /**
         * DIF-wide parameters that define the concrete syntax of EFCP for this
         *  DIF and other DIF-wide values
         */
        DataTransferConstants dataTransferConstants;

        /**
         * When a PDU arrives for a Data Transfer Flow terminating in this
         * IPC-Process and there is no active DTSV, this policy consults the
         * ResourceAllocator to determine what to do.
         */
        PolicyConfig unknownFlowPolicy;

        /**
         * The QoS cubes supported by the DIF, and its associated EFCP policies
         */
        std::list<QoSCube> qosCubes;
};

class FlowAllocatorConfiguration {
public:
        FlowAllocatorConfiguration();
        const PolicyConfig& getAllocateNotifyPolicy() const;
        void setAllocateNotifyPolicy(const PolicyConfig& allocateNotifyPolicy);
        const PolicyConfig& getAllocateRetryPolicy() const;
        void setAllocateRetryPolicy(const PolicyConfig& allocateRetryPolicy);
        int getMaxCreateFlowRetries() const;
        void setMaxCreateFlowRetries(int maxCreateFlowRetries);
        const PolicyConfig& getNewFlowRequestPolicy() const;
        void setNewFlowRequestPolicy(const PolicyConfig& newFlowRequestPolicy);
        const PolicyConfig& getSeqRollOverPolicy() const;
        void setSeqRollOverPolicy(const PolicyConfig& seqRollOverPolicy);

private:
        /** Maximum number of attempts to retry the flow allocation */
        int maxCreateFlowRetries;

        /**
         * This policy determines when the requesting application is given
         * an Allocate_Response primitive. In general, the choices are once
         * the request is determined to be well-formed and a create_flow
         * request has been sent, or withheld until a create_flow response has
         * been received and MaxCreateRetires has been exhausted.
         */
        PolicyConfig allocateNotifyPolicy;

        /**
         * This policy is used when the destination has refused the create_flow
         * request, and the FAI can overcome the cause for refusal and try
         * again. This policy should re-formulate the request. This policy
         * should formulate the contents of the reply.
         */
        PolicyConfig allocateRetryPolicy;

        /**
         * This policy is used to convert an allocate request to a create flow
         * request. Its primary task is to translate the request into the
         * proper QoS-class set, flow set and access control capabilities.
         */
        PolicyConfig newFlowRequestPolicy;

        /**
         * This policy is used when the SeqRollOverThres event occurs and
         * action may be required by the Flow Allocator to modify the bindings
         * between connection-endpoint-ids and port-ids.
         */
        PolicyConfig seqRollOverPolicy;
};

/**
 * Contains the configuration data of the Relaying and Multiplexing Task for a
 * particular DIF
 */
class RMTConfiguration {
public:
        RMTConfiguration();
        const PolicyConfig& getMaxQueuePolicy() const;
        void setMaxQueuePolicy(const PolicyConfig& maxQueuePolicy);
        const PolicyConfig& getRmtQueueMonitorPolicy() const;
        void setRmtQueueMonitorPolicy(const PolicyConfig& rmtQueueMonitorPolicy);
        const PolicyConfig& getRmtSchedulingPolicy() const;
        void setRmtSchedulingPolicy(const PolicyConfig& rmtSchedulingPolicy);

private:
        /**
         * Three parameters are provided to monitor the queues. This policy
         * can be invoked whenever a PDU is placed in a queue and may keep
         * additional variables that may be of use to the decision process of
         * the RMT-Scheduling Policy and the MaxQPolicy.
         */
        PolicyConfig rmtQueueMonitorPolicy;

        /**
         * This is the meat of the RMT. This is the scheduling algorithm that
         * determines the order input and output queues are serviced. We have
         * not distinguished inbound from outbound. That is left to the policy.
         * To do otherwise, would impose a policy. This policy may implement
         * any of the standard scheduling algorithms, FCFS, LIFO,
         * longestQfirst, priorities, etc.
         */
        PolicyConfig rmtSchedulingPolicy;

        /**
         * This policy is invoked when a queue reaches or crosses the threshold
         *  or maximum queue lengths allowed for this queue. Note that maximum
         *  length may be exceeded.
         */
        PolicyConfig maxQueuePolicy;
};

/**
 * Link State routing configuration
 */
class LinkStateRoutingConfiguration {
private:
        static const int PULSES_UNTIL_FSO_EXPIRATION_DEFAULT = 100000;
        static const int WAIT_UNTIL_READ_CDAP_DEFAULT = 5001;
        static const int WAIT_UNTIL_ERROR_DEFAULT = 5001;
        static const int WAIT_UNTIL_PDUFT_COMPUTATION_DEFAULT = 103;
        static const int WAIT_UNTIL_FSODB_PROPAGATION_DEFAULT = 101;
        static const int WAIT_UNTIL_AGE_INCREMENT_DEFAULT = 997;
        static const std::string DEFAULT_ROUTING_ALGORITHM;
        int objectMaximumAge;
        int waitUntilReadCDAP;
        int waitUntilError;
        int waitUntilPDUFTComputation;
        int waitUntilFSODBPropagation;
        int waitUntilAgeIncrement;
        std::string routingAlgorithm;
public:
        LinkStateRoutingConfiguration();
        const std::string toString();
        int getWaitUntilAgeIncrement() const;
        void setWaitUntilAgeIncrement(const int waitUntilAgeIncrement);
        int getWaitUntilError() const;
        void setWaitUntilError(const int waitUntilError);
        int getWaitUntilFSODBPropagation() const;
        void setWaitUntilFSODBPropagation(const int waitUntilFsodbPropagation);
        int getWaitUntilPDUFTComputation() const;
        void setWaitUntilPDUFTComputation(const int waitUntilPduftComputation);
        int getWaitUntilReadCDAP() const;
        void setWaitUntilReadCDAP(const int waitUntilReadCdap);
        int getObjectMaximumAge() const;
        void setObjectMaximumAge(const int objectMaximumAge);
        const std::string& getRoutingAlgorithm() const;
        void setRoutingAlgorithm(const std::string& routingAlgorithm);
};

/**
 * PDU F Table Generator Configuration
 */
class PDUFTableGeneratorConfiguration {
private:
        /** Name, version and configuration of the PDU FT Generator policy */
        PolicyConfig pduFTGeneratorPolicy;

        /**
         * Link state routing configuration parameters - only relevant if a
         * link-state routing PDU FT Generation policy is used
         */
        LinkStateRoutingConfiguration linkStateRoutingConfiguration;
public:
        PDUFTableGeneratorConfiguration();
        PDUFTableGeneratorConfiguration(const PolicyConfig& pduFTGeneratorPolicy);
        const PolicyConfig& getPduFtGeneratorPolicy() const;
        void setPduFtGeneratorPolicy(const PolicyConfig& pduFtGeneratorPolicy);
        const LinkStateRoutingConfiguration& getLinkStateRoutingConfiguration() const;
        void setLinkStateRoutingConfiguration(
                        const LinkStateRoutingConfiguration& linkStateRoutingConfiguration);
};

/**
 * Contains the data about a DIF Configuration
 * (QoS cubes, policies, parameters, etc)
 */
class DIFConfiguration {

        /** The address of the IPC Process in the DIF */
        unsigned int address;

        /** Configuration of the Error and Flow Control Protocol */
        EFCPConfiguration efcpConfiguration;

	/** Configuration of the Relaying and Multiplexing Task */
	RMTConfiguration rmtConfiguration;

	/** PDUFT Configuration parameters of the DIF	*/
	PDUFTableGeneratorConfiguration pdufTableGeneratorConfiguration;

	/** Flow Allocator configuration parameters of the DIF */
	FlowAllocatorConfiguration faConfiguration;

	/** Other configuration parameters of the DIF */
	std::list<Parameter> parameters;

        /** Other policies of the DIF */
        std::list<PolicyConfig> policies;

public:
        unsigned int getAddress() const;
        void setAddress(unsigned int address);
        const EFCPConfiguration& getEfcpConfiguration() const;
        void setEfcpConfiguration(const EFCPConfiguration& efcpConfiguration);
        const PDUFTableGeneratorConfiguration&
                getPduFTableGeneratorConfiguration() const;
        void setPduFTableGeneratorConfiguration(
                        const PDUFTableGeneratorConfiguration& pdufTableGeneratorConfiguration);
        const RMTConfiguration& getRmtConfiguration() const;
        void setRmtConfiguration(const RMTConfiguration& rmtConfiguration);
	const std::list<PolicyConfig>& getPolicies();
	void setPolicies(const std::list<PolicyConfig>& policies);
	void addPolicy(const PolicyConfig& policy);
	const std::list<Parameter>& getParameters() const;
	void setParameters(const std::list<Parameter>& parameters);
	void addParameter(const Parameter& parameter);
        const FlowAllocatorConfiguration& getFaConfiguration() const;
        void setFaConfiguration(
                        const FlowAllocatorConfiguration& faConfiguration);
};

/**
 * Contains the information about a DIF (name, type, configuration)
 */
class DIFInformation{
	/** The type of DIF */
	std::string difType;

	/** The name of the DIF */
	ApplicationProcessNamingInformation difName;

	/** The DIF Configuration (qoscubes, policies, parameters, etc) */
	DIFConfiguration difConfiguration;

public:
	const ApplicationProcessNamingInformation& getDifName() const;
	void setDifName(const ApplicationProcessNamingInformation& difName);
	const std::string& getDifType() const;
	void setDifType(const std::string& difType);
	const DIFConfiguration& getDifConfiguration() const;
	void setDifConfiguration(const DIFConfiguration& difConfiguration);
};

/**
 * Represents an IPC Process with whom we're enrolled
 */
class Neighbor {

        /** The IPC Process name of the neighbor */
        ApplicationProcessNamingInformation name;

        /**
         * The name of the supporting DIF used to exchange data
         * with the neighbor
         */
        ApplicationProcessNamingInformation supportingDifName;

        /**
         * The names of all the supporting DIFs of this neighbor
         */
        std::list<ApplicationProcessNamingInformation> supportingDifs;

        /** The address */
        unsigned int address;

        /** Tells if it is enrolled or not */
        bool enrolled;

        /** The average RTT in ms */
        unsigned int averageRTTInMs;

        /** The underlying portId used to communicate with this neighbor */
        int underlyingPortId;

        /**
         * The last time a KeepAlive message was received from
         * that neighbor, in ms
         */
        long long lastHeardFromTimeInMs;

        /**
         * The number of times we have tried to re-enroll with the
         * neighbor after the connectivity has been lost
         */
        unsigned int numberOfEnrollmentAttempts;

public:
        Neighbor();
        bool operator==(const Neighbor &other) const;
        bool operator!=(const Neighbor &other) const;
        const ApplicationProcessNamingInformation& getName() const;
        void setName(const ApplicationProcessNamingInformation& name);
        const ApplicationProcessNamingInformation&
                getSupportingDifName() const;
        void setSupportingDifName(
                const ApplicationProcessNamingInformation& supportingDifName);
        const std::list<ApplicationProcessNamingInformation>& getSupportingDifs();
        void setSupportingDifs(
                        const std::list<ApplicationProcessNamingInformation>& supportingDifs);
        void addSupoprtingDif(const ApplicationProcessNamingInformation& supportingDif);
        unsigned int getAddress() const;
        void setAddress(unsigned int address);
        unsigned int getAverageRttInMs() const;
        void setAverageRttInMs(unsigned int averageRttInMs);
        bool isEnrolled() const;
        void setEnrolled(bool enrolled);
        long long getLastHeardFromTimeInMs() const;
        void setLastHeardFromTimeInMs(long long lastHeardFromTimeInMs);
        int getUnderlyingPortId() const;
        void setUnderlyingPortId(int underlyingPortId);
        unsigned int getNumberOfEnrollmentAttempts() const;
        void setNumberOfEnrollmentAttempts(
                        unsigned int numberOfEnrollmentAttempts);
        const std::string toString();
};

/**
 * Represents the value of an object stored in the RIB
 */
class RIBObjectValue{
	//TODO
};

/**
 * Represents an object in the RIB
 */
class RIBObject{

	/** The class (type) of object */
	std::string clazz;

	/** The name of the object (unique within a class)*/
	std::string name;

	/** A synonim for clazz+name (unique within the RIB) */
	unsigned long instance;

	/** The value of the object */
	RIBObjectValue value;

	/**
	 * The value of the object, encoded in an string for
	 * displayable purposes
	 */
	std::string displayableValue;

	/** Geneartes a unique object instance */
	unsigned long generateObjectInstance();

public:
	RIBObject();
	RIBObject(std::string clazz, std::string name,
	                long long instance, RIBObjectValue value);
	bool operator==(const RIBObject &other) const;
	bool operator!=(const RIBObject &other) const;
	const std::string& getClazz() const;
	void setClazz(const std::string& clazz);
	unsigned long getInstance() const;
	void setInstance(unsigned long  instance);
	const std::string& getName() const;
	void setName(const std::string& name);
	RIBObjectValue getValue() const;
	void setValue(RIBObjectValue value);
        const std::string& getDisplayableValue() const;
        void setDisplayableValue(const std::string& displayableValue);
};

/**
 * Thrown when there are problems initializing librina
 */
class InitializationException: public IPCException {
public:
	InitializationException():
		IPCException("Problems initializing librina"){
	}
	InitializationException(const std::string& description):
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
 * Initialize librina providing the local Netlink port-id where this librina
 * instantiation will be bound
 * @param localPort NL port used by this instantiation of librina
 * @param logLevel librina log level
 * @param pathToLogFile the path to the log file
 */
void initialize(unsigned int localPort, const std::string& logLevel,
                const std::string& pathToLogFile)
        throw (InitializationException);

/**
 * Initialize librina letting the OS choose the Netlink port-id where this
 * librina instantiation will be bound
 * @param logLevel librina log level
 * @param pathToLogFile the path to the log file
 */
void initialize(const std::string& logLevel,
                const std::string& pathToLogFile)
        throw (InitializationException);

}
#endif

#endif
