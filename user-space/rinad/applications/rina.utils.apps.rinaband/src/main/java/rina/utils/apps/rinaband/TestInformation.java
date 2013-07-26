package rina.utils.apps.rinaband;

/**
 * Contains the information for one test
 * @author eduardgrasa
 *
 */
public class TestInformation {

	/**
	 * The number of flows to open
	 */
	private int numberOfFlows = 0;
	
	/**
	 * True if client sends SDUs
	 */
	private boolean clientSendsSDUs = false;
	
	/**
	 * True if server sends SDUs
	 */
	private boolean serverSendsSDUs = false;
	
	/**
	 * The size of one SDU
	 */
	private int sduSize = 0;
	
	/**
	 * The number of SDUs per direction per flow
	 */
	private int numberOfSDUs = 0;
	
	/**
	 * Pattern of data written into each SDU
	 */
	private String pattern = null;
	
	/**
	 * The id of a QoS cube for the data flows
	 */
	private String qos = null;
	
	/**
	 * AEI specified by the server for the data entity
	 */
	private String aei = null;
	
	public TestInformation(){
	}

	public int getNumberOfFlows() {
		return numberOfFlows;
	}

	public void setNumberOfFlows(int numberOfFlows) {
		this.numberOfFlows = numberOfFlows;
	}

	public boolean isClientSendsSDUs() {
		return clientSendsSDUs;
	}

	public void setClientSendsSDUs(boolean clientSendsSDUs) {
		this.clientSendsSDUs = clientSendsSDUs;
	}

	public boolean isServerSendsSDUs() {
		return serverSendsSDUs;
	}

	public void setServerSendsSDUs(boolean serverSendsSDUs) {
		this.serverSendsSDUs = serverSendsSDUs;
	}

	public int getSduSize() {
		return sduSize;
	}

	public void setSduSize(int sduSize) {
		this.sduSize = sduSize;
	}

	public int getNumberOfSDUs() {
		return numberOfSDUs;
	}

	public void setNumberOfSDUs(int numberOfSDUs) {
		this.numberOfSDUs = numberOfSDUs;
	}

	public String getPattern() {
		return pattern;
	}

	public void setPattern(String pattern) {
		this.pattern = pattern;
	}

	public String getQos() {
		return qos;
	}

	public void setQos(String qos) {
		this.qos = qos;
	}

	public String getAei() {
		return aei;
	}

	public void setAei(String aei) {
		this.aei = aei;
	}
	
	public String toString(){
		String result = "";
		result = result + "Number of Flows: " + this.getNumberOfFlows() + "\n";
		result = result + "Number of SDUs per flow: " + this.getNumberOfSDUs() + "\n";
		result = result + "SDU size in bytes: " + this.getSduSize() + "\n";
		result = result + "Client sends SDUs: " + this.isClientSendsSDUs() + "\n";
		result = result + "Server sends SDUs: " + this.isServerSendsSDUs() + "\n";
		result = result + "Pattern: " + this.getPattern() + "\n";
		result = result + "QoS: " + this.getQos() + "\n";
		if (aei != null){
			result = result + "AEI: " + this.getAei() + "\n";
		}
		
		return result;
	}
}