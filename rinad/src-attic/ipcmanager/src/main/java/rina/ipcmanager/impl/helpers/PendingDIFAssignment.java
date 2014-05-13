package rina.ipcmanager.impl.helpers;

import eu.irati.librina.DIFInformation;
import eu.irati.librina.IPCProcess;

public class PendingDIFAssignment {

	DIFInformation difInformation;
	IPCProcess ipcProcess;
	
	public PendingDIFAssignment(
			DIFInformation difInformation, IPCProcess ipcProcess){
		this.difInformation = difInformation;
		this.ipcProcess = ipcProcess;
	}
	
	public DIFInformation getDIFInformation() {
		return difInformation;
	}
	public void setDIFInformation(DIFInformation difInformation) {
		this.difInformation = difInformation;
	}
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	
}
