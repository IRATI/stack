package rina.ipcmanager.impl.helpers;

import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.IPCProcess;

public class PendingRegistration {

	ApplicationRegistrationRequestEvent event;
	IPCProcess ipcProcess;
	
	public PendingRegistration(
			ApplicationRegistrationRequestEvent event, IPCProcess ipcProcess){
		this.event = event;
		this.ipcProcess = ipcProcess;
	}
	
	public ApplicationRegistrationRequestEvent getEvent() {
		return event;
	}
	public void setEvent(ApplicationRegistrationRequestEvent event) {
		this.event = event;
	}
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	
}
