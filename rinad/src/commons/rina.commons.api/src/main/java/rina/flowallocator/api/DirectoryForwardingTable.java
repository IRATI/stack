package rina.flowallocator.api;

import eu.irati.librina.ApplicationProcessNamingInformation;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * The directory. Maps application process names to IPC process addresses
 * @author eduardgrasa
 *
 */
public interface DirectoryForwardingTable {
	
	public static final String DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + 
		RIBObjectNames.DIF + RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + 
		RIBObjectNames.FLOW_ALLOCATOR + RIBObjectNames.SEPARATOR + RIBObjectNames.DIRECTORY_FORWARDING_TABLE_ENTRIES;
	
	public static final String DIRECTORY_FORWARDING_TABLE_ENTRY_SET_RIB_OBJECT_CLASS = "directoryforwardingtableentry set";
	
	public static final String DIRECTORY_FORWARDING_TABLE_ENTRY_RIB_OBJECT_CLASS = "directoryforwardingtableentry";

	/**
	 * Returns the address of the IPC process where the application process is, or 
	 * null otherwise
	 * @param apNamingInfo
	 * @return
	 */
	public long getAddress(ApplicationProcessNamingInformation apNamingInfo);
}
