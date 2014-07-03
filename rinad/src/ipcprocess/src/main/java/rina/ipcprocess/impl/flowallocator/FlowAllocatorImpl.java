package rina.ipcprocess.impl.flowallocator;

import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowResponseEvent;
import eu.irati.librina.CreateConnectionResponseEvent;
import eu.irati.librina.CreateConnectionResultEvent;
import eu.irati.librina.ExtendedIPCManagerSingleton;
import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.UpdateConnectionResponseEvent;
import eu.irati.librina.rina;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.api.Encoder;
import rina.flowallocator.api.Flow;
import rina.flowallocator.api.FlowAllocator;
import rina.flowallocator.api.FlowAllocatorInstance;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.flowallocator.ribobjects.FlowSetRIBObject;
import rina.ipcprocess.impl.flowallocator.ribobjects.QoSCubeSetRIBObject;
import rina.ipcprocess.impl.flowallocator.validation.AllocateRequestValidator;
import rina.registrationmanager.api.RegistrationManager;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;

/** 
 * Implements the Flow Allocator
 */
public class FlowAllocatorImpl implements FlowAllocator{
	
	private static final Log log = LogFactory.getLog(FlowAllocatorImpl.class);

	/**
	 * Flow allocator instances, each one associated to a port_id
	 */
	private ConcurrentMap<Integer, FlowAllocatorInstance> flowAllocatorInstances = null;
	
	/**
	 * Validates allocate requests
	 */
	private AllocateRequestValidator allocateRequestValidator = null;
	
	/**
	 * The RIB Daemon
	 */
	private RIBDaemon ribDaemon = null;
	
	/**
	 * The Encoder
	 */
	private Encoder encoder = null;
	
	private IPCProcess ipcProcess = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private ExtendedIPCManagerSingleton ipcManager = null;
	
	private RegistrationManager registrationManager = null;
	
	public FlowAllocatorImpl(){
		allocateRequestValidator = new AllocateRequestValidator();
		flowAllocatorInstances = new ConcurrentHashMap<Integer, FlowAllocatorInstance>();
	}

	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		ipcManager = rina.getExtendedIPCManager();
		ribDaemon = ipcProcess.getRIBDaemon();
		encoder = ipcProcess.getEncoder();
		cdapSessionManager = ipcProcess.getCDAPSessionManager();
		registrationManager = ipcProcess.getRegistrationManager();
		populateRIB(ipcProcess);
	}
	
	/**
	 * Called by the flow allocator instance when it finishes to cleanup the state.
	 * @param portId
	 */
	public void removeFlowAllocatorInstance(int portId){
		this.flowAllocatorInstances.remove(new Integer(portId));
	}
	
	private void populateRIB(IPCProcess ipcProcess){
		try{
			RIBObject ribObject = new FlowSetRIBObject(ipcProcess, this);
			ribDaemon.addRIBObject(ribObject);
		    ribObject = new QoSCubeSetRIBObject(ipcProcess);
			ribDaemon.addRIBObject(ribObject);
		}catch(RIBDaemonException ex){
			ex.printStackTrace();
			log.error("Could not subscribe to RIB Daemon:" +ex.getMessage());
		}
	}
	
	public Map<Integer, FlowAllocatorInstance> getFlowAllocatorInstances(){
		return this.flowAllocatorInstances;
	}
	
	/**
	 * When an Flow Allocator receives a Create_Request PDU for a Flow object, it consults its local Directory to see if it has an entry.
	 * If there is an entry and the address is this IPC Process, it creates an FAI and passes the Create_request to it.If there is an 
	 * entry and the address is not this IPC Process, it forwards the Create_Request to the IPC Process designated by the address.
	 * @param cdapMessage
	 * @param underlyingPortId
	 */
	public synchronized void createFlowRequestMessageReceived(CDAPMessage cdapMessage, int underlyingPortId){
		Flow flow = null;
		long myAddress = 0;
		int portId = 0;
		
		try{
			flow = (Flow) encoder.decode(cdapMessage.getObjValue().getByteval(), Flow.class);
		}catch (Exception ex){
			ex.printStackTrace();
			return;
		}
	
		long address = registrationManager.getAddress(flow.getDestinationNamingInfo());
		myAddress = ipcProcess.getAddress().longValue();
		
		if (address == 0){
			log.error("The directory forwarding table returned no entries when looking up " 
						+ flow.getDestinationNamingInfo().toString());
			return;
		}
		
		if (address == myAddress){
			//There is an entry and the address is this IPC Process, create a FAI, extract the Flow object from the CDAP message and
			//call the FAI
			try {
				portId = ipcManager.allocatePortId(flow.getDestinationNamingInfo());
			}catch (Exception ex) {
				log.error("Problems requesting an available port-id: "+ex.getMessage() 
						+ " Ignoring the Flow allocation request");
				return;
			}
			
			log.debug("The destination application process is reachable through me. Assigning the local portId " 
						+portId+" to the flow allocation.");
			FlowAllocatorInstance flowAllocatorInstance = 
					new FlowAllocatorInstanceImpl(ipcProcess, this, cdapSessionManager, portId);
			flowAllocatorInstances.put(new Integer(new Integer(portId)), flowAllocatorInstance);
			flowAllocatorInstance.createFlowRequestMessageReceived(flow, cdapMessage, underlyingPortId);
			return;
		}
		
		//The address is not this IPC process, forward the CDAP message to that address increment the hop count of the Flow object
		//extract the flow object from the CDAP message
		flow.setHopCount(flow.getHopCount() - 1);
		if (flow.getHopCount()  <= 0){
			//TODO send negative create Flow response CDAP message to the source IPC process, specifying that the application process
			//could not be found before the hop count expired
		}

		try{
			int destinationPortId = Utils.mapAddressToPortId(address, ipcProcess);
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(encoder.encode(flow));
			cdapMessage.setObjValue(objectValue);
			ribDaemon.sendMessage(cdapMessage, destinationPortId, null);
		}catch(Exception ex){
			//Error that has to be fixed, we cannot continue, log it and return
			log.error("Fatal error when serializing a Flow object. " +ex.getMessage());
			return;
		}
	}
	
	private void replyToIPCManager(FlowRequestEvent event, int result) {
		try{
			ipcManager.allocateFlowRequestResult(event, result);
		} catch(Exception ex) {
			log.error("Problems communicating with the IPC Manager Daemon: " + ex.getMessage());
		}
	}
	
	/**
	 * Validate the request, create a Flow Allocator Instance and forward it the request for further processing
	 * @param flowService
	 * @param applicationCallback the callback to invoke the application for allocateResponse and any other calls
	 * @throws IPCException
	 */
	public synchronized void submitAllocateRequest(FlowRequestEvent event) {
		log.debug("Local application invoked allocate request: "+event.toString());
		int portId = 0;
		
		try{
			allocateRequestValidator.validateAllocateRequest(event);
		} catch(Exception ex) {
			log.error("Problems validating Flow Allocation Request: "+ex.getMessage());
			replyToIPCManager(event, -1);
		}
		
		try {
			portId = ipcManager.allocatePortId(event.getLocalApplicationName());
			log.debug("Got assigned port-id " + portId);
		} catch (Exception ex) {
			log.error("Problems requesting an available port-id to the Kernel IPC Maanager: " 
					+ ex.getMessage());
			replyToIPCManager(event, -1);
		}
		
		event.setPortId(portId);
		FlowAllocatorInstance flowAllocatorInstance = 
				new FlowAllocatorInstanceImpl(ipcProcess, this, cdapSessionManager, portId);
		flowAllocatorInstances.put(portId, flowAllocatorInstance);
		
		try {
			flowAllocatorInstance.submitAllocateRequest(event);
		} catch(Exception ex) {
			log.error("Problems allocating flow: "+ex.getMessage());
			
			flowAllocatorInstances.remove(portId);
			
			try {
				ipcManager.deallocatePortId(portId);
			} catch (Exception e) {
				log.error("Problems releasing port-id +portId");
			}
			
			replyToIPCManager(event, -1);
		}
	}
	
	public synchronized void processCreateConnectionResponseEvent(CreateConnectionResponseEvent event) {
		FlowAllocatorInstance flowAllocatorInstance = getFlowAllocatorInstance(event.getPortId());
		if (flowAllocatorInstance == null) {
			log.error("Received a crete connection response event associated to " +
						"unknown port-id: "+ event.getPortId());
			return;
		}
		
		flowAllocatorInstance.processCreateConnectionResponseEvent(event);
	}

	/**
	 * Forward the call to the right FlowAllocator Instance. If the application process 
	 * rejected the flow request, remove the flow allocator instance from the list of 
	 * active flow allocator instances
	 * @param portId
	 * @param success
	 * @param reason
	 */
	public synchronized void submitAllocateResponse(AllocateFlowResponseEvent event) {
		log.debug("Local application invoked allocate response seq num "
				+event.getSequenceNumber() +" with result "+event.getResult());
		FlowAllocatorInstance flowAllocatorInstance = null;

		log.debug("Looking for FAI associated to seq num " + event.getSequenceNumber());
		Iterator<Entry<Integer, FlowAllocatorInstance>> iterator = flowAllocatorInstances.entrySet().iterator();
		Entry<Integer, FlowAllocatorInstance> entry = null;
		while (iterator.hasNext()) {
			entry = iterator.next();
			if (entry.getValue().getAllocateResponseMessageHandle() == event.getSequenceNumber()) {
				flowAllocatorInstance = entry.getValue();
				break;
			}
		}
		
		if (flowAllocatorInstance == null){
			log.error("Problems looking for FAI with handle " + event.getSequenceNumber());
			return;
		}

		flowAllocatorInstance.submitAllocateResponse(event);
	}
	
	public synchronized void processCreateConnectionResultEvent(CreateConnectionResultEvent event) {
		FlowAllocatorInstance flowAllocatorInstance = null;

		log.debug("Looking for FAI associated to port-id: "+event.getPortId());
		flowAllocatorInstance = getFlowAllocatorInstance(event.getPortId());
		if (flowAllocatorInstance == null){
			log.error("Problems looking for FAI at portId " + event.getPortId());
			try{
				ipcManager.deallocatePortId(event.getPortId());
			} catch (Exception e) {
				log.error("Prpblems requesting IPC Manager to deallocate port id "
						+ event.getPortId() + ". "+e.getMessage());
			}
			
			return;
		}

		flowAllocatorInstance.processCreateConnectionResultEvent(event);
	}
	
	public void processUpdateConnectionResponseEvent(UpdateConnectionResponseEvent event) {
		FlowAllocatorInstance flowAllocatorInstance = null;

		flowAllocatorInstance = getFlowAllocatorInstance(event.getPortId());
		if (flowAllocatorInstance == null){
			log.error("Problems looking for FAI at portId " + event.getPortId());
			try{
				ipcManager.deallocatePortId(event.getPortId());
			} catch (Exception e) {
				log.error("Problems requesting IPC Manager to deallocate port id "
						+ event.getPortId() + ". "+e.getMessage());
			}
			
			return;
		}

		flowAllocatorInstance.processUpdateConnectionResponseEvent(event);
	}

	/**
	 * Forward the deallocate request to the Flow Allocator Instance.
	 * @param portId
	 */
	public void submitDeallocate(FlowDeallocateRequestEvent event){
		FlowAllocatorInstance flowAllocatorInstance = null;

		flowAllocatorInstance = getFlowAllocatorInstance(event.getPortId());
		if (flowAllocatorInstance == null){
			log.error("Problems looking for FAI at portId " + event.getPortId());
			try{
				ipcManager.deallocatePortId(event.getPortId());
			} catch (Exception e) {
				log.error("Prpblems requesting IPC Manager to deallocate port id "
						+ event.getPortId() + ". "+e.getMessage());
			}
			
			try{
				ipcManager.notifyflowDeallocated(event, -1);
			}catch(Exception ex){
				log.error("Error communicating with the IPC Manager: "+ex.getMessage());
			}
			
			return;
		}
		
		flowAllocatorInstance.submitDeallocate(event);
		
		try{
			ipcManager.notifyflowDeallocated(event, 0);
		}catch(Exception ex){
			log.error("Error communicating with the IPC Manager: "+ex.getMessage());
		}
	}
	
	/**
	 * Returns the flow allocator instance that manages the flow identified by portId
	 * @param portId
	 * @return
	 */
	private FlowAllocatorInstance getFlowAllocatorInstance(int portId) {
		FlowAllocatorInstance flowAllocatorInstance = 
				flowAllocatorInstances.get(new Integer(portId));
		
		return flowAllocatorInstance;
	}
}