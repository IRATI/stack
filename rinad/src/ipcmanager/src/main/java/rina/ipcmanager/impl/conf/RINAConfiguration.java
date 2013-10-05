package rina.ipcmanager.impl.conf;

import java.util.List;

import eu.irati.librina.DIFConfiguration;

/**
 * The global configuration of the RINA software
 * @author eduardgrasa
 *
 */
public class RINAConfiguration {
	
	/**
	 * The local software configuration (ports, timeouts, ...)
	 */
	private LocalConfiguration localConfiguration = null;
	
	/**
	 * The list of IPC Process to create when the software starts
	 */
	private List<IPCProcessToCreate> ipcProcessesToCreate = null;
	
	/**
	 * The configurations of zero or more DIFs
	 */
	private List<DIFProperties> difConfigurations = null;

	/**
	 * The single instance of this class
	 */
	private static RINAConfiguration instance = null;
	
	public static void setConfiguration(RINAConfiguration rinaConfiguration){
		instance = rinaConfiguration;
	}
	
	public static RINAConfiguration getInstance(){
		return instance;
	}
	
	public LocalConfiguration getLocalConfiguration() {
		return localConfiguration;
	}

	public List<IPCProcessToCreate> getIpcProcessesToCreate() {
		return ipcProcessesToCreate;
	}

	public void setIpcProcessesToCreate(
			List<IPCProcessToCreate> ipcProcessesToCreate) {
		this.ipcProcessesToCreate = ipcProcessesToCreate;
	}

	public void setLocalConfiguration(LocalConfiguration localConfiguration) {
		this.localConfiguration = localConfiguration;
	}

	public List<DIFProperties> getDifConfigurations() {
		return difConfigurations;
	}

	public void setDifConfigurations(List<DIFProperties> difConfigurations) {
		this.difConfigurations = difConfigurations;
	}
	
	/**
	 * Return the configuration of the DIF named "difName" if it is known, null 
	 * otherwise
	 * @param difName
	 * @return
	 */
	public DIFProperties getDIFConfiguration(String difName){
		if (difConfigurations == null){
			return null;
		}
		
		for(int i=0; i<difConfigurations.size(); i++){
			if (difConfigurations.get(i).getDifName().equals(difName)){
				return difConfigurations.get(i);
			}
		}
		
		return null;
	}
	
	public IPCProcessToCreate getIPCProcessToCreate(String apName, String apInstance){
		IPCProcessToCreate result = null;
		
		for(int i=0; i<this.getIpcProcessesToCreate().size(); i++){
			result = this.getIpcProcessesToCreate().get(i);
			if (result.getApplicationProcessName().equals(apName) && 
					result.getApplicationProcessInstance().equals(apInstance)){
				return result;
			}
		}
		
		return null;
	}
	
	public String toString(){
		return localConfiguration.toString();
	}
}
