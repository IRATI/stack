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

#ifndef LIBRINA_IPC_MANAGER_H
#define LIBRINA_IPC_MANAGER_H

#include <map>

#include "librina-common.h"

namespace rina {

/**
 * Encapsulates the state and operations that can be performed over
 * a single IPC Process (besides creation/destruction)
 */
class IPCProcess {

	/** The identifier of the IPC Process, unique within the system */
	unsigned int id;

	/** The IPC Process type */
	DIFType type;

	/** The name of the IPC Process */
	ApplicationProcessNamingInformation name;

public:
	IPCProcess();
	IPCProcess(unsigned int id, DIFType type,
			const ApplicationProcessNamingInformation& name);
	unsigned int getId() const;
	DIFType getType() const;
	const ApplicationProcessNamingInformation& getName() const;

	/**
	 * Invoked by the IPC Manager to make an existing IPC Process a member of a
	 * DIF. Preconditions: the IPC Process must exist and must not be already a
	 * member of the DIF. Assigning an IPC Process to a DIF will initialize the
	 * IPC Process with all the information required to operate in the DIF (DIF
	 * name, data transfer constants, qos cubes, supported policies, address,
	 * credentials, etc). The operation will block until the IPC Process is
	 * assigned to the DIF or an error is returned.
	 *
	 * @param difConfiguration The configuration of the DIF
	 * @throws IPCException if an error happens during the process
	 */
	void assignToDIF(const DIFConfiguration& difConfiguration)
			throw (IPCException);

	/**
	 * Invoked by the IPC Manager to notify an IPC Process that he has been
	 * registered to the N-1 DIF designed by difName
	 *
	 * @param difName The name of the N-1 DIF where the IPC Process has been
	 * registered
	 * @throws IPCException if the IPC Process was already registered to
	 * that DIF
	 */
	void notifyRegistrationToSupportingDIF(
			const ApplicationProcessNamingInformation& difName)
					throw (IPCException);

	/**
	 * Invoked by the IPC Manager to notify an IPC Process that he has been
	 * unregistered from the N-1 DIF designed by difName
	 *
	 * @param difName The name of the N-1 DIF where the IPC Process has been
	 * unregistered
	 * @throws IPCException if the IPC Process was not registered to the DIF
	 */
	void notifyUnregistrationFromSupportingDIF(
			const ApplicationProcessNamingInformation& difName)
					throw (IPCException);

	/**
	 * Invoked by the IPC Manager to trigger the enrollment of an IPC Process
	 * in the system with a DIF, reachable through a certain N-1 DIF. The
	 * operation blocks until the IPC Process has successfully enrolled or an
	 * error occurs.
	 *
	 * @param difName The DIF that the IPC Process will try to join
	 * @param supportingDifName The supporting DIF used to contact the DIF to
	 * join
	 * @throws IPCException if the enrollment is unsuccessful
	 */
	void enroll(const ApplicationProcessNamingInformation& difName,
			const ApplicationProcessNamingInformation& supportinDifName)
					throw (IPCException);

	/**
	 * Invoked by the IPC Manager to force an IPC Process to deallocate all the
	 * N-1 flows to a neighbor IPC Process (for example, because it has been
	 * identified as a "rogue" member of the DIF). The operation blocks until the
	 * IPC Process has successfully completed or an error is returned.
	 *
	 * @param neighbor The neighbor to disconnect from
	 * @throws IPCException if an error occurs
	 */
	void disconnectFromNeighbor(
			const ApplicationProcessNamingInformation& neighbor)
					throw (IPCException);

	/**
	 * Invoked by the IPC Manager to register an application in a DIF through
	 * an IPC Process. The operation blocks until the IPC Process has
	 * successfully registered the application or an error occurs.
	 *
	 * @param applicationName The name of the application to be registered
	 * @throws IPCException if an error occurs
	 */
	void registerApplication(
			const ApplicationProcessNamingInformation& applicationName)
					throw (IPCException);

	/**
	 * Invoked by the IPC Manager to unregister an application in a DIF through
	 * an IPC Process. The operation blocks until the IPC Process has
	 * successfully unregistered the application or an error occurs.
	 *
	 * @param applicationName The name of the application to be unregistered
	 * @throws IPCException if an error occurs
	 */
	void unregisterApplication(
			const ApplicationProcessNamingInformation& applicationName)
					throw (IPCException);

	/**
	 * Invoked by the IPC Manager to request an IPC Process the allocation of a
	 * flow. Since all flow allocation requests go through the IPC Manager, and
	 * port_ids have to be unique within the whole system, the IPC Manager is
	 * the best candidate for managing the port-id space.
	 *
	 * @param flowRequest contains the names of source and destination
	 * applications, the portId as well as the characteristics required for the
	 * flow
	 * @throws ICPException if an error occurs
	 */
	void allocateFlow(const FlowRequest& flowRequest) throw (IPCException);

	/**
	 * TODO: Placeholder for the real query RIB method.
	 */
	void queryRIB();
};

/**
 * Provides functions to create, destroy and list IPC processes in the
 * system. This class is a singleton.
 */
class IPCProcessFactory {

	/** The current IPC Processes in the system*/
	std::map<int, IPCProcess*> ipcProcesses;

public:
	static const std::string unknown_ipc_process_error;

	/**
	 * Invoked by the IPC Manager to instantiate a new IPC Process in the
	 * system. The operation will block until the IPC Process is created or an
	 * error is returned.
	 *
	 * @param ipcProcessId An identifier that uniquely identifies an IPC Process
	 * within a aystem.
	 * @param ipcProcessName The naming information of the IPC Process
	 * @param difType The type of IPC Process (Normal or one of the shims)
	 * @return a pointer to a data structure holding the IPC Process state
	 * @throws IPCProcessException if an error happens during the creation
	 */
	IPCProcess * create(
			const ApplicationProcessNamingInformation& ipcProcessName,
			DIFType difType) throw (IPCException);

	/**
	 * Invoked by the IPC Manager to delete an IPC Process from the system. The
	 * operation will block until the IPC Process is destroyed or an error is
	 * returned.
	 *
	 * @param ipcProcessId The identifier of the IPC Process to be destroyed
	 * @throws IPCException if an error happens during the operation execution
	 */
	void destroy(unsigned int ipcProcessId) throw (IPCException);

	/**
	 * Returns a list to all the IPC Processes that are currently running in
	 * the system.
	 *
	 * @return vector<IPCProcess *> A list of the IPC Processes in the system
	 */
	std::vector<IPCProcess *> listIPCProcesses();
};

/**
 * Make IPC Process Factory singleton
 */
extern Singleton<IPCProcessFactory> ipcProcessFactory;

/**
 * Class to interact with application processes
 */
class ApplicationManager {
public:

	/**
	 * Invoked by the IPC Manager to notify an application about the  result of
	 * the registration in a DIF operation.
	 *
	 * @param transactionId identifies the request whose reply this operation
	 * is providing
	 * @param response The result of the registration operation
	 * @throws IPCException If an error occurs during the operation
	 */
	void applicationRegistered(unsigned int transactionId,
			const std::string& response) throw (IPCException);

	/**
	 * Invoked by the IPC Manager to notify an application about the  result of
	 * the unregistration in a DIF operation.
	 *
	 * @param transactionId identifies the request whose reply this operation
	 * is providing
	 * @param response The result of the unregistration operation
	 * @throws IPCException If an error occurs during the operation
	 */
	void applicationUnregistered(unsigned int transactionId,
			const std::string& response) throw (IPCException);

	/**
	 * Invoked by the IPC Manager to respond to the Application Process that
	 * requested a flow.
	 *
	 * @param transactionId identifies the request whose reply this operation
	 * is providing
	 * @param portId The port-id assigned to the flow, in case the operation
	 * was successful
	 * @param ipcProcessId Required so that the application process can contact
	 * the IPC Process to read/write the flow
	 * @param response The result of the unregistration operation
	 * @param difName The name of the DIF where the flow has been allocated
	 * @throws IPCException If an error occurs during the operation
	 */
	void flowAllocated(unsigned int transactionId, int portId,
			unsigned int ipcProcessPid, std::string response,
			const ApplicationProcessNamingInformation& difName)
					throw (IPCException);
};

/**
 * Make Application Manager singleton
 */
extern Singleton<ApplicationManager> applicationManager;

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
 * Event informing that an application has requested a flow to
 * a destination application
 */
class FlowAllocationRequestEvent: public IPCEvent {
	/** The source application name */
	ApplicationProcessNamingInformation sourceApplicationName;

	/** The destination application name */
	ApplicationProcessNamingInformation destinationApplicationName;

	/** The destination application name */
	FlowSpecification flowSpecification;

public:
	FlowAllocationRequestEvent(
			const ApplicationProcessNamingInformation& sourceName,
			const ApplicationProcessNamingInformation& destName,
			const FlowSpecification& flowSpec,
			unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getSourceApplicationName() const;
	const ApplicationProcessNamingInformation&
			getDestinationApplicationName() const;
	const FlowSpecification& getFlowSpecification() const;
};

}

#endif
