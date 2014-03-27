package rina.utils.apps.rinaperf;

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
	 * The number of SDUs received
	 */
	private int sdusReceived = 0;
	
	/**
	 * The number of SDUs actually sent
	 */
	private int sdusSent = 0;
	
	private long firstSDUSentTime = 0;
	
	private long lastSDUReceivedTime = 0;
	
	private int time = 0;
	
	public TestInformation(){
	}

	public int getSduSize() {
		return sduSize;
	}

	public void setSduSize(int sduSize) {
		this.sduSize = sduSize;
	}
	
	public int getSDUsReceived() {
		return sdusReceived;
	}
	
	public void sduReceived() {
		sdusReceived ++;
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
	
	public int getSdusSent() {
		return sdusSent;
	}

	public void setSdusSent(int sdusSent) {
		this.sdusSent = sdusSent;
	}

	public int getTime() {
		return time;
	}

	public void setTime(int time) {
		this.time = time;
	}

	public String toString(){
		String result = "";
		result = result + "SDU size in bytes: " + this.getSduSize() + "\n";
		result = result + "Test duration in seconds: " + this.getTime() + "\n";
		
		return result;
	}
}