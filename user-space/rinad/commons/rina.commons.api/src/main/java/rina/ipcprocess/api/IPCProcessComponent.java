package rina.ipcprocess.api;

/**
 * A component of an IPC process
 * @author eduardgrasa
 */
public interface IPCProcessComponent {
	
	/**
	 * The IPC process where this IPC Process component belongs
	 * @param ipcProcess
	 */
	public void setIPCProcess(IPCProcess ipcProcess);
	
	/**
	 * Returns the ipc process this component is a part of
	 * @return
	 */
	public IPCProcess getIPCProcess();
	
	/**
	 * Return the name of this IPC Process Component
	 * @return
	 */
	public String getName();
	
	/**
	 * Called to stop this component
	 */
	public void stop();
}
