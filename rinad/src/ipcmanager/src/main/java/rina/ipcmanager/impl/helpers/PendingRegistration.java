package rina.ipcmanager.impl.helpers;

import rina.ipcmanager.impl.IPCManager;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.IPCProcess;

public class PendingRegistration {

	private ApplicationRegistrationRequestEvent event;
	private IPCProcess ipcProcess;
	private long ipcProcessId = IPCManager.NO_IPC_PROCESS_ID;
	
	public PendingRegistration(
			ApplicationRegistrationRequestEvent event, IPCProcess ipcProcess){
		this.event = event;
		this.ipcProcess = ipcProcess;
	}
	
	public PendingRegistration(
			ApplicationRegistrationRequestEvent event, IPCProcess ipcProcess, 
			long ipcProcessId){
		this.event = event;
		this.ipcProcess = ipcProcess;
		this.ipcProcessId = ipcProcessId;
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
	public long getIPCProcessId(){
		return ipcProcessId;
	}
	
	public boolean isApplicationRegisteredIPCProcess(){
		return ipcProcessId != IPCManager.NO_IPC_PROCESS_ID;
	}
	
}
