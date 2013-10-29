package rina.ipcmanager.impl.helpers;

import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.IPCProcess;

public class PendingDIFConfiguration {

	DIFConfiguration difConfiguration;
	IPCProcess ipcProcess;
	
	public PendingDIFConfiguration(
			DIFConfiguration difConfiguration, IPCProcess ipcProcess){
		this.difConfiguration = difConfiguration;
		this.ipcProcess = ipcProcess;
	}
	
	public DIFConfiguration getDIFConfiguration() {
		return difConfiguration;
	}
	public void setDIFConfiguration(DIFConfiguration difConfiguration) {
		this.difConfiguration = difConfiguration;
	}
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	
}
