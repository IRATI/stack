package rina.ipcmanager.api;

import java.util.List;

import rina.aux.BlockingQueueWithSubscriptor;
import rina.cdap.api.CDAPSessionManagerFactory;
import rina.delimiting.api.DelimiterFactory;
import rina.encoding.api.EncoderFactory;
import rina.flowallocator.api.Flow;
import rina.flowallocator.api.FlowAllocatorInstance;
import rina.idd.api.InterDIFDirectoryFactory;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcservice.api.IPCException;

/**
 * The IPC Manager is the component of a DAF that manages the local IPC resources. In its current implementation it 
 * manages IPC Processes (creates/destroys them), and serves as a broker between applications and IPC Processes. Applications 
 * can use the RINA library to establish a connection to the IPC Manager and interact with the RINA stack.
 * @author eduardgrasa
 *
 */
public interface IPCManager {
	
	/**
	 * Invoked by the Flow Allocator Instance when it receives a new create flow request. The IPCManager has to call the destination application 
	 * to see wether it accepts or not the flow allocation request.
	 * @param flow
	 * @param ipcProcess
	 */
	public void createFlowRequestMessageReceived(Flow flow, FlowAllocatorInstance flowAllocatorInstance);
	
	/**
	 * Setter for the InterDIFDirectoryFactory
	 * @param iddFactory
	 */
	public void setInterDIFDirectoryFactory(InterDIFDirectoryFactory idd);
	
	/**
	 * Executes a runnable in a thread. The IPCManager maintains a single thread pool 
	 * for all the RINA prototype
	 * @param runnable
	 */
	public void execute(Runnable runnable);
	
	/* PORT_ID MANAGEMENT */
	/**
	 * Get a portId available for its use
	 * @return
	 */
	public int getAvailablePortId();
	
	/**
	 * Mark a portId as available to be reused
	 * @param portId
	 */
	public void freePortId(int portId);
	
	/* INCOMING AND OUTGOING FLOW QUEUES MANAGEMENT */
	/**
	 * Add an incoming and outgoing flow queues to support the flow identified by portId
	 * @param portId
	  * @param queueCapacity the capacity of the queue, it it is <= 0 an unlimited capacity queue will be used
	 * @throws IPCException if the portId is already in use
	 */
	public void addFlowQueues(int portId, int queueCapacity) throws IPCException;
	
	/**
	 * Remove the incoming and outgoing flow queues that support the flow identified by portId
	 * @param portId
	 */
	public void removeFlowQueues(int portId);
	
	/**
	 * Get the incoming queue that supports the flow identified by portId
	 * @param portId
	 * @return
	 * @throws IPCException if there is no incoming queue associated to portId
	 */
	public BlockingQueueWithSubscriptor<byte[]> getIncomingFlowQueue(int portId) throws IPCException;
	
	/**
	 * Get the outgoing queue that supports the flow identified by portId
	 * @param portId
	 * @return
	 * @throws IPCException if there is no outgoing queue associated to portId
	 */
	public BlockingQueueWithSubscriptor<byte[]> getOutgoingFlowQueue(int portId) throws IPCException;
	
	/* CONVENIENT OPERATIONS */
	public CDAPSessionManagerFactory getCDAPSessionManagerFactory();
	public DelimiterFactory getDelimiterFactory();
	public EncoderFactory getEncoderFactory();
	
	public List<IPCProcess> listIPCProcesses();
	public List<String> listDIFNames();
	public IPCProcess getIPCProcessBelongingToDIF(String difName);
}
