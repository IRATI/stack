package rina.idd.api;

import java.util.List;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Maps application process names to the DIF from where they are available
 * @author eduardgrasa
 *
 */
public interface InterDIFDirectory {

	/**
	 * Maps application process names to the DIF from where they are available
	 * @param apNamingInfo The destination application process naming info (AP Name and optionally AP Instance)
	 * @return
	 */
	public List<String> mapApplicationProcessNamingInfoToDIFName(ApplicationProcessNamingInfo apNamingInfo);
	
	public void addMapping(ApplicationProcessNamingInfo application, String difName);
	
	public void addMapping(ApplicationProcessNamingInfo application, List<String> difNames);
	
	public void removeMapping(ApplicationProcessNamingInfo application, List<String> difNames);
	
	public void removeAllMappings(ApplicationProcessNamingInfo application);
}
