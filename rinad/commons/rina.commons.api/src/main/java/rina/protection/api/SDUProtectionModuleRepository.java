package rina.protection.api;

import rina.ipcprocess.api.IPCProcessComponent;
import rina.ipcservice.api.IPCException;

/**
 * Maintains the implementation of SDU Protection modules
 * @author eduardgrasa
 *
 */
public interface SDUProtectionModuleRepository extends IPCProcessComponent{

	public static final String NULL = "NULL";
	public static final String HOPCOUNT = "HOPCOUNT";
	
	/**
	 * Return an instance of the SDU protection module whose type 
	 * matches the one provided in the operation's argument
	 * @param type the SDU Protection module type
	 * @return The instance of the SDU Protection module
	 * @throws IPCException if no instance of an SDU Protection module of a given type exists
	 */
	public SDUProtectionModule getSDUProtectionModule(String type) throws IPCException;
}
