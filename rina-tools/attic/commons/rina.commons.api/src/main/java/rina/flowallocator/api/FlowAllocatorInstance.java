package rina.flowallocator.api;

import eu.irati.librina.AllocateFlowResponseEvent;
import eu.irati.librina.CreateConnectionResponseEvent;
import eu.irati.librina.CreateConnectionResultEvent;
import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.UpdateConnectionResponseEvent;
import rina.cdap.api.message.CDAPMessage;

/**
 * The interface between the FA and the FAI
 * @author elenitrouva
 *
 */
public interface FlowAllocatorInstance{
	
	/**
	 * Returns the portId associated to this Flow Allocator Instance
	 * @return
	 */
	public int getPortId();
	
	/**
	 * Return the Flow object associated to this Flow Allocator Instance
	 * @return
	 */
	public Flow getFlow();

	/**
	 * Called by the FA to forward an Allocate request to a FAI
	 * @param event
	 * @param applicationCallback the callback to invoke the application for allocateResponse and any other calls
	 * @throws IPCException
	 */
	public void submitAllocateRequest(FlowRequestEvent event) throws IPCException;
	
	public void processCreateConnectionResponseEvent(CreateConnectionResponseEvent event);
	
	/**
	 * Called by the Flow Allocator when an M_CREATE CDAP PDU with a Flow object 
	 * is received by the Flow Allocator
	 * @param flow
	 * @param portId the destination portid as decided by the Flow allocator
	 * @param requestMessate the CDAP request message
	 * @param underlyingPortId the port id to reply later on
	 */
	public void createFlowRequestMessageReceived(Flow flow, CDAPMessage requestMessage, int underlyingPortId);
	
	/**
	 * When the FAI gets a Allocate_Response from the destination application, it formulates a Create_Response 
	 * on the flow object requested.If the response was positive, the FAI will cause DTP and if required DTCP 
	 * instances to be created to support this allocation. A positive Create_Response Flow is sent to the 
	 * requesting FAI with the connection-endpoint-id and other information provided by the destination FAI. 
	 * The Create_Response is sent to requesting FAI with the necessary information reflecting the existing flow, 
	 * or an indication as to why the flow was refused.  
	 * If the response was negative, the FAI does any necessary housekeeping and terminates.
	 * @param AllocateFlowResponseEvent - the reply from the application
	 * @throws IPCException
	 */
	public void submitAllocateResponse(AllocateFlowResponseEvent event);
	
	public void processCreateConnectionResultEvent(CreateConnectionResultEvent event);
	
	public void processUpdateConnectionResponseEvent(UpdateConnectionResponseEvent event);
	
	/**
	 * When a deallocate primitive is invoked, it is passed to the FAI responsible for that port-id.  
	 * The FAI sends an M_DELETE request CDAP PDU on the Flow object referencing the destination port-id, deletes the local 
	 * binding between the Application and the DTP-instance and waits for a response.  (Note that 
	 * the DTP and DTCP if it exists will be deleted automatically after 2MPL)
	 * @param the flow deallocate request event
	 * @throws IPCException
	 */
	public void submitDeallocate(FlowDeallocateRequestEvent event);
	 
	/**
	 * When this PDU is received by the FAI with this port-id, the FAI invokes a Deallocate.deliver to notify the local Application, 
	 * deletes the binding between the Application and the local DTP-instance, and sends a Delete_Response indicating the result.
	 */
	public void deleteFlowRequestMessageReceived(CDAPMessage requestMessage, int underlyingPortId);
	
	public long getAllocateResponseMessageHandle();
}