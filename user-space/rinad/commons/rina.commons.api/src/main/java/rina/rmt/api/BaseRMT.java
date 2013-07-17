package rina.rmt.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

/**
 * Provides the component name for the RMT
 * @author eduardgrasa
 *
 */
public abstract class BaseRMT extends BaseIPCProcessComponent implements RMT {

	public static final String getComponentName(){
		return RMT.class.getName();
	}
	
	public String getName() {
		return BaseRMT.getComponentName();
	}

}
