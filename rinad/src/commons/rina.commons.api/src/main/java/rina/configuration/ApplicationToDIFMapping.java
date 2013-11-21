package rina.configuration;

public class ApplicationToDIFMapping {
	
	/**
	 * The application name encoded this way:
	 * processName-processInstance-entityName-entityInstance
	 */
	private String encodedAppName = null;
	
	/**
	 * The difName
	 */
	private String difName = null;

	public String getEncodedAppName() {
		return encodedAppName;
	}

	public void setEncodedAppName(String encodedAppName) {
		this.encodedAppName = encodedAppName;
	}

	public String getDifName() {
		return difName;
	}

	public void setDifName(String difName) {
		this.difName = difName;
	}
}
