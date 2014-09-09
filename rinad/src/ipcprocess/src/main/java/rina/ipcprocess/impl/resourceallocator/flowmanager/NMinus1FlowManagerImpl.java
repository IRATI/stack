package rina.ipcprocess.impl.resourceallocator.flowmanager;

import java.util.Iterator;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSession;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.events.NMinusOneFlowAllocatedEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowAllocationFailedEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowDeallocatedEvent;
import rina.ipcprocess.impl.resourceallocator.ribobjects.DIFRegistrationRIBObject;
import rina.ipcprocess.impl.resourceallocator.ribobjects.DIFRegistrationSetRIBObject;
import rina.ipcprocess.impl.resourceallocator.ribobjects.NMinus1FlowRIBObject;
import rina.ipcprocess.impl.resourceallocator.ribobjects.NMinus1FlowSetRIBObject;
import rina.resourceallocator.api.NMinus1FlowManager;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;
import eu.irati.librina.AllocateFlowRequestResultEvent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationVector;
import eu.irati.librina.DeallocateFlowResponseEvent;
import eu.irati.librina.ExtendedIPCManagerSingleton;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.FlowPointerVector;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcessDIFRegistrationEvent;
import eu.irati.librina.rina;

/**
 * Manages the allocation and lifetime of N-1 Flows for 
 * an IPC Process 
 * @author eduardgrasa
 *
 */
public class NMinus1FlowManagerImpl implements NMinus1FlowManager{
	
	private static final Log log = LogFactory.getLog(NMinus1FlowManagerImpl.class);
	
	/**
	 * The RIB Daemon
	 */
	private RIBDaemon ribDaemon = null;
	
	/**
	 * The CDAP Session Manager
	 */
	private CDAPSessionManager cdapSessionManager = null;
	
	private ExtendedIPCManagerSingleton ipcManager = null;
	private IPCProcess ipcProcess = null;

	public NMinus1FlowManagerImpl(){
		ipcManager = rina.getExtendedIPCManager();
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		this.ribDaemon = ipcProcess.getRIBDaemon();
		this.cdapSessionManager = ipcProcess.getCDAPSessionManager();
		populateRIB();
	}
	
	private void populateRIB(){
		try{
			RIBObject ribObject = new NMinus1FlowSetRIBObject(ipcProcess, this);
			ribDaemon.addRIBObject(ribObject);
			ribObject = new DIFRegistrationSetRIBObject(ipcProcess, this);
			ribDaemon.addRIBObject(ribObject);
		}catch(RIBDaemonException ex){
			ex.printStackTrace();
			log.error("Could not subscribe to RIB Daemon:" +ex.getMessage());
		}
	}
	
	/**
	 * Return the N-1 Flow descriptor associated to the flow identified by portId
	 * @param portId
	 * @return the N-1 Flow descriptor
	 * @throws IPCException if no N-1 Flow identified by portId exists
	 */
	public FlowInformation getNMinus1FlowInformation(int portId) throws IPCException {
		Flow flow = ipcManager.getAllocatedFlow(portId);
		if (flow == null) {
			throw new IPCException("Could not find allocated N-1 flow with portId "+ portId);
		}
		
		return flow.getFlowInformation();
	}
	
	public FlowInformation[] getAllNMinus1FlowsInformation() {
		FlowPointerVector allocatedFlows = ipcManager.getAllocatedFlows();
		FlowInformation[] result = new FlowInformation[(int)allocatedFlows.size()];
		for(int i=0; i<allocatedFlows.size(); i++){
			result[i] = allocatedFlows.get(i).getFlowInformation();
		}
		
		return result;
	}
	
	/**
	 * Request the allocation of an N-1 Flow with the requested QoS 
	 * to the destination IPC Process 
	 * @param flowService contains the destination IPC Process and requested QoS information
	 */
	public synchronized long allocateNMinus1Flow(FlowInformation flowInformation) throws IPCException {
		long handle =	ipcManager.requestFlowAllocationInDIF(flowInformation.getLocalAppName(), 
				flowInformation.getRemoteAppName(), flowInformation.getDifName(), 
				flowInformation.getFlowSpecification());
		log.info("Requested the allocation of N-1 flow to application " + 
				flowInformation.getRemoteAppName() + " through DIF " + 
				flowInformation.getDifName().getProcessName());
		return handle;
	}
	
	public synchronized void allocateRequestResult(AllocateFlowRequestResultEvent event) throws IPCException{
		if (event.getPortId() > 0) {
			Flow flow = ipcManager.commitPendingFlow(
					event.getSequenceNumber(), event.getPortId(), event.getDIFName());
			try{
				ribDaemon.create(NMinus1FlowRIBObject.N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS, 
						NMinus1FlowSetRIBObject.N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME + 
						RIBObjectNames.SEPARATOR + event.getPortId(), 
						flow.getFlowInformation());
			}catch(RIBDaemonException ex){
				log.warn("Error creating N Minus One Flow RIB Object", ex);
			}

			//TODO Move this to the routing module
			/*if (!nMinus1FlowDescriptor.isManagement()){
				long destinationAddress = this.getNeighborAddress(nMinus1FlowDescriptor.getFlowService().getDestinationAPNamingInfo());
				if (destinationAddress != -1){
					int qosId = nMinus1FlowDescriptor.getFlowService().getQoSSpecification().getQosCubeId();
					this.pduForwardingTable.addEntry(destinationAddress, qosId, new int[]{portId});
				}

				//Send NO-OP PDU if the underlying IPC Process is a SHIM IP DIF and the N-1 flow is reliable
				IPCProcess ipcProcess = (IPCProcess) nMinus1FlowDescriptor.getIpcService();
				if (ipcProcess.getType().equals(IPCProcessType.SHIM_IP) && 
						nMinus1FlowDescriptor.getFlowService().getQoSSpecification().getQosCubeId() == 2){
					try{
						PDU noOpPDU = this.pduParser.generateIdentifySenderPDU(this.ipcProcess.getAddress().longValue(),  
								nMinus1FlowDescriptor.getFlowService().getQoSSpecification().getQosCubeId());
						byte[] sdu = nMinus1FlowDescriptor.getSduProtectionModule().protectPDU(noOpPDU);
						this.ipcManager.getOutgoingFlowQueue(portId).writeDataToQueue(sdu);
					}catch(Exception ex){
						ex.printStackTrace();
						log.error("Problems sending No OP PDU through N-1 flow "+portId, ex);
					}
				}
			}*/
			
			//Notify about the event
			NMinusOneFlowAllocatedEvent allocEvent = new NMinusOneFlowAllocatedEvent(
					flow.getFlowInformation(), event.getSequenceNumber());
			ribDaemon.deliverEvent(allocEvent);
		}else{
			log.error("Allocation of N-1 flow denied. Error code "+event.getPortId());
			FlowInformation flowInformation = ipcManager.withdrawPendingFlow(event.getSequenceNumber());
			flowInformation.setPortId(event.getPortId());
			
			//Notify about the event
			NMinusOneFlowAllocationFailedEvent failedEvent = new NMinusOneFlowAllocationFailedEvent(
					event.getSequenceNumber(), flowInformation, ""+event.getPortId());
			ribDaemon.deliverEvent(failedEvent);
		}
	}
	
	/**
	 * Called by an N-1 DIF
	 */
	public synchronized void flowAllocationRequested(FlowRequestEvent event) throws IPCException {
		if (!event.getLocalApplicationName().getProcessName().equals(
				ipcProcess.getName().getProcessName())
				|| !event.getLocalApplicationName().getProcessInstance().equals(
						ipcProcess.getName().getProcessInstance())){
			log.error("Rejected flow from "+event.getRemoteApplicationName() + 
					" since this IPC Process is not the intended destination of this flow");
			ipcManager.allocateFlowResponse(event, -1, true);
			return;
		}
		
		if (ipcProcess.getOperationalState() != IPCProcess.State.ASSIGNED_TO_DIF) {
			log.error("Rejected flow since this IPC Process is not assigned to a DIF yet");
			ipcManager.allocateFlowResponse(event, -1, true);
			return;
		}

		//TODO deal with the different AEs (Management vs. Data transfer), right now assuming the flow
		//is both used for data transfer and management purposes
		if (ipcManager.getFlowToRemoteApp(event.getRemoteApplicationName()) != null) {
			log.info("Rejecting flow since we already have a" + 
					" flow to the remote application: " + event.getRemoteApplicationName());
			ipcManager.allocateFlowResponse(event, -1, true);
			return;
		}

		Flow flow = ipcManager.allocateFlowResponse(event, 0, true);
		log.info("Accepted new flow from "+ flow.getRemoteApplcationName());

		try{
			ribDaemon.create(NMinus1FlowRIBObject.N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS, 
					NMinus1FlowSetRIBObject.N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME + 
					RIBObjectNames.SEPARATOR + event.getPortId(), 
					flow.getFlowInformation());
		}catch(RIBDaemonException ex){
			log.warn("Error creating N Minus One Flow RIB Object", ex);
		}

		//Notify about the event
		NMinusOneFlowAllocatedEvent allocEvent = new NMinusOneFlowAllocatedEvent(
				flow.getFlowInformation(), event.getSequenceNumber());
		this.ribDaemon.deliverEvent(allocEvent);
	}

	/**
	 * Deallocate the N-1 Flow identified by portId
	 * @param portId
	 * @throws IPCException if no N-1 Flow identified by portId exists
	 */
	public void deallocateNMinus1Flow(int portId) throws IPCException {
		ipcManager.requestFlowDeallocation(portId);
	}
	
	public void deallocateFlowResponse(DeallocateFlowResponseEvent event) throws IPCException {
		boolean success = false;
		if (event.getResult() == 0){
			success = true;
		}
		
		ipcManager.flowDeallocationResult(event.getPortId(), success);
		
		/*
		if (!success) {
			return;
		}
		*/
		
		cleanFlowAndNotify(event.getPortId());
	}
	
	/**
	 * The N-1 flow identified by portId has been deallocated. Generate an N-1 Flow 
	 * deallocated event to trigger a Forwarding table recalculation
	 */
	public void flowDeallocatedRemotely(FlowDeallocatedEvent event) throws IPCException {
		ipcManager.flowDeallocated(event.getPortId());
		cleanFlowAndNotify(event.getPortId());
	}
	
	private void cleanFlowAndNotify(int portId) {
		try{
			ribDaemon.delete(NMinus1FlowRIBObject.N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS, 
					NMinus1FlowSetRIBObject.N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME +
					RIBObjectNames.SEPARATOR + portId);
		}catch(RIBDaemonException ex){
			log.warn("Error deleting N Minus One Flow RIB Object", ex);
		}
		
		//TODO Move this to the routing module
		/*if (!nMinus1FlowDescriptor.isManagement()){
			long destinationAddress = this.getNeighborAddress(nMinus1FlowDescriptor.getFlowService().getDestinationAPNamingInfo());
			if (destinationAddress != -1){
				int qosId = nMinus1FlowDescriptor.getFlowService().getQoSSpecification().getQosCubeId();
				this.pduForwardingTable.removeEntry(destinationAddress, qosId);
			}
		}*/
		
		//Notify about the event
		CDAPSession cdapSession = cdapSessionManager.getCDAPSession(portId);
		CDAPSessionDescriptor cdapSessionDescriptor = null;
		if (cdapSession != null){
			cdapSessionDescriptor = cdapSession.getSessionDescriptor();
		}
		NMinusOneFlowDeallocatedEvent deEvent = 
				new NMinusOneFlowDeallocatedEvent(portId, cdapSessionDescriptor);
		this.ribDaemon.deliverEvent(deEvent);
	}
	
	public void processRegistrationNotification(IPCProcessDIFRegistrationEvent event) throws IPCException {
		if (event.isRegistered()) {
			ipcManager.appRegistered(event.getIPCProcessName(), event.getDIFName());
			log.info("IPC Process registered to N-1 DIF "+ event.getDIFName().getProcessName());
			
			//TODO add RIBDaemon entry
			try{
				this.ribDaemon.create(DIFRegistrationRIBObject.DIF_REGISTRATION_RIB_OBJECT_CLASS, 
						DIFRegistrationSetRIBObject.DIF_REGISTRATION_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR 
						+ event.getDIFName().getProcessName(), event.getDIFName().getProcessName());
			}catch(RIBDaemonException ex){
				log.warn("Error creating DIF Registration RIB Object", ex);
			}
			
		} else{
			ipcManager.appUnregistered(event.getIPCProcessName(), event.getDIFName());
			log.info("IPC Process unregistered from N-1 DIF "+ event.getDIFName().getProcessName());
			
			//TODO remove RIB Daemon entry
		}
	}
	
	/**
	 * True if the DIF name is a supoprting DIF, false otherwise
	 * @param difName
	 * @return
	 */
	public boolean isSupportingDIF(ApplicationProcessNamingInformation difName) {
		ApplicationRegistrationVector registeredApps = ipcManager.getRegisteredApplications();
		Iterator<ApplicationProcessNamingInformation> iterator = null;
		for(int i=0; i<registeredApps.size(); i++) {
			iterator = registeredApps.get(i).getDIFNames().iterator();
			while (iterator.hasNext()){
				if (iterator.next().getProcessName().equals(difName.getProcessName())) {
					return true;
				}
			}
		}
		
		return false;
	}
	
	/**
	 * Get the address of the neighbor
	 * @param apNamingInfo the naming info of the neighbor
	 * @return the neighbor's address, or -1 if it could not be found
	 */
	/*private long getNeighborAddress(ApplicationProcessNamingInfo apNamingInfo){
		List<Neighbor> neighbors = this.ipcProcess.getNeighbors();
		for(int i=0; i<neighbors.size(); i++){
			if (neighbors.get(i).getApplicationProcessName().equals(apNamingInfo.getApplicationProcessName()) && 
					neighbors.get(i).getApplicationProcessInstance().equals(apNamingInfo.getApplicationProcessInstance())){
				return neighbors.get(i).getAddress();
			}
		}
		
		return -1;
	}*/

}
