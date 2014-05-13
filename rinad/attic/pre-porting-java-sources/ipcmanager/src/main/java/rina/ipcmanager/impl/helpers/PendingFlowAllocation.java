package rina.ipcmanager.impl.helpers;

import java.util.ArrayList;
import java.util.List;

import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCProcess;

public class PendingFlowAllocation {

	FlowRequestEvent event = null;
	IPCProcess ipcProcess = null;
	List<String> triedIPCProcesses = null;
	boolean tryOnlyOneDIF = false;
	
	public PendingFlowAllocation(
			FlowRequestEvent event, IPCProcess ipcProcess){
		this.event = event;
		this.ipcProcess = ipcProcess;
		this.triedIPCProcesses = new ArrayList<String>();
	}
	
	public FlowRequestEvent getEvent() {
		return event;
	}
	public void setEvent(FlowRequestEvent event) {
		this.event = event;
	}
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	public void addTriedIPCProcess(String encodedIPCProcessName){
		triedIPCProcesses.add(encodedIPCProcessName);
	}
	public List<String> getTriedIPCProcesses(){
		return triedIPCProcesses;
	}

	public boolean isTryOnlyOneDIF() {
		return tryOnlyOneDIF;
	}

	public void setTryOnlyOneDIF(boolean tryOnlyOneDIF) {
		this.tryOnlyOneDIF = tryOnlyOneDIF;
	}
}
