package rina.ipcmanager.impl.helpers;

import eu.irati.librina.IPCProcess;

public class PendingIPCProcessRegistration {
	
	private IPCProcess ipcProcess;
	private IPCProcess nMinusOneIPCProcess;
	private String difName = null;
	
	public PendingIPCProcessRegistration(String difName,
			IPCProcess ipcProcess, IPCProcess nMinusOneIPCProcess){
		this.ipcProcess = ipcProcess;
		this.nMinusOneIPCProcess = nMinusOneIPCProcess;
		this.difName = difName;
	}
	
	public IPCProcess getIpcProcess() {
		return ipcProcess;
	}
	
	public void setIpcProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}

	public IPCProcess getnMinusOneIPCProcess() {
		return nMinusOneIPCProcess;
	}

	public void setnMinusOneIPCProcess(IPCProcess nMinusOneIPCProcess) {
		this.nMinusOneIPCProcess = nMinusOneIPCProcess;
	}

	public String getDifName() {
		return difName;
	}

	public void setDifName(String difName) {
		this.difName = difName;
	}
	
}
