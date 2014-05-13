package rina.configuration;

import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;

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
	 * The list of application to DIF mappings
	 */
	private List<ApplicationToDIFMapping> applicationToDIFMappings = null;

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
	
	public List<ApplicationToDIFMapping> getApplicationToDIFMappings() {
		return applicationToDIFMappings;
	}

	public void setApplicationToDIFMappings(
			List<ApplicationToDIFMapping> applicationToDIFMappings) {
		this.applicationToDIFMappings = applicationToDIFMappings;
	}

	public static void setInstance(RINAConfiguration instance) {
		RINAConfiguration.instance = instance;
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
	
	/**
     * Return the address of the IPC process named "apName" if it is known, 
     * null otherwise
     * @param difName
     * @param apName
     * @param instance
     * @return
     */
	public KnownIPCProcessAddress getIPCProcessAddress(String difName, String apName, String apInstance){
		DIFProperties difConfiguration = this.getDIFConfiguration(difName);
		if (difConfiguration == null){
			return null;
		}

		List<KnownIPCProcessAddress> knownIPCProcessAddresses = difConfiguration.getKnownIPCProcessAddresses();
		if (knownIPCProcessAddresses == null){
			return null;
		}

		KnownIPCProcessAddress currentAddress = null;
		for(int i=0; i<knownIPCProcessAddresses.size(); i++){
			currentAddress = knownIPCProcessAddresses.get(i);
			if (currentAddress.getApName().equals(apName)){
				if (apInstance == null && currentAddress.getApInstance() == null){        
					return currentAddress;
				}else if (apInstance != null && currentAddress.getApInstance() != null && apInstance.equals(currentAddress.getApInstance())){
					return currentAddress;
				}
			}
		}

		return null;
	}
    
	/**
	 * Return the configuration of the IPC process whose address is "address" if it is known, 
	 * null otherwise
	 * @param difName
	 * @param address
	 * @return
	 */
	public KnownIPCProcessAddress getIPCProcessAddress(String difName, long address){
		DIFProperties difConfiguration = this.getDIFConfiguration(difName);
		if (difConfiguration == null){
			return null;
		}

		List<KnownIPCProcessAddress> knownIPCProcessAddresses = difConfiguration.getKnownIPCProcessAddresses();
		if (knownIPCProcessAddresses == null){
			return null;
		}

		for(int i=0; i<knownIPCProcessAddresses.size(); i++){
			if (knownIPCProcessAddresses.get(i).getAddress() == address){
				return knownIPCProcessAddresses.get(i);
			}
		}

		return null;
	}
	
	/**
     * Get the address prefix that corresponds to the application process name of the 
     * IPC Process. Return -1 if there is no matching.
     * @param difName
     * @param ipcProcessName
     * @return the address prefix
     */
	public long getAddressPrefixConfiguration(String difName, String apName){
		DIFProperties difConfiguration = this.getDIFConfiguration(difName);
		if (difConfiguration == null){
			return -1;
		}

		List<AddressPrefixConfiguration> addressPrefixes = difConfiguration.getAddressPrefixes();
		if (addressPrefixes == null){
			return -1;
		}

		for(int i=0; i<addressPrefixes.size(); i++){
			if (apName.indexOf(addressPrefixes.get(i).getOrganization()) != -1){
				return addressPrefixes.get(i).getAddressPrefix();
			}
		}

		return -1;
	}
	
	public String getDIFNameAssociatedToApName(ApplicationProcessNamingInformation apName){
		if (applicationToDIFMappings == null || apName == null) {
			return null;
		}
		
		String encodedApName = apName.getEncodedString();
		for(int i=0; i<applicationToDIFMappings.size(); i++) {
			if (encodedApName.equals(applicationToDIFMappings.get(i).getEncodedAppName())) {
				return applicationToDIFMappings.get(i).getDifName();
			}
		}
		
		return null;
	}
	
	public String toString(){
		return localConfiguration.toString();
	}
}
