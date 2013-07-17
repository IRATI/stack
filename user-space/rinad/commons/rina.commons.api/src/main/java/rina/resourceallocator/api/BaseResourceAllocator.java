package rina.resourceallocator.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

public abstract class BaseResourceAllocator extends BaseIPCProcessComponent implements ResourceAllocator{

	public static final String getComponentName(){
		return ResourceAllocator.class.getName();
	}

	public String getName(){
		return BaseResourceAllocator.getComponentName();
	}

}
