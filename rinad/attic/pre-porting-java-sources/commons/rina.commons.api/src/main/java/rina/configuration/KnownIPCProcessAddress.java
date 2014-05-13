package rina.configuration;

/**
 * The configuration of a known IPC Process
 * @author eduardgrasa
 *
 */
public class KnownIPCProcessAddress {

	/**
	 * The application process name of the remote IPC Process
	 */
	private String apName = null;
	
	/**
	 * The application process instance of the remote IPC Process
	 */
	private String apInstance = null;
	
	/**
	 * The address of the remote IPC Process
	 */
	private long address = 0;
	
	public String getApName() {
		return apName;
	}

	public void setApName(String apName) {
		this.apName = apName;
	}

	public String getApInstance() {
		return apInstance;
	}

	public void setApInstance(String apInstance) {
		this.apInstance = apInstance;
	}

	public long getAddress() {
		return address;
	}

	public void setAddress(long address) {
		this.address = address;
	}
}
