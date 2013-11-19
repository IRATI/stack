package rina.ipcmanager.impl.helpers;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.IPCProcess;

public class PendingEnrollmentOperation {

	private IPCProcess ipcProcess = null;
	private ApplicationProcessNamingInformation difName = null;
	private ApplicationProcessNamingInformation supportingDifName = null;
	private ApplicationProcessNamingInformation neighborName = null;
	
	public PendingEnrollmentOperation(){
	}
	
	public PendingEnrollmentOperation(IPCProcess ipcProcess, 
			ApplicationProcessNamingInformation difName, 
			ApplicationProcessNamingInformation supportingDifName,
			ApplicationProcessNamingInformation neighborName) {
		this.ipcProcess = ipcProcess;
		this.difName = difName;
		this.supportingDifName = supportingDifName;
		this.neighborName = neighborName;
	}
	
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	public ApplicationProcessNamingInformation getDifName() {
		return difName;
	}
	public void setDifName(ApplicationProcessNamingInformation difName) {
		this.difName = difName;
	}
	public ApplicationProcessNamingInformation getSupportingDifName() {
		return supportingDifName;
	}
	public void setSupportingDifName(
			ApplicationProcessNamingInformation supportingDifName) {
		this.supportingDifName = supportingDifName;
	}
	public ApplicationProcessNamingInformation getNeighborName() {
		return neighborName;
	}
	public void setNeighborName(ApplicationProcessNamingInformation neighborName) {
		this.neighborName = neighborName;
	}
}
