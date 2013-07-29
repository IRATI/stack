package rina.idd.api;

import rina.ipcmanager.api.IPCManager;

/**
 * Creates InterDIF Directories
 * @author eduardgrasa
 *
 */
public interface InterDIFDirectoryFactory {
	/**
	 * Creates an InterDIFDirectory instance
	 * @param ipcManager the IPCManager that will be using the Inter-DIF Directory
	 * @return
	 */
	public InterDIFDirectory createIDD(IPCManager ipcManager);
}
