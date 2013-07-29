package rina.delimiting.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

/**
 * Provides the component name for the Delimiter
 * @author eduardgrasa
 *
 */
public abstract class BaseDelimiter extends BaseIPCProcessComponent implements Delimiter{
	
	public static final String getComponentName(){
		return Delimiter.class.getName();
	}
	
	public String getName(){
		return BaseDelimiter.getComponentName();
	}

}
