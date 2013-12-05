package rina.flowallocator.api;

import eu.irati.librina.CreateConnectionResponseEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import rina.cdap.api.message.CDAPMessage;

/**
 * This interface must be implemented by the class that implements the flow allocator
 * @author eduardgrasa
 */
public interface FlowAllocator {
	
	/**
	 * The Flow Allocator is invoked when an Allocate_Request.submit is received.  The source Flow 
	 * Allocator determines if the request is well formed.  If not well-formed, an Allocate_Response.deliver 
	 * is invoked with the appropriate error code.  If the request is well-formed, a new instance of an 
	 * FlowAllocator is created and passed the parameters of this Allocate_Request to handle the allocation. 
	 * It is a matter of DIF policy (AllocateNoificationPolicy) whether an Allocate_Request.deliver is invoked 
	 * with a status of pending, or whether a response is withheld until an Allocate_Response can be delivered 
	 * with a status of success or failure.
	 * @param allocateRequest the characteristics of the flow to be allocated.
	 * to honour the request
	 */
	public void submitAllocateRequest(FlowRequestEvent flowRequestEvent);
	
	public void processCreateConnectionResponseEvent(CreateConnectionResponseEvent event);
	
	/**
	 * Forward the allocate response to the Flow Allocator Instance.
	 * @param portId the portId associated to the allocate response
	 * @param success successful or unsucessful allocate request
	 * @param reasonl
	 * @throws IPCException
	 */
	public void submitAllocateResponse(int portId, boolean success, String reason) throws IPCException;
	
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
}
