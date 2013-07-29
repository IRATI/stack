package rina.protection.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

public abstract class BaseSDUProtectionModuleRepository extends BaseIPCProcessComponent implements SDUProtectionModuleRepository{

	public static final String getComponentName(){
		return SDUProtectionModuleRepository.class.getName();
	}

	public String getName(){
		return BaseSDUProtectionModuleRepository.getComponentName();
	}
}
