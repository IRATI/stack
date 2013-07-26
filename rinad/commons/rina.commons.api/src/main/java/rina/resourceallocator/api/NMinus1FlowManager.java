package rina.resourceallocator.api;

import java.util.List;

import rina.configuration.SDUProtectionOption;
import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.IPCException;

/**
 * Manages the allocation and lifetime of N-1 Flows for 
 * an IPC Process 
 * @author eduardgrasa
 *
 */
public interface NMinus1FlowManager {

	/**
	 * Allocate an N-1 Flow with the requested QoS to the destination 
	 * IPC Process 
	 * @param flowService contains the destination IPC Process and requested QoS information
	 */
	public void allocateNMinus1Flow(FlowService flowService);
	
	/**
	 * Deallocate the N-1 Flow identified by portId
	 * @param portId
	 * @throws IPCException if no N-1 Flow identified by portId exists
	 */
	public void deallocateNMinus1Flow(int portId) throws IPCException;
	
	/**
	 * Return the N-1 Flow descriptor associated to the flow identified by portId
	 * @param portId
	 * @return the N-1 Flow descriptor
	 * @throws IPCException if no N-1 Flow identified by portId exists
	 */
	public NMinus1FlowDescriptor getNMinus1FlowDescriptor(int portId) throws IPCException;
	
	/**
	 * Register the IPC Process to one or more N-1 DIFs
	 * @param difName The N-1 DIF where this IPC Process will register
	 * @throws IPCException
	 */
	public void registerIPCProcess(String difName) throws IPCException;
	
	/**
	 * Set the list of preferred SDU protection options as specified by management
	 * @param sduProtectionOptions
	 */
	public void setSDUProtecionOptions(List<SDUProtectionOption> sduProtectionOptions);
	
	/**
	 * Return the type of SDU Protection module to be used for the DIF called "nminus1DIFName"
	 * (return the NULL type if no entries for "nminus1DIFName" are found)
	 * @param nMinus1DIFName
	 * @return
	 */
	public String getSDUProtectionOption(String nMinus1DIFName);
}
