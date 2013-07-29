package rina.ipcprocess.api;

/**
 * Base class for an IPC Process component
 * @author eduardgrasa
 *
 */
public abstract class BaseIPCProcessComponent implements IPCProcessComponent{
	
	private IPCProcess ipcProcess = null;
	
	public void setIPCProcess(IPCProcess ipcProcess) {
		this.ipcProcess = ipcProcess;
	}
	
	public synchronized IPCProcess getIPCProcess(){
		return this.ipcProcess;
	}
	
	public void stop(){
		//Default implementation: do nothing
	}

}
