package rina.utils.apps.echo;

/**
 * Contains the information for one test
 * @author eduardgrasa
 *
 */
public class TestInformation {
	
	/**
	 * The size of one SDU
	 */
	private int sduSize = 0;
	
	/**
	 * The number of SDUs
	 */
	private int numberOfSDUs = 0;
	
	/**
	 * The number of SDUs received
	 */
	private int sdusReceived = 0;
	
	/**
	 * The number of SDUs actually sent
	 */
	private int sdusSent = 0;
	
	private long firstSDUSentTime = 0;
	
	private long lastSDUReceivedTime = 0;
	
	public TestInformation(){
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
	
	public int getSDUsReceived() {
		return sdusReceived;
	}
	
	public void sduReceived() {
		sdusReceived ++;
	}
	
	public boolean receivedAllSDUs(){
		if (sdusReceived < numberOfSDUs) {
			return false;
		} else {
			return true;
		}
	}
	
	public long getFirstSDUSentTime() {
		return firstSDUSentTime;
	}

	public void setFirstSDUSentTime(long firstSDUSentTime) {
		this.firstSDUSentTime = firstSDUSentTime;
	}

	public long getLastSDUReceivedTime() {
		return lastSDUReceivedTime;
	}

	public void setLastSDUReceivedTime(long lastSDUReceivedTime) {
		this.lastSDUReceivedTime = lastSDUReceivedTime;
	}
	
	public synchronized int getSdusSent() {
		return sdusSent;
	}

	public synchronized void setSdusSent(int sdusSent) {
		this.sdusSent = sdusSent;
	}

	public String toString(){
		String result = "";
		result = result + "Number of SDUs: " + this.getNumberOfSDUs() + "\n";
		result = result + "SDU size in bytes: " + this.getSduSize() + "\n";
		
		return result;
	}
}