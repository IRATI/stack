package rina.resourceallocator.api;

import rina.ipcprocess.api.IPCProcessComponent;
import eu.irati.librina.AllocateFlowRequestResultEvent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.DeallocateFlowResponseEvent;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcessDIFRegistrationEvent;

/**
 * Manages the allocation and lifetime of N-1 Flows for 
 * an IPC Process 
 * @author eduardgrasa
 *
 */
public interface NMinus1FlowManager extends IPCProcessComponent {

	/**
	 * Allocate an N-1 Flow with the requested QoS to the destination 
	 * IPC Process 
	 * @param flowInformation contains the destination IPC Process and requested 
	 * QoS information
	 * @return handle to the flow request
	 */
	public long allocateNMinus1Flow(FlowInformation flowInformation) throws IPCException;
	
	/**
	 * Process the result of an allocate request event
	 * @param event
	 * @throws IPCException
	 */
	public void allocateRequestResult(AllocateFlowRequestResultEvent event) throws IPCException;
	
	/**
	 * Process a flow allocation request
	 * @param event 
	 * @throws IPCException if something goes wrong
	 */
	public void flowAllocationRequested(FlowRequestEvent event) throws IPCException;
	
	/**
	 * Deallocate the N-1 Flow identified by portId
	 * @param portId
	 * @throws IPCException if no N-1 Flow identified by portId exists
	 */
	public void deallocateNMinus1Flow(int portId) throws IPCException;
	
	/**
	 * Process the response of a flow deallocation request
	 * @throws IPCException
	 */
	public void deallocateFlowResponse(DeallocateFlowResponseEvent event) throws IPCException;
	
	/**
	 * A flow has been deallocated remotely, process
	 * @param portId
	 */
	public void flowDeallocatedRemotely(FlowDeallocatedEvent event) throws IPCException;
	
	/**
	 * Return the N-1 Flow descriptor associated to the flow identified by portId
	 * @param portId
	 * @return the N-1 Flow information
	 * @throws IPCException if no N-1 Flow identified by portId exists
	 */
	public FlowInformation getNMinus1FlowInformation(int portId) throws IPCException;
	
	/**
	 * Return the information of all the N-1 flows
	 * @return
	 * @throws IPCException
	 */
	public FlowInformation[] getAllNMinus1FlowsInformation();
	
	/**
	 * The IPC Process has been unregistered from or registered to an N-1 DIF
	 * @param evet
	 * @throws IPCException
	 */
	public void processRegistrationNotification(IPCProcessDIFRegistrationEvent evet) throws IPCException;
	
	/**
	 * True if the DIF name is a supoprting DIF, false otherwise
	 * @param difName
	 * @return
	 */
	public boolean isSupportingDIF(ApplicationProcessNamingInformation difName);
}
