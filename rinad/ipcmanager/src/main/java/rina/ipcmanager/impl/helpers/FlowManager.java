package rina.ipcmanager.impl.helpers;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowException;
import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.IPCProcessPointerVector;

public class FlowManager {

	private static final Log log = LogFactory.getLog(FlowManager.class);
	public static final int MAX_FLOWS = 1000;
	
	private IPCProcessFactorySingleton ipcProcessFactory = null;
	private ApplicationManagerSingleton applicationManager = null;
	private Map<Integer, FlowState> flows;
	
	public FlowManager(IPCProcessFactorySingleton ipcProcessFactory, 
			ApplicationManagerSingleton applicationManager){
		this.ipcProcessFactory = ipcProcessFactory;
		this.applicationManager = applicationManager;
		this.flows = new ConcurrentHashMap<Integer, FlowState>();
	}
	
	public void allocateFlowLocal(FlowRequestEvent event) throws Exception{
		int portId = 0;
		
		try{
			portId = getAvailablePortId();
			event.setPortId(portId);
			FlowState flowState = new FlowState(event);
			flows.put(portId, flowState);
			IPCProcess ipcProcess = tryFlowAllocation(event);
			event.setDIFName(ipcProcess.getConfiguration().getDifName());
			flowState.setIpcProcessId(ipcProcess.getId());
			flowState.setDifName(ipcProcess.getConfiguration().getDifName().getProcessName());
		}catch(Exception ex){
			log.error("Error allocating flow. "+ex.getMessage());
			flows.remove(portId);
			event.setPortId(-1);
			applicationManager.flowAllocated(event);
			return;
		}
		
		applicationManager.flowAllocated(event);
	}
	
	public void allocateFlowRemote(FlowRequestEvent event) throws Exception{
		String difName = event.getDIFName().getProcessName();
		IPCProcess ipcProcess = selectIPCProcessOfDIF(difName);
		if (ipcProcess == null){
			log.error("Received an allocate remote flow request event, but could not " 
						+ "find a local IPC Process belonging to DIF "+difName);
			return;
		}

		int portId = 0;
		try{
			portId = getAvailablePortId();
			event.setPortId(portId);
			FlowState flowState = new FlowState(event);
			flowState.setIpcProcessId(ipcProcess.getId());
			flowState.setDifName(ipcProcess.getConfiguration().getDifName().getProcessName());
			flows.put(portId, flowState);
			applicationManager.flowRequestArrived(event.getLocalApplicationName(), 
					event.getRemoteApplicationName(), event.getFlowSpecification(), 
					event.getDIFName(), event.getPortId());
		}catch(Exception ex){
			log.error("Error allocating flow. "+ex.getMessage());
			flows.remove(portId);
			ipcProcess.allocateFlowResponse(event, -1);
			return;
		}
		
		ipcProcess.allocateFlowResponse(event, 0);
	}
	
	public void deallocateFlow(FlowDeallocateRequestEvent event) throws Exception{
		FlowState flow = null;
		IPCProcess ipcProcess = null;
		String difName = null;
		
		try{
			flow = getFlow(event.getPortId());
			difName = flow.getDifName();
			ipcProcess = selectIPCProcessOfDIF(difName);
			if (ipcProcess == null){
				throw new Exception("Could not find IPC process belonging to DIF "+difName);
			}
			
			ipcProcess.deallocateFlow(event.getPortId());
			flows.remove(event.getPortId());
		}catch(Exception ex){
			log.error("Error deallocating flow. "+ex.getMessage());
			applicationManager.flowDeallocated(event, -1);
			return;
		}
		
		applicationManager.flowDeallocated(event, 0);
	}
	
	public void flowDeallocated(FlowDeallocatedEvent event) throws Exception{
		FlowState flow = null;
		try{
			flow = getFlow(event.getPortId());
			flows.remove(event.getPortId());
			applicationManager.flowDeallocatedRemotely(
					event.getPortId(), event.getCode(), 
					flow.getLocalApplication());
		}catch(Exception ex){
			log.error("Error processing notification of flow deallocation. "+ex.getMessage());
		}
	}
	
	private int getAvailablePortId() throws Exception{
		for(int i=1; i<=MAX_FLOWS; i++){
			if (flows.containsKey(i)){
				continue;
			}
			
			return i;
		}
		
		throw new Exception("Cannot allocate flow: reached maximum number of flows ("+MAX_FLOWS+")");
	}
	
	private IPCProcess tryFlowAllocation(FlowRequestEvent event) throws Exception{
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		if (ipcProcesses.size() == 0){
			throw new Exception("Could not find IPC Process to allocate the flow");
		}

		IPCProcess ipcProcess = null;
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			try{
				ipcProcess.allocateFlow(event);
				return ipcProcess;
			}catch(AllocateFlowException ex){
				log.info("Tried to allocate a flow in IPC Process "+
						ipcProcess.getId()+" but failed: "+ex.getMessage());
				continue;
			}
		}
		
		throw new Exception("Flow allocation failed after trying in all the IPC Processes.");
	}
	
	private IPCProcess selectIPCProcessOfDIF(String difName) {
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		IPCProcess ipcProcess = null;
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			log.info("Trying IPC Process "+ipcProcess.getId());
			DIFConfiguration difConfiguration = ipcProcess.getConfiguration();
			if (difConfiguration != null && 
					difConfiguration.getDifName().getProcessName().equals(difName)){
				return ipcProcess;
			}
		}
		
		return null;
	}
	
	private FlowState getFlow(int portId) throws Exception{
		FlowState result = flows.get(portId);
		if (result == null){
			throw new Exception("Could not find flow with portId "+portId);
		}
		
		return result;
	}
}
