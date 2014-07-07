package rina.registrationmanager.api;

import rina.flowallocator.api.DirectoryForwardingTableEntry;
import rina.ipcprocess.api.IPCProcessComponent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;

public interface RegistrationManager extends IPCProcessComponent {

	/**
	 * Returns the address of the IPC process where the application process is, or 
	 * null otherwise
	 * @param apNamingInfo
	 * @return
	 */
	public long getAddress(ApplicationProcessNamingInformation apNamingInfo);
	
	/**
	 * Returns the IPC Process id (0 if not an IPC Process) of the registered 
	 * application, or -1 if the app is not registered
	 * @param apNamingInfo
	 * @return
	 */
	public int getRegIPCProcessId(ApplicationProcessNamingInformation apNamingInfo);
	
	/**
	 * Add an entry to the directory forwarding table
	 * @param entry
	 */
	public void addDFTEntry(DirectoryForwardingTableEntry entry);
	
	/**
	 * Get an entry from the application name
	 * @param apNamingInfo
	 * @return
	 */
	public DirectoryForwardingTableEntry getDFTEntry(
			ApplicationProcessNamingInformation apNamingInfo);

	/**
	 * Remove an entry from the directory forwarding table
	 * @param apNamingInfo
	 */
	public void removeDFTEntry(ApplicationProcessNamingInformation apNamingInfo);
	
	/** 
	 * Process an application registration request
	 * @param event
	 */
	public void processApplicationRegistrationRequestEvent(
			ApplicationRegistrationRequestEvent event);
	
	/**
	 * Process an application unregistration request
	 * @param event
	 */
	public void processApplicationUnregistrationRequestEvent(
			ApplicationUnregistrationRequestEvent event);

}
