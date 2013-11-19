package rina.ipcmanager.impl.helpers;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowResponseEvent;
import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessPointerVector;
import eu.irati.librina.IpcmAllocateFlowRequestResultEvent;
import eu.irati.librina.IpcmDeallocateFlowResponseEvent;

public class FlowManager {

	private static final Log log = LogFactory.getLog(FlowManager.class);
	public static final int MAX_FLOWS = 1000;
	
	private IPCProcessManager ipcProcessManager = null;
	private ApplicationManagerSingleton applicationManager = null;
	private Map<Long, PendingFlowAllocation> pendingFlowAllocations = null;
	private Map<Long, PendingFlowDeallocation> pendingFlowDeallocations = null;
	
	public FlowManager(IPCProcessManager ipcProcessManager, 
			ApplicationManagerSingleton applicationManager){
		this.ipcProcessManager = ipcProcessManager;
		this.applicationManager = applicationManager;
		this.pendingFlowAllocations = 
				new ConcurrentHashMap<Long, PendingFlowAllocation>();
		this.pendingFlowDeallocations = 
				new ConcurrentHashMap<Long, PendingFlowDeallocation>();
	}
	
	public synchronized void requestAllocateFlowLocal(FlowRequestEvent event) throws Exception{
		PendingFlowAllocation pendingFlowAllocation = null;
		long handle = 0;
		IPCProcess ipcProcess = null;
		String difName = null;
		
		if (event.getDIFName() != null && event.getDIFName().getProcessName() != null && 
				!event.getDIFName().getProcessName().equals("")) {
			difName = event.getDIFName().getProcessName();
		}
		
		try{
			if (difName == null){
				ipcProcess = ipcProcessManager.selectAnyIPCProcess();
			} else {
				ipcProcess = ipcProcessManager.selectIPCProcessOfDIF(difName);
			}
			
			handle = ipcProcess.allocateFlow(event);
			
			log.debug("Requested allocation of flow from "+event.getLocalApplicationName().toString()
					+ "to remote application "+event.getRemoteApplicationName().toString()
					+" to the DIF "+ipcProcess.getDIFInformation().getDifName().toString() + 
					". Got handle "+handle);
			
			pendingFlowAllocation = new PendingFlowAllocation(event, ipcProcess);
			if (difName != null) {
				pendingFlowAllocation.setTryOnlyOneDIF(true);
			}
			pendingFlowAllocations.put(handle, pendingFlowAllocation);
		}catch(Exception ex){
			log.error("Error allocating flow. "+ex.getMessage());
			event.setPortId(-1);
			applicationManager.flowAllocated(event);
			return;
		}
	}
	
	/**
	 * Looks for the pending registration with the event sequence number, and updates the 
	 * registration state based on the result
	 * @throws Exception
	 */
	public synchronized void allocateFlowRequestResult(
			IpcmAllocateFlowRequestResultEvent event) throws Exception {
		PendingFlowAllocation pendingFlowAllocation = null;
		IPCProcess ipcProcess = null;
		FlowRequestEvent flowReqEvent = null;
		boolean success;
		long handle;
		
		pendingFlowAllocation = pendingFlowAllocations.remove(event.getSequenceNumber());
		if (pendingFlowAllocation == null){
			throw new Exception("Could not find a pending local flow allocation associated to the handle "
					+event.getSequenceNumber());
		}
		
		ipcProcess = pendingFlowAllocation.getIpcProcess();
		flowReqEvent = pendingFlowAllocation.getEvent();
		if (event.getResult() == 0){
			success = true;
		}else{
			success = false;
		}
		
		try {
			ipcProcess.allocateFlowResult(event.getSequenceNumber(), success, event.getPortId());
			if (success){
				flowReqEvent.setPortId(event.getPortId());
				log.info("Successfully allocated flow from "+ 
						flowReqEvent.getLocalApplicationName().toString() + " to remote application "
						+ flowReqEvent.getRemoteApplicationName().toString() +
						" in DIF "+ipcProcess.getDIFInformation().getDifName().toString() + 
						" with port-id "+ event.getPortId());
			} else {
				log.info("Could not allocate flow from application "+ 
						flowReqEvent.getLocalApplicationName().toString() + " to remote application "
						+ flowReqEvent.getRemoteApplicationName().toString() +
						" in DIF "+ipcProcess.getDIFInformation().getDifName().toString());
				
				if (pendingFlowAllocation.isTryOnlyOneDIF()) {
					log.info("No more IPC Processes to try, giving up");
					flowReqEvent.setPortId(-1);
				}else {
					pendingFlowAllocation.addTriedIPCProcess(ipcProcess.getName().toString());
					ipcProcess = ipcProcessManager.getIPCProcessNotInList(
							pendingFlowAllocation.getTriedIPCProcesses());

					if (ipcProcess == null){
						log.info("No more IPC Processes to try, giving up");
						flowReqEvent.setPortId(-1);
					}else{
						log.info("Trying flow allocation again with IPC Process " + 
								ipcProcess.getDIFInformation().getDifName().toString());
						handle = ipcProcess.allocateFlow(flowReqEvent);
						pendingFlowAllocations.put(handle, pendingFlowAllocation);
						return;
					}
				}
			}
		}catch(Exception ex){
			log.error("Problems processing IpcmAllocateFlowRequestResultEvent. Handle: "+event.getSequenceNumber() + 
					  "; Local application name: "+ flowReqEvent.getLocalApplicationName().toString() +
					  "; Remote application name: "+ flowReqEvent.getRemoteApplicationName().toString() +
					  "; DIF name: " + ipcProcess.getDIFInformation().getDifName().toString());
			flowReqEvent.setPortId(-1);
		}
		
		applicationManager.flowAllocated(flowReqEvent);
	}
	
	public synchronized void allocateFlowRemote(FlowRequestEvent event) throws Exception{
		PendingFlowAllocation pendingFlowAllocation = null;
		IPCProcess ipcProcess = null;
		String difName = null;
		long handle = 0;
		
		difName = event.getDIFName().getProcessName();
		ipcProcess = ipcProcessManager.selectIPCProcessOfDIF(difName);
		if (ipcProcess == null){
			log.error("Received an allocate remote flow request event, but could not " 
						+ "find a local IPC Process belonging to DIF "+difName);
			return;
		}

		try{
			handle = applicationManager.flowRequestArrived(event.getLocalApplicationName(), 
					event.getRemoteApplicationName(), event.getFlowSpecification(), 
					event.getDIFName(), event.getPortId());
			log.debug("Requested allocation of remote flow from "+event.getRemoteApplicationName().toString()
					+ "to local application "+event.getLocalApplicationName().toString()
					+" through the DIF "+ipcProcess.getDIFInformation().getDifName().toString() + 
					". Got handle "+handle + " and portId " + event.getPortId());
			pendingFlowAllocation = new PendingFlowAllocation(event, ipcProcess);
			pendingFlowAllocations.put(handle, pendingFlowAllocation);
		}catch(Exception ex){
			log.error("Error allocating flow. "+ex.getMessage());
			ipcProcess.allocateFlowResponse(event, -1, true);
			return;
		}
	}
	
	public synchronized void allocateFlowResponse(AllocateFlowResponseEvent event) throws Exception{
		PendingFlowAllocation pendingFlowAllocation = null;
		IPCProcess ipcProcess = null;
		FlowRequestEvent flowReqEvent = null;
		boolean success;

		pendingFlowAllocation = pendingFlowAllocations.remove(event.getSequenceNumber());
		if (pendingFlowAllocation == null){
			throw new Exception("Could not find a pending local flow allocation associated to the handle "
					+event.getSequenceNumber());
		}

		ipcProcess = pendingFlowAllocation.getIpcProcess();
		flowReqEvent = pendingFlowAllocation.getEvent();
		if (event.getResult() == 0){
			success = true;
		}else{
			success = false;
		}

		try {
			ipcProcess.allocateFlowResponse(flowReqEvent, event.getResult(), event.isNotifySource());
			if (success){
				log.info("Successfully allocated flow from "+ 
						flowReqEvent.getLocalApplicationName().toString() + " to remote application "
						+ flowReqEvent.getRemoteApplicationName().toString() +
						" in DIF "+ipcProcess.getDIFInformation().getDifName().toString());
			} else {
				log.info("Could not allocate flow from application "+ 
						flowReqEvent.getLocalApplicationName().toString() + " to remote application "
						+ flowReqEvent.getRemoteApplicationName().toString() +
						" in DIF "+ipcProcess.getDIFInformation().getDifName().toString());
				flowReqEvent.setPortId(-1);
			}
		}catch(Exception ex){
			log.error("Problems processing IpcmAllocateFlowRequestResultEvent. Handle: "+event.getSequenceNumber() + 
					"; Local application name: "+ flowReqEvent.getLocalApplicationName().toString() +
					"; Remote application name: "+ flowReqEvent.getRemoteApplicationName().toString() +
					"; DIF name: " + ipcProcess.getDIFInformation().getDifName().toString());

			flowReqEvent.setPortId(-1);
		}
	}
	
	public synchronized void deallocateFlowRequest(FlowDeallocateRequestEvent event) throws Exception{
		PendingFlowDeallocation pendingFlowDeallocation = null;
		long handle = 0;
		IPCProcess ipcProcess = null;
		
		try{
			ipcProcess = ipcProcessManager.selectIPCProcessWithFlow(event.getPortId());
			handle = ipcProcess.deallocateFlow(event.getPortId());
			log.debug("Requested deallocation of flow with portId " + event.getPortId() + 
					" to DIF " + ipcProcess.getDIFInformation().getDifName().getProcessName());
			pendingFlowDeallocation = new PendingFlowDeallocation(event, ipcProcess);
			pendingFlowDeallocations.put(handle, pendingFlowDeallocation);
		}catch(Exception ex){
			log.error("Error deallocating flow. "+ex.getMessage());
			applicationManager.flowDeallocated(event, -1);
			return;
		}
	}
	
	public synchronized void deallocateFlowResponse(IpcmDeallocateFlowResponseEvent event) throws Exception{
		PendingFlowDeallocation pendingFlowDeallocation = null;
		IPCProcess ipcProcess = null;
		FlowDeallocateRequestEvent flowDelEvent = null;
		boolean success;

		pendingFlowDeallocation = pendingFlowDeallocations.remove(event.getSequenceNumber());
		if (pendingFlowDeallocation == null){
			throw new Exception("Could not find a pending local flow deallocation associated to the handle "
					+event.getSequenceNumber());
		}

		ipcProcess = pendingFlowDeallocation.getIpcProcess();
		flowDelEvent = pendingFlowDeallocation.getEvent();
		if (event.getResult() == 0){
			success = true;
		}else{
			success = false;
		}

		try {
			ipcProcess.deallocateFlowResult(event.getSequenceNumber(), success);
			if (success){
				log.info("Successfully deallocated flow with portId "+ flowDelEvent.getPortId() + 
						" in DIF "+ipcProcess.getDIFInformation().getDifName().toString());
			} else {
				log.info("Could not deallocate flow from application with portId "+ flowDelEvent.getPortId() + 
						" in DIF "+ipcProcess.getDIFInformation().getDifName().toString());
			}
			
			if (flowDelEvent.getSequenceNumber() > 0) {
				applicationManager.flowDeallocated(flowDelEvent, event.getResult());
			}
		}catch(Exception ex){
			log.error("Problems processing IpcmDeallocateFlowResponseEvent. Handle: "+event.getSequenceNumber() + 
					"; Port id: "+ flowDelEvent.getPortId()+
					"; DIF name: " + ipcProcess.getDIFInformation().getDifName().toString());

			if (flowDelEvent.getSequenceNumber() > 0) {
				applicationManager.flowDeallocated(flowDelEvent, -1);
			}
		}
	}
	
	public synchronized void flowDeallocated(FlowDeallocatedEvent event) throws Exception{
		FlowInformation flowInformation = null;
		IPCProcess ipcProcess = ipcProcessManager.selectIPCProcessWithFlow(event.getPortId());
		flowInformation = ipcProcess.flowDeallocated(event.getPortId());
		applicationManager.flowDeallocatedRemotely(event.getPortId(), event.getCode(), 
				flowInformation.getLocalAppName());
	}
	
	/**
	 * Called when the IPC Manager is informed that the application identified by apName (process + instance) 
	 * has terminated. We have to look for potential flows of the application and deallocate them
	 * @param apName
	 */
	public synchronized void cleanFlows(ApplicationProcessNamingInformation apName){
		List<FlowInformation> flows = getFlowsForApplication(apName);
		log.info(flows.size() + " flows are going to be deallocated");
		
		IPCProcess ipcProcess = null;
		FlowInformation currentFlow = null;
		long handle = -1;
		FlowDeallocateRequestEvent event = null;
		PendingFlowDeallocation pendingFlowDeallocation = null;
		for(int i=0; i<flows.size(); i++){
			currentFlow = flows.get(i);
			try{
				ipcProcess = ipcProcessManager.selectIPCProcessOfDIF(
						currentFlow.getDifName().getProcessName());
				if (ipcProcess == null){
					log.error("Could not find IPC Process of DIF "+ 
							currentFlow.getDifName().getProcessName());
					continue;
				}

				handle = ipcProcess.deallocateFlow(currentFlow.getPortId());
				log.debug("Requested deallocation of flow with portId " + currentFlow.getPortId() + 
						" to DIF " + ipcProcess.getDIFInformation().getDifName().getProcessName());
				event = new FlowDeallocateRequestEvent(currentFlow.getPortId(), 0);
				pendingFlowDeallocation = new PendingFlowDeallocation(event, ipcProcess);
				pendingFlowDeallocations.put(handle, pendingFlowDeallocation);
			}catch(Exception ex){
				log.error("Error deallocating flow with port id " + currentFlow.getPortId());
			}
		}
	}
	
	private List<FlowInformation> getFlowsForApplication(ApplicationProcessNamingInformation apName){
		List<FlowInformation> result = new ArrayList<FlowInformation>();
		IPCProcessPointerVector ipcProcesses = ipcProcessManager.listIPCProcesses();
		Iterator<FlowInformation> ipcProcessFlowsIterator = null;
		FlowInformation currentFlow = null;
		
		for(int i=0; i<ipcProcesses.size(); i++) {
			ipcProcessFlowsIterator = ipcProcesses.get(i).getAllocatedFlows().iterator();
			while (ipcProcessFlowsIterator.hasNext()) {
				currentFlow = ipcProcessFlowsIterator.next();
				if (currentFlow.getLocalAppName().getProcessName().equals(apName.getProcessName()) && 
						currentFlow.getLocalAppName().getProcessInstance().equals(apName.getProcessInstance())){
					result.add(currentFlow);
				}
			}
		}
		
		return result;
	}
}
