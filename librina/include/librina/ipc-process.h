/*
 * IPC Process
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef LIBRINA_IPC_PROCESS_H
#define LIBRINA_IPC_PROCESS_H

#ifdef __cplusplus

#include <string>
#include <list>

#include "librina/configuration.h"
#include "librina/application.h"
#include "librina/enrollment.h"
#include "librina/ipc-daemons.h"

namespace rina {

/**
 * The IPC Manager requests the IPC Process to become a member of a
 * DIF, and provides de related information
 */
class AssignToDIFRequestEvent: public IPCEvent {
public:
	/** The information of the DIF the IPC Process is being assigned to*/
	DIFInformation difInformation;

	AssignToDIFRequestEvent(const DIFInformation& difInformation,
			unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
	const DIFInformation& getDIFInformation() const;
#endif
};

/**
 * The IPC Manager requests the IPC Process to update the configuration
 * of the DIF he is currently a member of
 */
class UpdateDIFConfigurationRequestEvent: public IPCEvent {
public:
        /** The new configuration of the DIF*/
        DIFConfiguration difConfiguration;

        UpdateDIFConfigurationRequestEvent(
                        const DIFConfiguration& difConfiguration,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        const DIFConfiguration& getDIFConfiguration() const;
#endif
};


/**
 * Supporting class for IPC Process DIF Registration events
 */
class IPCProcessDIFRegistrationEvent: public IPCEvent {
public:
	/** The name of the IPC Process registered to the N-1 DIF */
	ApplicationProcessNamingInformation ipcProcessName;

	/** The name of the N-1 DIF where the IPC Process has been registered*/
	ApplicationProcessNamingInformation difName;

	/** True if the IPC Process has been registered in a DIF, false otherwise */
	bool registered;

	IPCProcessDIFRegistrationEvent(
	                const ApplicationProcessNamingInformation& ipcProcessName,
			const ApplicationProcessNamingInformation& difName,
			bool registered,
			unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
	const ApplicationProcessNamingInformation& getIPCProcessName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
	bool isRegistered() const;
#endif
};

/**
 * The IPC Manager queries the RIB of the IPC Process
 */
class QueryRIBRequestEvent: public IPCEvent {
public:
	/** The class of the object being queried*/
	std::string objectClass;

	/** The name of the object being queried */
	std::string objectName;

	/**
	 * The instance of the object being queried. Either objectname +
	 * object class or object instance have to be specified
	 */
	long objectInstance;

	/** Number of levels below the object_name the query affects*/
	int scope;

	/**
	 * Regular expression applied to all nodes affected by the query
	 * in order to decide whether they have to be returned or not
	 */
	std::string filter;

	QueryRIBRequestEvent(const std::string& objectClass,
			const std::string& objectName, long objectInstance, int scope,
			const std::string& filter, unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
	const std::string& getObjectClass() const;
	const std::string& getObjectName() const;
	long getObjectInstance() const;
	int getScope() const;
	const std::string& getFilter() const;
#endif
};

/**
 * The IPC Manager accesses a policy-set-related parameter
 * of an IPC process
 */
class SetPolicySetParamRequestEvent: public IPCEvent {
public:
        /** The path of the sybcomponent/policy-set to be addressed */
	std::string path;

	/** The name of the parameter being accessed */
	std::string name;

	/** The value to assign to the parameter */
	std::string value;

	SetPolicySetParamRequestEvent(const std::string& path,
                        const std::string& name, const std::string& value,
			unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * The IPC Manager selects a policy-set for an IPC process component
 */
class SelectPolicySetRequestEvent: public IPCEvent {
public:
        /** The path of the sybcomponent to be addressed */
	std::string path;

	/** The name of the policy-set to select */
	std::string name;

	SelectPolicySetRequestEvent(const std::string& path,
                                    const std::string& name,
			            unsigned int sequenceNumber,
				    unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * The IPC Manager wants to load or unload a plugin for
 * an IPC process
 */
class PluginLoadRequestEvent: public IPCEvent {
public:
	/** The name of the plugin to be loaded or unloaded */
	std::string name;

	/** Specifies whether the plugin is to be loaded or unloaded */
	bool load;

	PluginLoadRequestEvent(const std::string& name, bool load,
                               unsigned int sequenceNumber,
			       unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * The Kernel components of the IPC Process report about the result of a
 * create EFCP connection operation
 */
class CreateConnectionResponseEvent: public IPCEvent {
public:
        /** The port-id where the connection will be bound to */
        int portId;

        /**
         * The source connection-endpoint id if the connection was created
         * successfully, or a negative number indicating an error code in
         * case of failure
         */
        int cepId;

        CreateConnectionResponseEvent(int portId, int cepId,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        int getCepId() const;
        int getPortId() const;
#endif
};

/**
 * The Kernel components of the IPC Process report about the result of a
 * create EFCP connection operation
 */
class UpdateConnectionResponseEvent: public IPCEvent {
public:
        /** The port-id where the connection will be bound to */
        int portId;

        /**
         * The result of the operation (0 successful)
         */
        int result;

        UpdateConnectionResponseEvent(int portId, int result,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        int getResult() const;
        int getPortId() const;
#endif
};

/**
 * The Kernel components of the IPC Process report about the result of a
 * create EFCP connection arrived operation
 */
class CreateConnectionResultEvent: public IPCEvent {
public:
        /** The port-id where the connection will be bound to */
        int portId;

        /**
         * The source connection-endpoint id if the connection was created
         * successfully, or a negative number indicating an error code in
         * case of failure
         */
        int sourceCepId;

        /** The destination cep-id of the connection */
        int destCepId;

        CreateConnectionResultEvent(int portId, int sourceCepId,
                        int destCepId, unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        int getSourceCepId() const;
        int getDestCepId() const;
        int getPortId() const;
#endif
};

class DestroyConnectionResultEvent: public IPCEvent {
public:
        /** The port-id where the connection will be bound to */
        int portId;

        /** The destination cep-id of the connection */
        int result;

        DestroyConnectionResultEvent(int portId, int result,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        int getResult() const;
        int getPortId() const;
#endif
};

/**
 * Thrown when there are problems notifying the IPC Manager about the
 * result of an Assign to DIF operation
 */
class AssignToDIFResponseException: public IPCException {
public:
	AssignToDIFResponseException():
		IPCException("Problems informing the IPC Manager about the result of an assign to DIF operation"){
	}
	AssignToDIFResponseException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying the IPC Manager about the
 * result of a register application operation
 */
class RegisterApplicationResponseException: public IPCException {
public:
	RegisterApplicationResponseException():
		IPCException("Problems informing the IPC Manager about the result of a register application response operation"){
	}
	RegisterApplicationResponseException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying the IPC Manager about the
 * result of an unregister application operation
 */
class UnregisterApplicationResponseException: public IPCException {
public:
	UnregisterApplicationResponseException():
		IPCException("Problems informing the IPC Manager about the result of an unegister application response operation"){
	}
	UnregisterApplicationResponseException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying the IPC Manager about the
 * result of an allocate flow operation
 */
class AllocateFlowResponseException: public IPCException {
public:
	AllocateFlowResponseException():
		IPCException("Problems informing the IPC Manager about the result of an unegister application response operation"){
	}
	AllocateFlowResponseException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems notifying the IPC Manager about the
 * result of a query RIB operation
 */
class QueryRIBResponseException: public IPCException {
public:
	QueryRIBResponseException():
		IPCException("Problems informing the IPC Manager about the result of a query RIB response operation"){
	}
	QueryRIBResponseException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems allocating a remote flow to a
 * local application
 */
class AllocateFlowRequestArrivedException: public IPCException {
public:
	AllocateFlowRequestArrivedException():
		IPCException("Problems allocating a remote flow to a local application"){
	}
	AllocateFlowRequestArrivedException(const std::string& description):
		IPCException(description){
	}
};


/**
 * Thrown when there are problems notifying the application about the
 * result of a deallocate operation
 */
class DeallocateFlowResponseException: public IPCException {
public:
	DeallocateFlowResponseException():
		IPCException("Problems informing the application about the result of a deallocate operation"){
	}
	DeallocateFlowResponseException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems requesting the Kernel to create an EFCP connection
 */
class CreateConnectionException: public IPCException {
public:
        CreateConnectionException():
                IPCException("Problems creating an EFCP connection"){
        }
        CreateConnectionException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems requesting the Kernel to update an EFCP connection
 */
class UpdateConnectionException: public IPCException {
public:
        UpdateConnectionException():
                IPCException("Problems updating an EFCP connection"){
        }
        UpdateConnectionException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems requesting the Kernel to destroy an EFCP connection
 */
class DestroyConnectionException: public IPCException {
public:
        DestroyConnectionException():
                IPCException("Problems destroying an EFCP connection"){
        }
        DestroyConnectionException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems requesting the Kernel to allocate or deallocate a
 * port-id
 */
class PortAllocationException: public IPCException {
public:
        PortAllocationException():
                IPCException("Problems requesting the allocation/deallocation of a port-id"){
        }
        PortAllocationException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems requesting the Kernel to modify
 * the PDU Forwarding table
 * port-id
 */
class PDUForwardingTableException: public IPCException {
public:
        PDUForwardingTableException():
                IPCException("Problems requesting modification of PDU Forwarding Table"){
        }
        PDUForwardingTableException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Class used by the IPC Processes to interact with the IPC Manager. Extends
 * the basic IPC Manager in librina-application with IPC Process specific
 * functionality
 */
class ExtendedIPCManager: public IPCManager {
public:
	/** The ID of the IPC Process */
	unsigned short ipcProcessId;

	/** The portId of the IPC Manager */
	unsigned int ipcManagerPort;

	/**
	 * True if the IPC Process has been initialized,
	 * false otherwise
	 */
	bool ipcProcessInitialized;

	/** The current configuration of the IPC Process */
	DIFInformation currentDIFInformation;

	static const std::string error_allocate_flow;

	Lockable lock;

	ExtendedIPCManager();
	~ExtendedIPCManager() throw();
#ifndef SWIG
	const DIFInformation& getCurrentDIFInformation() const;
	void setCurrentDIFInformation(const DIFInformation& currentDIFInformation);
	unsigned short getIpcProcessId() const;
	void setIpcProcessId(unsigned short ipcProcessId);
	void setIPCManagerPort(unsigned int ipcManagerPort);
#endif

	/**
	 * Notify the IPC Manager about the successful initialization of the
	 * IPC Process Daemon. Now it is ready to receive messages.
	 * @param name the name of the IPC Process
	 * @throws IPCException if the process is already initialized or
	 * an error occurs
	 */
	void notifyIPCProcessInitialized(
		const ApplicationProcessNamingInformation& name);

	/**
	 * True if the IPC Process has been successfully initialized, false
	 * otherwise
	 * @return
	 */
	bool isIPCProcessInitialized() const;

	/**
	 * The IPC Process has been registered to an N-1 DIF
	 * @param appName
	 * @param DIFName
	 * @return
	 */
	ApplicationRegistration * appRegistered(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName);

	/**
	 * The IPC Process has been unregistered from the DIF called DIFName,
	 * update the internal data structrues
	 * @param appName
	 * @param DIFName
	 */
	void appUnregistered(const ApplicationProcessNamingInformation& appName,
			     const ApplicationProcessNamingInformation& DIFName);

	/**
	 * Reply to the IPC Manager, informing it about the result of an "assign
	 * to DIF" operation
	 * @param event the event that trigered the operation
	 * @param result the result of the operation (0 successful)
	 * @throws AssignToDIFResponseException
	 */
	void assignToDIFResponse(const AssignToDIFRequestEvent& event, int result);

	/**
	 * Reply to the IPC Manager, informing it about the result of an "enroll
	 * to DIF" operation
	 * @param event the event that trigerred the operation
	 * @param result the result of the operation (0 successful)
	 * @param newNeighbors the new neighbors after the enrollment operation
	 * @param DIFInforamtion the DIF configuration after enrollment
	 * @throws EnrollException if there are problems communicating with the
	 * IPC Manager
	 */
	void enrollToDIFResponse(const EnrollToDAFRequestEvent& event,
				 int result,
				 const std::list<Neighbor> & newNeighbors,
				 const DIFInformation& difInformation);

	/**
	 * Reply to the IPC Manager, informing it about the result of a "disconnect
	 * neighbor" operation
	 * @param event the event that trigerred the operation
	 * @param result the result of the operation (0 successful)
	 * @throws IPCException if there are problems communicating with the
	 * IPC Manager
	 */
	void disconnectNeighborResponse(const DisconnectNeighborRequestEvent& event,
				        int result);

	/**
	 * Reply to the IPC Manager, informing it about the result of a "register
	 * application request" operation
	 * @param event
	 * @param result
	 * @throws RegisterApplicationResponseException
	 */
	void registerApplicationResponse(
			const ApplicationRegistrationRequestEvent& event, int result);

	/**
	 * Reply to the IPC Manager, informing it about the result of a "unregister
	 * application request" operation
	 * @param event
	 * @param result
	 * @throws UnregisterApplicationResponseException
	 */
	void unregisterApplicationResponse(
			const ApplicationUnregistrationRequestEvent& event, int result);

	/**
	 * Reply to the IPC Manager, informing it about the result of a "allocate
	 * flow response" operation
	 * @param event
	 * @param result
	 * @throws AllocateFlowResponseException
	 */
	void allocateFlowRequestResult(const FlowRequestEvent& event, int result);

	/**
	 * Tell the IPC Manager that an allocate flow request targeting a local
	 * application registered in this IPC Process has arrived. The IPC manager
	 * will contact the application and ask it if it accepts the flow. IF it
	 * does it, it will assign a port-id to the flow. Either way it will reply
	 * the IPC Process
	 * @param localAppName
	 * @param remoteAppName
	 * @param flowSpecification
	 * @param portId the portId for the flow
	 * @returns a handler to correlate the response
	 * @throws AllocateFlowRequestArrivedException if there are issues during
	 * the operation or the application rejects the flow
	 */
	unsigned int allocateFlowRequestArrived(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const FlowSpecification& flowSpecification,
			int portId);

	/**
	 * Overrides IPCManager's operation
	 * Requests the allocation of a Flow
	 *
	 * @param localAppName The naming information of the local application
	 * @param remoteAppName The naming information of the remote application
	 * @param flowSpecifiction The characteristics required for the flow
	 * @return A handler to be able to identify the proper response event
	 * @throws FlowAllocationException if there are problems during the flow allocation
	 */
	unsigned int requestFlowAllocation(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const FlowSpecification& flow);

	/**
	 * Overrides IPCManager's operation
	 * Requests the allocation of a flow using a speficif dIF
	 * @param localAppName The naming information of the local application
	 * @param remoteAppName The naming information of the remote application
	 * @param flowSpecifiction The characteristics required for the flow
	 * @param difName The DIF through which we want the flow allocated
	 * @return A handler to be able to identify the proper response event
	 * @throws FlowAllocationException if there are problems during the flow allocation
	 */
	unsigned int requestFlowAllocationInDIF(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const ApplicationProcessNamingInformation& difName,
			const FlowSpecification& flow);

	/**
	 * Overrides IPCManager's operation
	 * Confirms or denies the request for a flow to this application.
	 *
	 * @param flowRequestEvent information of the flow request
	 * @param result 0 means the flow is accepted, a different number
	 * indicates the deny code
	 * @param notifySource if true the source IPC Process will get
	 * the allocate flow response message back, otherwise it will be ignored
	 * @return Flow If the flow is accepted, returns the flow object
	 * @throws FlowAllocationException If there are problems
	 * confirming/denying the flow
	 */
	FlowInformation allocateFlowResponse(const FlowRequestEvent& flowRequestEvent,
					     int result,
					     bool notifySource);

	/**
	 * Invoked by the ipC Process to notify that a flow has been remotely
	 * unallocated
	 * @param portId
	 * @param code
	 * @throws DeallocateFlowResponseException
	 */
	void flowDeallocatedRemotely(int portId, int code);

	/**
	 * Reply to the IPC Manager, providing 0 or more RIB Objects in response to
	 * a "query RIB request"
	 * @param event
	 * @param result
	 * @param ribObjects
	 * @throws QueryRIBResponseException
	 */
	void queryRIBResponse(const QueryRIBRequestEvent& event, int result,
			      const std::list<rib::RIBObjectData>& ribObjects);

	/**
	 * Request an available portId to the kernel
	 * @param The id of the IPC Process that will be using the flow
	 * associated to the port-id requested (0 if is an application)
	 * @param appName The name of the application that requested
	 * the flow (could be an IPC Process or a regular application)
	 * @return the handle to the response message
	 * @throws PortAllocationException if something goes wrong
	 */
	unsigned int allocatePortId(const ApplicationProcessNamingInformation& appName,
				    const FlowSpecification& fspec);

	/**
	 * Request the kernel to free a used port-id
	 * @param portId the port-id to be freed
	 * @return the handle to the request message
	 * @throws PortAllocationException if something goes wrong
	 */
	unsigned int deallocatePortId(int portId);

	/**
	 * Reply to the IPC Manager, informing it about the result of a
         * setPolicySetParam operation
	 * @param event the event that trigered the operation
	 * @param result the result of the operation (0 successful)
	 * @throws SetPolicySetParamException
	 */
	void setPolicySetParamResponse(
                const SetPolicySetParamRequestEvent& event, int result);

	/**
	 * Reply to the IPC Manager, informing it about the result of a
         * selectPolicySet operation
	 * @param event the event that trigered the operation
	 * @param result the result of the operation (0 successful)
	 * @throws SelectPolicySetException
	 */
	void selectPolicySetResponse(
                const SelectPolicySetRequestEvent& event, int result);

	/**
	 * Reply to the IPC Manager, informing it about the result of a
         * pluginLoad operation
	 * @param event the event that trigered the operation
	 * @param result the result of the operation (0 successful)
	 * @throws PluginLoadException
	 */
	void pluginLoadResponse(const PluginLoadRequestEvent& event,
                                int result);

        /**
         * Forward to the IPC Manager a CDAP request message
         * @param event Seqnum of the event that triggered the operation
         * @param The serialized CDAP message to forward
         * @throws FwdCDAPMsgException
         */
        void forwardCDAPRequest(unsigned sequenceNumber,
                                 const ser_obj_t& sermsg,
                                 int result);

	/**
	 * Forward to the IPC Manager a CDAP response message
	 * @param event Seqnum of the event that triggered the operation
	 * @param The serialized CDAP message to forward
	 * @throws FwdCDAPMsgException
	 */
	void forwardCDAPResponse(unsigned sequenceNumber,
				 const ser_obj_t& sermsg,
				 int result);

	/**
	 * Send the IPCM a report from a scan of the media (radio, etc), informing
	 * it what radio DIFs are available via what access point IPCPs (addresses)
	 * and what signal power levels
	 */
	void sendMediaReport(const MediaReport& report);

	/**
	 * Tell librina that an internal flow has been allocated, so that it creates
	 * the flow structure and opens an I/O dev file descriptor associated to it
	 *
	 * @param information about the flow that has been allocated
	 * @return the file descriptor
	 */
	int internal_flow_allocated(const rina::FlowInformation& flow_info);

	/**
	 * Tell librina that an internal flow has been deallocated, so that it deletes
	 * the flow structure and closes the I/O dev file descriptor associated to it
	 */
	void internal_flow_deallocated(int port_id);

private:
	void send_base_resp_msg(irati_msg_t msg_t, unsigned int seq_num, int result);
	void send_fwd_msg(irati_msg_t msg_t, unsigned int sequenceNumber,
			  const ser_obj_t& sermsg, int result);
};

/**
 * Make Extended IPC Manager singleton
 */
extern Singleton<ExtendedIPCManager> extendedIPCManager;

class DTPStatistics {
public:
	DTPStatistics() : tx_bytes(0), tx_pdus(0),
		rx_bytes(0), rx_pdus(0), drop_pdus(0), err_pdus(0) {};

	unsigned long tx_bytes;
	unsigned int tx_pdus;
	unsigned long rx_bytes;
	unsigned int rx_pdus;
	unsigned int drop_pdus;
	unsigned int err_pdus;
};

/**
 * Represents the data to create an EFCP connection
 */
class Connection {
public:
        /** The port-id to which the connection is bound */
        int portId;

        /** The address of the IPC Process at the source of the conection */
        unsigned int sourceAddress;

        /** The address of the IPC Process at the destination of the connection */
        unsigned int destAddress;

        /** The id of the QoS cube associated to the connection */
        unsigned int qosId;

        /** The source CEP-id */
        int sourceCepId;

        /** The destination CEP-id */
        int destCepId;

        /** The DTP connection policies */
        DTPConfig dtpConfig;

        /** The DTCP connection policies */
        DTCPConfig dtcpConfig;

        /** Connection statistics */
	DTPStatistics stats;

        /**
         * The id of the IPC Process using the flow supported by this
         * connection (0 if it is an application that is not an IPC Process)
         */
        unsigned short flowUserIpcProcessId;

        Connection();
#ifndef SWIG
        unsigned int getDestAddress() const;
        void setDestAddress(unsigned int destAddress);
        int getPortId() const;
        void setPortId(int portId);
        unsigned int getQosId() const;
        void setQosId(unsigned int qosId);
        unsigned int getSourceAddress() const;
        void setSourceAddress(unsigned int sourceAddress);
        int getDestCepId() const;
        void setDestCepId(int destCepId);
        unsigned short getFlowUserIpcProcessId() const;
        void setFlowUserIpcProcessId(unsigned short flowUserIpcProcessId);
        int getSourceCepId() const;
        void setSourceCepId(int sourceCepId);
        const DTPConfig& getDTPConfig() const;
        void setDTPConfig(const DTPConfig& dtpConfig);
        const DTCPConfig& getDTCPConfig() const;
        void setDTCPConfig(const DTCPConfig& dtcpConfig);
#endif
        const std::string toString();
};

struct DTPInformation {
	unsigned int src_cep_id;
	unsigned int dest_cep_id;
	unsigned int src_address;
	unsigned int dest_address;
	unsigned int qos_id;
	unsigned int port_id;
	DTPConfig dtp_config;
	DTPStatistics stats;

	DTPInformation();
	DTPInformation(Connection * connection);
	const std::string toString() const;
};

struct IPCPNameAddresses {
	/* The name of the IPCP */
	std::string name;

	/* The active addresses of the IPCP */
	std::list<unsigned int> addresses;

	std::string get_addresses_as_string() const;
};

struct NHopAltList {
	/** Next hop and its alternates */
	std::list<IPCPNameAddresses> alts;

	NHopAltList() { }
	NHopAltList(const IPCPNameAddresses& x) { alts.push_back(x); }
	void add_alt(const IPCPNameAddresses& x) { alts.push_back(x); }
};

/// Models an entry of the routing table
class RoutingTableEntry {
public:
	/** The destination IPCP name and addresses */
	IPCPNameAddresses destination;

	/** The qos-id */
	unsigned int qosId;

	/** The cost */
	unsigned int cost;

	/** The next hop names and addresses */
	std::list<NHopAltList> nextHopNames;

	RoutingTableEntry();
	const std::string getKey() const;
};

struct PortIdAltlist {
	std::list<unsigned int> alts;

	PortIdAltlist();
	PortIdAltlist(unsigned int pid);
	static void from_c_pid_list(PortIdAltlist & pi,
				    struct port_id_altlist * pia);
	struct port_id_altlist * to_c_pid_list(void) const;

	void add_alt(unsigned int pid);
};

/**
 * Models an entry in the PDU Forwarding Table
 */
class PDUForwardingTableEntry {
public:
        /** The destination address */
        unsigned int address;

        /** The qos-id */
        unsigned int qosId;

	/** The cost */
	unsigned int cost;

        /** The N-1 portid */
        std::list<PortIdAltlist> portIdAltlists;

        PDUForwardingTableEntry();
        static void from_c_pff_entry(PDUForwardingTableEntry & pf,
        			     struct mod_pff_entry * pff);
        struct mod_pff_entry * to_c_pff_entry(void) const;

        bool operator==(const PDUForwardingTableEntry &other) const;
        bool operator!=(const PDUForwardingTableEntry &other) const;
#ifndef SWIG
        unsigned int getAddress() const;
        void setAddress(unsigned int address);
        const std::list<PortIdAltlist> getPortIdAltlists() const;
        void setPortIdAltlists(const std::list<PortIdAltlist>& portIds);
        unsigned int getQosId() const;
        void setQosId(unsigned int qosId);
#endif
        const std::string toString();
        const std::string getKey() const;
};

/**
 * Response of the Kernel IPC Process, reporting on the
 * number of entries in the PDU Forwarding Table for this
 * IPC Process
 */
class DumpFTResponseEvent: public IPCEvent {
public:
        /** The PDU Forwarding Table entries*/
        std::list<PDUForwardingTableEntry> entries;

        /** Result of the operation, 0 success */
        int result;

        DumpFTResponseEvent(const std::list<PDUForwardingTableEntry>& entries,
                        int result, unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
#ifndef SWIG
        const std::list<PDUForwardingTableEntry>& getEntries() const;
        int getResult() const;
#endif
};

class UpdateCryptoStateResponseEvent: public IPCEvent {
public:
	UpdateCryptoStateResponseEvent(int res,
                        	       int port_id,
                        	       unsigned int sequenceNumber,
				       unsigned int ctrl_p, unsigned short ipcp_id);

        // The N-1 port-id where crypto state was updated
        int port_id;

        // Result of the operation, 0 success
        int result;
};

class AllocatePortResponseEvent: public IPCEvent {
public:
	AllocatePortResponseEvent(int res,
                        	  int port_id,
                        	  unsigned int sequenceNumber,
				  unsigned int ctrl_p, unsigned short ipcp_id);

        // The N-1 port-id allocated
        int port_id;

        // Result of the operation, 0 success
        int result;
};

class DeallocatePortResponseEvent: public IPCEvent {
public:
	DeallocatePortResponseEvent(int res,
                        	    int port_id,
				    unsigned int sequenceNumber,
				    unsigned int ctrl_p, unsigned short ipcp_id);

        // The N-1 port-id deallocated
        int port_id;

        // Result of the operation, 0 success
        int result;
};

class WriteMgmtSDUResponseEvent: public IPCEvent {
public:
	WriteMgmtSDUResponseEvent(int res,
				  unsigned int sequenceNumber,
				  unsigned int ctrl_p, unsigned short ipcp_id);

        // Result of the operation, 0 success
        int result;
};

class ReadMgmtSDUResponseEvent: public IPCEvent {
public:
	ReadMgmtSDUResponseEvent(int res,
				 struct buffer * buf,
				 unsigned int port_id,
				 unsigned int sequenceNumber,
				 unsigned int ctrl_p, unsigned short ipcp_id);

        // Result of the operation, 0 success
        int result;
        rina::ser_obj_t msg;
        unsigned int port_id;
};

/**
 * FIXME: Quick hack to get multiple parameters back
 */
class ReadManagementSDUResult {
public:
        int bytesRead;
        int portId;

        ReadManagementSDUResult();
#ifndef SWIG
        int getBytesRead() const;
        void setBytesRead(int bytesRead);
        int getPortId() const;
        void setPortId(int portId);
#endif
};

/**
 * Abstraction of the data transfer and data transfer control parts of the
 * IPC Process, which are implemented in the Kernel. This class allows the
 * IPC Process Daemon to communicate with its components in the kernel
 */
class KernelIPCProcess {
public:
        /** The ID of the IPC Process */
        unsigned short ipcProcessId;

        void setIPCProcessId(unsigned short ipcProcessId);
        unsigned short getIPCProcessId() const;

        /**
         * Invoked by the IPC Process Deamon to allow the kernel components
         * to update its internal configuration based on the DIF the IPC
         * Process has been assigned to.
         *
         * @param difInformation The information of the DIF (name, type,
         * configuration)
         * @throws AssignToDIFException if an error happens during the process
         * @returns the handle to the response message
         */
        unsigned int assignToDIF(const DIFInformation& difInformation);

        /**
         * Invoked by the IPC Process Daemon to modify the configuration of
         * the kernel components of the IPC Process.
         *
         * @param difConfiguration The configuration of the DIF
         * @throws UpdateDIFConfigurationException if an error happens during
         * the process
         * @returns the handle to the response message
         */
        unsigned int updateDIFConfiguration(
                        const DIFConfiguration& difConfiguration);

        /**
         * Invoked by the IPC Process Daemon to request the creation of an
         * EFCP connection to the kernel components of the IPC Process
         *
         * @param connection
         * @throws CreateConnectionException
         * @return the handle to the response message
         */
        unsigned int createConnection(const Connection& connection);

        /**
         * Invoked by the IPC Process Daemon to request an update of an
         * EFCP connection to the kernel components of the IPC Process
         *
         * @param connection
         * @throws UpdateConnectionException
         * @return the handle to the response message
         */
        unsigned int updateConnection(const Connection& connection);

        /**
         * Invoked by the IPC Process Daemon to request a modification of an
         * EFCP connection to the kernel components of the IPC Process
         * (right now only src (=local) and destination (=remote) addresses)
         *
         * @param connection
         * @throws UpdateConnectionException
         * @return the handle to the response message
         */
        void modify_connection(const Connection& connection);

        /**
         * Invoked by the IPC Process Daemon to request the creation of an
         * EFCP connection to the kernel components of the IPC Process
         * (receiving side of the Flow allocation procedure)
         *
         * @param connection
         * @throws CreateConnectionException
         * @return the handle to the response message
         */
        unsigned int createConnectionArrived(const Connection& connection);

        /**
         * Invoked by the IPC Process Daemon to request the destruction of an
         * EFCP connection to the kernel components of the IPC Process
         *
         * @param connection
         * @throws DestroyConnectionException
         * @return the handle to the response message
         */
        unsigned int destroyConnection(const Connection& connection);

        /**
         * Modify the entries of the PDU forwarding table
         * @param entries to be modified
         * @param mode 0 add, 1 remove, 2 flush and add
         */
        void modifyPDUForwardingTableEntries(const std::list<PDUForwardingTableEntry *>& entries,
                        int mode);

        /**
         * Request the Kernel IPC Process to provide a list of
         * all the entries in the PDU Forwarding table
         * @return a handle to the response event
         * @throws PDUForwardingTabeException if something goes wrong
         */
        unsigned int dumptPDUFT();

        /// Request the kernel to update the state of cryptographic protection policies
        unsigned int updateCryptoState(const CryptoState& state);

        /// Inform the kernel that the IPCP address has changed and trigger
        /// the address change procedure: i) Accept PDUs with new address,
        /// ii) start using it after a timeout and iii) deprecate old address
        /// after another timeout
        unsigned int changeAddress(unsigned int new_address,
                		   unsigned int old_address,
        			   unsigned int use_new_t,
        			   unsigned int deprecate_old_t);

        /**
         * Request the Kernel IPC Process to modify a policy-set-related
         * parameter.
         * @param path The identificator of the component/policy-set to
         *             be addressed
         * @param name The name of the parameter to be modified
         * @param value The value to set the parameter to
         * @return a handle to the response event
         * @throws SetPolicySetParamException if something goes wrong
         */
        unsigned int setPolicySetParam(const std::string& path,
                                       const std::string& name,
                                       const std::string& value);

        /**
         * Request the Kernel IPC Process to select a policy-set
         * for an IPC process component.
         * @param path The identificator of the component to
         *             be addressed
         * @param name The name of the policy-set to be selected
         * @return a handle to the response event
         * @throws SelectPolicySetException if something goes wrong
         */
        unsigned int selectPolicySet(const std::string& path,
                                     const std::string& name);

        /**
         * Requests the kernel to write a management SDU to the
         * N-1 portId specified
         *
         * @param sdu A buffer that contains the SDU data
         * @param size The size of the SDU data, in bytes
         * @param portId The N-1 portId where the data has to be written to
         * @throws WriteSDUException
         */
        unsigned int writeMgmgtSDUToPortId(void * sdu, int size, unsigned int portId);
};

/**
 * Make Kernel IPC Process singleton
 */
extern Singleton<KernelIPCProcess> kernelIPCProcess;

/// An entry of the directory forwarding table
class DirectoryForwardingTableEntry {
public:
	DirectoryForwardingTableEntry();
	bool operator==(const DirectoryForwardingTableEntry &object);
#ifndef SWIG
	ApplicationProcessNamingInformation get_ap_naming_info() const;
	void set_ap_naming_info(const ApplicationProcessNamingInformation& ap_naming_info);
	unsigned int get_address() const;
	void set_address(unsigned int address);
	long get_seqnum() const;
	void set_seqnum(unsigned int seqnum);
#endif

	/**
	 * Returns a key identifying this entry
	 * @return
	 */
	const std::string getKey() const;
	std::string toString();

	/// The name of the application process
	ApplicationProcessNamingInformation ap_naming_info_;

	/// The address of the IPC process it is currently attached to
	unsigned int address_;

	/// A sequence number for this entry
	unsigned int seqnum_;
};

/// Defines a whatevercast name (or a name of a set of names).
/// In traditional architectures, sets that returned all members were called multicast; while
/// sets that returned one member were called anycast.  It is not clear what sets that returned
/// something in between were called.  With the more general definition here, these
/// distinctions are unnecessary.
class WhatevercastName {
public:
	bool operator==(const WhatevercastName &other);
	std::string toString();

	/// The name
	std::string name_;

	/// The members of the set
	std::list<std::string> set_members_;

	/// The rule to select one or more members from the set
	std::string rule_;
};

/**
 * The IPC Manager requests the IPC Process to update the configuration
 * of the DIF he is currently a member of
 */
class ScanMediaRequestEvent: public IPCEvent {
public:
	ScanMediaRequestEvent(unsigned int sequenceNumber, unsigned int ctrl_p,
			      unsigned short ipcp_id);
};

}

#endif

#endif
