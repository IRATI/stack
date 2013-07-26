package rina.ipcmanager.impl.test;

import java.util.ArrayList;
import java.util.List;

import rina.idd.api.InterDIFDirectory;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;

public class MockInterDIFDirectory implements InterDIFDirectory{

	public List<String> mapApplicationProcessNamingInfoToDIFName(ApplicationProcessNamingInfo apNamingInfo) {
		List<String> result = new ArrayList<String>();
		result.add("test.DIF");
		return result;
	}

	public void addMapping(ApplicationProcessNamingInfo arg0, String arg1) {
		// TODO Auto-generated method stub
		
	}

	public void addMapping(ApplicationProcessNamingInfo arg0, List<String> arg1) {
		// TODO Auto-generated method stub
		
	}

	public void removeAllMappings(ApplicationProcessNamingInfo arg0) {
		// TODO Auto-generated method stub
		
	}

	public void removeMapping(ApplicationProcessNamingInfo arg0,
			List<String> arg1) {
		// TODO Auto-generated method stub
		
	}

}
