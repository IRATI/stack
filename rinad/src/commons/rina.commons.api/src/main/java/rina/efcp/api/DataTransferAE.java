package rina.efcp.api;

import rina.aux.BlockingQueueWithSubscriptor;
import rina.flowallocator.api.Flow;
import rina.ipcprocess.api.IPCProcessComponent;
import rina.ipcservice.api.APService;
import rina.ipcservice.api.IPCException;

/**
 * Creates and manages the lifecycle of DataTrasnferAEInstances
 * @author eduardgrasa
 *
 */
public interface DataTransferAE extends IPCProcessComponent {
	
	/**
	 * Reserve a number of CEP ids (connection endpoint ids) that will be used during the lifetime
	 * of a flow (identified by portId).
	 * @param numberOfCEPIds The number of CEP ids to reserve
	 * @param portId the portId of the flow that will use these CEP ids
	 * @return
	 */
	public int[] reserveCEPIds(int numberOfCEPIds, int portId);
	
	/**
	 * Free the CEP ids (connection endpoint ids) associated to a flow identified by portId
	 * @param portId
	 */
	public void freeCEPIds(int portId);

	/**
	 * Initialize the state of a new local connection and bind it to the portId
	 * @param portId
	 * @param remotePortId
	 * @param applicationCallback the callback to the application, used to deliver the data
	 */
	public void createLocalConnectionAndBindToPortId(int portId, int remotePortId, APService applicationCallback);
	
	/**
	 * Initialize the state of a new connection, and bind it to the portId (all the SDUs delivered 
	 * to the portId by an application will be sent through this connection)
	 * @param flow the flow object, describing the service supported by this connection
	 */
	public void createConnectionAndBindToPortId(Flow flow);
	
	/**
	 * Destroy the instance of the data transfer AE associated to this connection endpoint Id
	 * @param connection endpoint id
	 */
	public void deleteConnection(long connectionEndpointId);
	
	/**
	 * Subscribe to the outgoing flow queue identified by portId
	 * @param portId the id of the incoming flow queue
	 */
	public void subscribeToFlow(int portId) throws IPCException;
	
	/**
	 * Get the incoming queue that supports the connection identified by connectionEndpointId
	 * @param connectionId
	 * @return
	 * @throws IPCException if there is no incoming queue associated to connectionEndpointId
	 */
	public BlockingQueueWithSubscriptor<PDU> getIncomingConnectionQueue(long connectionEndpointId) throws IPCException;
	
	/**
	 * Get the outgoing queue that supports the connection identified by connectionEndpointId
	 * @param connectionId
	 * @return
	 * @throws IPCException if there is no outgoing queue associated to connectionEndpointId
	 */
	public BlockingQueueWithSubscriptor<PDU> getOutgoingConnectionQueue(long connectionEndpointId) throws IPCException;
	
	/**
	 * Return the PDU Parser to be used by this IPC Process
	 * @return
	 */
	public PDUParser getPDUParser();
}
