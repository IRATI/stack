package rina.efcp.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

/**
 * Provides the component name for the Data Transfer AE
 * @author eduardgrasa
 *
 */
public abstract class BaseDataTransferAE extends BaseIPCProcessComponent implements DataTransferAE{
	
	public static final String getComponentName(){
		return DataTransferAE.class.getName();
	}
	
	public String getName(){
		return BaseDataTransferAE.getComponentName();
	}

}
