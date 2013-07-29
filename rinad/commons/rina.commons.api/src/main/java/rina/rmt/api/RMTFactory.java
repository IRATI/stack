package rina.rmt.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Creates, destroys and stores instances of the RMTs. Each 
 * instance of an RMT maps to an instance of an IPC Process 
 * @author eduardgrasa
 *
 */
public interface RMTFactory {
	
	/**
	 * Creates a new RMT
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this RMT belongs
	 */
	public RMT createRMT(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Destroys an existing RMT
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this RMT belongs
	 */
	public void destroyRMT(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Get an existing RMT
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this RMT belongs
	 */
	public RMT getRMT(ApplicationProcessNamingInfo namingInfo);
}
