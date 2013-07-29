package rina.efcp.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Creates, destroys and stores instances of the DataTransferAE. Each 
 * instance of an DataTransferAE maps to an instance of an IPC Process 
 * @author eduardgrasa
 *
 */
public interface DataTransferAEFactory {
	
	/**
	 * Creates a new DataTransferAE
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this DataTransferAE belongs
	 */
	public DataTransferAE createDataTransferAE(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Destroys an existing DataTransferAE
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this DataTransferAE belongs
	 */
	public void destroyDataTransferAE(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Get an existing DataTransferAE
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this DataTransferAE belongs
	 */
	public DataTransferAE getDataTransferAE(ApplicationProcessNamingInfo namingInfo);
}
