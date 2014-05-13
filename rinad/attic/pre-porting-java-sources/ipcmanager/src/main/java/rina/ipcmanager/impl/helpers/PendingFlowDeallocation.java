package rina.ipcmanager.impl.helpers;

import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.IPCProcess;

public class PendingFlowDeallocation {

	FlowDeallocateRequestEvent event = null;
	IPCProcess ipcProcess = null;
	
	public PendingFlowDeallocation(
			FlowDeallocateRequestEvent event, IPCProcess ipcProcess){
		this.event = event;
		this.ipcProcess = ipcProcess;
	}
	
	public FlowDeallocateRequestEvent getEvent() {
		return event;
	}
	public void setEvent(FlowDeallocateRequestEvent event) {
		this.event = event;
	}
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	
}
