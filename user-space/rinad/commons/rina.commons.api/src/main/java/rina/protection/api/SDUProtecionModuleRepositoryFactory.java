package rina.protection.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Creates, destroys and stores instances of the SDUProtecionModuleRepository. Each 
 * instance of a SDUProtecionModuleRepository maps to an instance of an IPC Process 
 * @author eduardgrasa
 *
 */
public interface SDUProtecionModuleRepositoryFactory {
	
	/**
	 * Creates a new SDUProtecionModuleRepository
	 * @param namingInfo the name of the IPC process where this SDUProtecionModuleRepository belongs
	 */
	public SDUProtectionModuleRepository createSDUProtecionModuleRepository(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Destroys an existing SDUProtecionModuleRepository
	 * @param namingInfo the name of the IPC process where this SDUProtecionModuleRepositorybelongs
	 */
	public void destroySDUProtecionModuleRepository(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Get an existing SDUProtecionModuleRepository
	 * @param namingInfo the name of the IPC process where SDUProtecionModuleRepository belongs
	 */
	public SDUProtectionModuleRepository getSDUProtecionModuleRepository(ApplicationProcessNamingInfo namingInfo);
}
