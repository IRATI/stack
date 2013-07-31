package rina.ipcmanager.impl.helpers;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowException;
import eu.irati.librina.ApplicationManagerSingleton;
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
	
	public void allocateFlow(FlowRequestEvent event) throws Exception{
		try{
			int portId = getAvailablePortId();
			event.setPortId(portId);
			IPCProcess ipcProcess = tryFlowAllocation(event);
			event.setDIFName(ipcProcess.getConfiguration().getDifName());
			FlowState flowState = new FlowState(event);
			flowState.setIpcProcessId(ipcProcess.getId());
			flowState.setDifName(ipcProcess.getConfiguration().getDifName().getProcessName());
			flows.put(portId, flowState);
		}catch(Exception ex){
			log.error("Error allocating flow. "+ex.getMessage());
			event.setPortId(-1);
			applicationManager.flowAllocated(event, ex.getMessage());
			return;
		}
		
		applicationManager.flowAllocated(event, "");
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
}
