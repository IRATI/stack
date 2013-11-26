package rina.ipcprocess.impl.flowallocator;

import java.util.Map;
import java.util.Timer;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.encoding.api.Encoder;
import rina.flowallocator.api.FlowAllocator;
import rina.flowallocator.api.FlowAllocatorInstance;
import rina.ipcprocess.impl.IPCProcess;
import rina.ipcprocess.impl.flowallocator.ribobjects.QoSCubeSetRIBObject;
import rina.ipcprocess.impl.flowallocator.validation.AllocateRequestValidator;
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
	
	private Timer timer = null;
	
	public FlowAllocatorImpl(){
		allocateRequestValidator = new AllocateRequestValidator();
		flowAllocatorInstances = new ConcurrentHashMap<Integer, FlowAllocatorInstance>();
		timer = new Timer();
	}

	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		ribDaemon = ipcProcess.getRIBDaemon();
		encoder = ipcProcess.getEncoder();
		cdapSessionManager = ipcProcess.getCDAPSessionManager();
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
			//RIBObject ribObject = new FlowSetRIBObject(this, ipcProcess);
			//ribDaemon.addRIBObject(ribObject);
		    RIBObject ribObject = new QoSCubeSetRIBObject();
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
	public void createFlowRequestMessageReceived(CDAPMessage cdapMessage, int underlyingPortId){
	/*	Flow flow = null;
		long myAddress = 0;
		int portId = 0;
		
		try{
			flow = (Flow) encoder.decode(cdapMessage.getObjValue().getByteval(), Flow.class);
		}catch (Exception ex){
			ex.printStackTrace();
			return;
		}
	
		long address = directoryForwardingTable.getAddress(flow.getDestinationNamingInfo());
		myAddress = this.getIPCProcess().getAddress().longValue();
		
		if (address == 0){
			//error, the table should have at least returned a default IPC process address to continue looking for the application process
			log.error("The directory forwarding table returned no entries when looking up " + flow.getDestinationNamingInfo().toString());
			return;
		}
		
		if (address == myAddress){
			//There is an entry and the address is this IPC Process, create a FAI, extract the Flow object from the CDAP message and
			//call the FAI
			APService applicationCallback = directoryForwardingTable.getLocalApplicationCallback(flow.getDestinationNamingInfo());
			if (applicationCallback == null){
				log.error("Ignoring the flow request because I could not find the callback for application " 
						+ flow.getDestinationNamingInfo().toString());
				return;
			}
			
			portId = this.getIPCProcess().getIPCManager().getAvailablePortId();
			log.debug("The destination application process is reachable through me. Assigning the local portId "+portId+" to the flow allocation.");
			FlowAllocatorInstance flowAllocatorInstance = new FlowAllocatorInstanceImpl(this.getIPCProcess(), this, cdapSessionManager, portId);
			flowAllocatorInstance.setApplicationCallback(applicationCallback);
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
			int destinationPortId = Utils.mapAddressToPortId(address, this.getIPCProcess());
			RIBDaemon ribDaemon = (RIBDaemon) getIPCProcess().getIPCProcessComponent(BaseRIBDaemon.getComponentName());
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(encoder.encode(flow));
			cdapMessage.setObjValue(objectValue);
			ribDaemon.sendMessage(cdapMessage, destinationPortId, null);
		}catch(Exception ex){
			//Error that has to be fixed, we cannot continue, log it and return
			log.error("Fatal error when serializing a Flow object. " +ex.getMessage());
			return;
		}*/
	}
	
	/**
	 * Called by the flow allocator instance when a request for a local flow is received
	 * @param flowService
	 * @throws IPCException
	 */
	public void receivedLocalFlowRequest(FlowRequestEvent flowRequestEvent) throws IPCException{
		/*int portId = this.getIPCProcess().getIPCManager().getAvailablePortId();
		FlowAllocatorInstance flowAllocatorInstance = new FlowAllocatorInstanceImpl(this.getIPCProcess(), this, portId);
		FlowService clonedFlowService = new FlowService();
		clonedFlowService.setSourceAPNamingInfo(flowService.getSourceAPNamingInfo());
		clonedFlowService.setDestinationAPNamingInfo(flowService.getDestinationAPNamingInfo());
		clonedFlowService.setQoSSpecification(flowService.getQoSSpecification());
		clonedFlowService.setPortId(flowService.getPortId());
		flowAllocatorInstances.put(new Integer(portId), flowAllocatorInstance);
		flowAllocatorInstance.receivedLocalFlowRequest(clonedFlowService);*/
	}
	
	/**
	 * Called by the flow allocator instance when a response for a local flow is received
	 * @param portId
	 * @param flowService
	 * @param result
	 * @param resultReason
	 * @throws IPCException
	 */
	public void receivedLocalFlowResponse(int portId, int remotePortId, boolean result, String resultReason) throws IPCException{
		FlowAllocatorInstance flowAllocatorInstance = getFlowAllocatorInstance(portId);
		flowAllocatorInstance.receivedLocalFlowResponse(remotePortId, result, resultReason);
		if (!result){
			flowAllocatorInstances.remove(new Integer(portId));
		}
	}
	
	/**
	 * Validate the request, create a Flow Allocator Instance and forward it the request for further processing
	 * @param flowService
	 * @param applicationCallback the callback to invoke the application for allocateResponse and any other calls
	 * @throws IPCException
	 */
	public int submitAllocateRequest(FlowRequestEvent flowRequestEvent) throws IPCException{
	/*	log.debug("Local application invoked allocate request: "+flowService.toString());
		allocateRequestValidator.validateAllocateRequest(flowService);
		int portId = this.getIPCProcess().getIPCManager().getAvailablePortId();
		flowService.setPortId(portId);
		FlowAllocatorInstance flowAllocatorInstance = new FlowAllocatorInstanceImpl(this.getIPCProcess(), this, cdapSessionManager, portId);
		flowAllocatorInstance.submitAllocateRequest(flowService, applicationCallback);
		flowAllocatorInstances.put(new Integer(portId), flowAllocatorInstance);
		return portId;*/
		return -1;
	}

	/**
	 * Forward the call to the right FlowAllocator Instance. If the application process 
	 * rejected the flow request, remove the flow allocator instance from the list of 
	 * active flow allocator instances
	 * @param portId
	 * @param success
	 * @param reason
	 * @param applicationCallback
	 */
	public void submitAllocateResponse(int portId, boolean success, String reason) throws IPCException{
		/*log.debug("Local application invoked allocate response for portId "+portId+" with result "+success);
		FlowAllocatorInstance flowAllocatorInstance = getFlowAllocatorInstance(portId);
		flowAllocatorInstance.submitAllocateResponse(success, reason, applicationCallback);
		if (!success){
			flowAllocatorInstances.remove(portId);
			this.getIPCProcess().getIPCManager().freePortId(portId);
		}*/
	}

	/**
	 * Forward the deallocate request to the Flow Allocator Instance.
	 * @param portId
	 */
	public void submitDeallocate(int portId) throws IPCException{
		log.debug("Local application invoked deallocate request for flow at portId "+portId);
		FlowAllocatorInstance flowAllocatorInstance = getFlowAllocatorInstance(portId);
		flowAllocatorInstance.submitDeallocate();
	}
	
	/**
	 * Request to deallocate a local flow
	 * @param portId
	 * @throws IPCException
	 */
	public void receivedDeallocateLocalFlowRequest(int portId) throws IPCException{
		FlowAllocatorInstance flowAllocatorInstance = getFlowAllocatorInstance(portId);
		flowAllocatorInstance.receivedDeallocateLocalFlowRequest();
	}
	
	/**
	 * Returns the flow allocator instance that manages the flow identified by portId
	 * @param portId
	 * @return
	 * @throws IPCException
	 */
	private FlowAllocatorInstance getFlowAllocatorInstance(int portId) throws IPCException{
		FlowAllocatorInstance flowAllocatorInstance = flowAllocatorInstances.get(new Integer(portId));
		
		if (flowAllocatorInstance == null){
			throw new IPCException("Could not find FAI bound to this port-id");
		}
		
		return flowAllocatorInstance;
	}
}