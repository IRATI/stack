package rina.cdap.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

/**
 * Provides the component name for the CDAP Session Manager
 * @author eduardgrasa
 *
 */
public abstract class BaseCDAPSessionManager extends BaseIPCProcessComponent implements CDAPSessionManager{
	
	public static final String getComponentName(){
		return CDAPSessionManager.class.getName();
	}
	
	public String getName(){
		return BaseCDAPSessionManager.getComponentName();
	}
}
