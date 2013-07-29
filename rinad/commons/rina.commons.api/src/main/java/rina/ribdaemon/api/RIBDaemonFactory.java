package rina.ribdaemon.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Creates, destroys and stores instances of the RIBDaemon. Each 
 * instance of a RIB daemon maps to an instance of an IPC Process 
 * @author eduardgrasa
 *
 */
public interface RIBDaemonFactory {
	
	/**
	 * Creates a new RIB Daemon
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this RIB Daemon belongs
	 */
	public RIBDaemon createRIBDaemon(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Destroys an existing RIB Daemon process
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this RIB Daemon belongs
	 */
	public void destroyRIBDaemon(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Get an existing RIB Daemon
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this RIB Daemon belongs
	 */
	public RIBDaemon getRIBDaemon(ApplicationProcessNamingInfo namingInfo);
}
