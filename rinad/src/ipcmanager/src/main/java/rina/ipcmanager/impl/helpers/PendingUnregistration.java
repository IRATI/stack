package rina.ipcmanager.impl.helpers;

import rina.ipcmanager.impl.IPCManager;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.IPCProcess;

public class PendingUnregistration {

	ApplicationUnregistrationRequestEvent event;
	IPCProcess ipcProcess;
	private long ipcProcessId = IPCManager.NO_IPC_PROCESS_ID;
	
	public PendingUnregistration(
			ApplicationUnregistrationRequestEvent event, IPCProcess ipcProcess){
		this.event = event;
		this.ipcProcess = ipcProcess;
	}
	
	public PendingUnregistration(
			ApplicationUnregistrationRequestEvent event, IPCProcess ipcProcess, 
			long ipcProcessId){
		this.event = event;
		this.ipcProcess = ipcProcess;
		this.ipcProcessId = ipcProcessId;
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
	public long getIPCProcessId(){
		return ipcProcessId;
	}
	
	public boolean isApplicationRegisteredIPCProcess(){
		return ipcProcessId != IPCManager.NO_IPC_PROCESS_ID;
	}
	
}
