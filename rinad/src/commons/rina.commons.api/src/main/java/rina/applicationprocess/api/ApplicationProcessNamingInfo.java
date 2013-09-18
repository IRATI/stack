package rina.applicationprocess.api;

import rina.ribdaemon.api.RIBObjectNames;

/**
 * All the elements needed to name an application process.
 */
public class ApplicationProcessNamingInfo {
	
	public static final String APPLICATION_PROCESS_NAMING_INFO_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DAF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + RIBObjectNames.NAMING + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.APNAME;

	public static final String APPLICATION_PROCESS_NAMING_INFO_RIB_OBJECT_CLASS = "apnaminginfo";
	
	public static final String SEPARATOR_1 = "#";
	public static final String SEPARATOR_2 = ":";
	
	private String applicationProcessName = null;
	private String applicationProcessInstance = null;
	private String applicationEntityName = null;
	private String applicationEntityInstance = null;
	
	public ApplicationProcessNamingInfo(){
	}
	
	public ApplicationProcessNamingInfo(String applicationProcessName, String applicationProcessInstance){
		this();
		this.applicationProcessName = applicationProcessName;
		this.applicationProcessInstance = applicationProcessInstance;
	}
	
	public ApplicationProcessNamingInfo(String applicationProcessName, String applicationProcessInstance, String applicationEntityName, String applicationEntityInstance){
		this();
		this.applicationProcessName = applicationProcessName;
		this.applicationProcessInstance = applicationProcessInstance;
		this.applicationEntityName = applicationEntityName;
		this.applicationEntityInstance = applicationEntityInstance;
	}
	
	/**
	 * Creates an instance of this class and initializes its attributes from an encoded string. The 
	 * string is encoded as follows: apname:apinstance#aename:aesintance where all parameters but 
	 * apname are optional.
	 * @param encodedAPNamingInfo
	 * @throws IllegalArgumentException
	 */
	public ApplicationProcessNamingInfo(String encodedAPNamingInfo) throws IllegalArgumentException{
		if (encodedAPNamingInfo == null){
			throw new IllegalArgumentException("Null String");
		}
		
		String[] substring1 = encodedAPNamingInfo.split(SEPARATOR_1);
		if (substring1.length > 2){
			throw new IllegalArgumentException("The "+SEPARATOR_1+" reserved character was used as part of " +
			"the application process naming information");
		}
		
		String[] substring2 = substring1[0].split(SEPARATOR_2);
		if (substring2.length > 2){
			throw new IllegalArgumentException("The "+SEPARATOR_2+" reserved character was used as part of " +
			"the application process naming information");
		}
		
		this.applicationProcessName = substring2[0];
		if (substring2.length == 2){
			this.applicationProcessInstance = substring2[1];
		}
		
		if (substring1.length == 2){
			substring2 = substring1[1].split(SEPARATOR_2);
			if (substring2.length > 2){
				throw new IllegalArgumentException("The "+SEPARATOR_2+" reserved character was used as part of " +
				"the application process naming information");
			}

			this.applicationEntityName = substring2[0];
			if (substring2.length == 2){
				this.applicationEntityInstance = substring2[1];
			}
		}
	}
	
	/**
	 * Return an encoded, human-readable string representing the application naming information.
	 * The encoded string will have the following syntax: apname:apinstance#aename:aesintance 
	 * @return
	 */
	public String getEncodedString(){
		String key = this.applicationProcessName;
		
		if (this.applicationProcessInstance != null){
			key = key + SEPARATOR_2 + this.applicationProcessInstance;
		}
		
		if (this.applicationEntityName != null){
			key = key + SEPARATOR_1 + this.applicationEntityName;
		}
		
		if (this.applicationEntityInstance != null){
			key = key + SEPARATOR_2 + this.applicationEntityInstance;
		}
		
		return key;
	}
	
	public String getApplicationProcessName() {
		return applicationProcessName;
	}

	public void setApplicationProcessName(String applicationProcessName) {
		this.applicationProcessName = applicationProcessName;
	}

	public String getApplicationProcessInstance() {
		return applicationProcessInstance;
	}

	public void setApplicationProcessInstance(String applicationProcessInstance) {
		this.applicationProcessInstance = applicationProcessInstance;
	}
	
	public String getApplicationEntityName() {
		return applicationEntityName;
	}

	public void setApplicationEntityName(String applicationEntityName) {
		this.applicationEntityName = applicationEntityName;
	}

	public String getApplicationEntityInstance() {
		return applicationEntityInstance;
	}

	public void setApplicationEntityInstance(String applicationEntityInstance) {
		this.applicationEntityInstance = applicationEntityInstance;
	}

	@Override
	public boolean equals(Object object){
		if (object == null){
			return false;
		}
		
		if (!(object instanceof ApplicationProcessNamingInfo)){
			return false;
		}
		
		ApplicationProcessNamingInfo candidate = (ApplicationProcessNamingInfo) object;
		
		return this.getEncodedString().equals(candidate.getEncodedString());
	}
	
	@Override
	public String toString(){
		String result = "Application Process Name: " + this.applicationProcessName + "\n";
		if (this.applicationProcessInstance != null){
			result = result + "Application Process Instance: " + this.getApplicationProcessInstance() + "\n";
		}
		if (this.applicationEntityName != null){
			result = result + "Application Entity name: " + this.getApplicationEntityName() + "\n";
		}
		if (this.applicationEntityInstance != null){
			result = result + "Application Entity instance: " + this.getApplicationEntityInstance() + "\n";
		}

		return result;
	}
	
	@Override
	public Object clone(){
		ApplicationProcessNamingInfo result = new ApplicationProcessNamingInfo();
		result.setApplicationProcessName(this.getApplicationProcessName());
		result.setApplicationProcessInstance(this.getApplicationProcessInstance());
		result.setApplicationEntityName(this.getApplicationEntityName());
		result.setApplicationEntityInstance(this.getApplicationEntityInstance());
		return result;
	}
	
}