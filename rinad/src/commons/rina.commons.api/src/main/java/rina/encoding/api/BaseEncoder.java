package rina.encoding.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

/**
 * Provides the component name for the Encoder
 * @author eduardgrasa
 *
 */
public abstract class BaseEncoder extends BaseIPCProcessComponent implements Encoder {

	public static final String getComponentName(){
		return Encoder.class.getName();
	}
	
	public String getName() {
		return BaseEncoder.getComponentName();
	}

}
