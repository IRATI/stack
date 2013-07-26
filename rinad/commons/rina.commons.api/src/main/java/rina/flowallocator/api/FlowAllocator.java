package rina.flowallocator.api;

import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.api.IPCProcessComponent;
import rina.ipcservice.api.APService;
import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.IPCException;

/**
 * This interface must be implemented by the class that implements the flow allocator
 * @author eduardgrasa
 */
public interface FlowAllocator extends IPCProcessComponent{
	
	/**
	 * Returns the directory
	 * @return
	 */
	public DirectoryForwardingTable getDirectoryForwardingTable();

	/**
	 * The Flow Allocator is invoked when an Allocate_Request.submit is received.  The source Flow 
	 * Allocator determines if the request is well formed.  If not well-formed, an Allocate_Response.deliver 
	 * is invoked with the appropriate error code.  If the request is well-formed, a new instance of an 
	 * FlowAllocator is created and passed the parameters of this Allocate_Request to handle the allocation. 
	 * It is a matter of DIF policy (AllocateNoificationPolicy) whether an Allocate_Request.deliver is invoked 
	 * with a status of pending, or whether a response is withheld until an Allocate_Response can be delivered 
	 * with a status of success or failure.
	 * @param allocateRequest the characteristics of the flow to be allocated.
	 * @param APService the application process that requested the allocation of the flow
	 * @return the portId
	 * @throws IPCException if the request is not well formed or there are not enough resources
	 * to honour the request
	 */
	public int submitAllocateRequest(FlowService allocateRequest, APService application) throws IPCException;
	
	/**
	 * Forward the allocate response to the Flow Allocator Instance.
	 * @param portId the portId associated to the allocate response
	 * @param success successful or unsucessful allocate request
	 * @param reason
	 * @param application the callback to invoke the application for any call
	 * @throws IPCException
	 */
	public void submitAllocateResponse(int portId, boolean success, String reason, APService applicationCallback) throws IPCException;
	
	/**
	 * Forward the deallocate request to the Flow Allocator Instance.
	 * @param portId
	 * @throws IPCException
	 */
	public void submitDeallocate(int portId) throws IPCException;
	
	/**
	 * When an Flow Allocator receives a Create_Request PDU for a Flow object, it consults its local Directory to see if it has an entry.
	 * If there is an entry and the address is this IPC Process, it creates an FAI and passes the Create_request to it.If there is an 
	 * entry and the address is not this IPC Process, it forwards the Create_Request to the IPC Process designated by the address.
	 * @param cdapMessage
	 * @param underlyingPortId
	 */
	public void createFlowRequestMessageReceived(CDAPMessage cdapMessage, int underlyingPortId);
	
	/**
	 * Called by the flow allocator instance when it finishes to cleanup the state.
	 * @param portId
	 */
	public void removeFlowAllocatorInstance(int portId);
	
	/* Deal with local flows (flows between applications from the same system) */
	
	/**
	 * Called by the flow allocator instance when a request for a local flow is received
	 * @param flowService
	 * @throws IPCException
	 */
	public void receivedLocalFlowRequest(FlowService flowService) throws IPCException;
	
	/**
	 * Called by the flow allocator instance when a response for a local flow is received
	 * @param portId
	 * @param remotePortId
	 * @param result
	 * @param resultReason
	 * @throws IPCException
	 */
	public void receivedLocalFlowResponse(int portId, int remotePortId, boolean result, String resultReason) throws IPCException;
	
	/**
	 * Request to deallocate a local flow
	 * @param portId
	 * @throws IPCException
	 */
	public void receivedDeallocateLocalFlowRequest(int portId) throws IPCException;
}
