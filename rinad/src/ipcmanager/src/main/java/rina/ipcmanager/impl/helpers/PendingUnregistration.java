package rina.ipcmanager.impl.helpers;

import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.IPCProcess;

public class PendingUnregistration {

	ApplicationUnregistrationRequestEvent event;
	IPCProcess ipcProcess;
	
	public PendingUnregistration(
			ApplicationUnregistrationRequestEvent event, IPCProcess ipcProcess){
		this.event = event;
		this.ipcProcess = ipcProcess;
	}
	
	public ApplicationUnregistrationRequestEvent getEvent() {
		return event;
	}
	public void setEvent(ApplicationUnregistrationRequestEvent event) {
		this.event = event;
	}
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	
}
