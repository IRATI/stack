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
	
	/** Test duration */
	private int time = 0;
	
	/** Maximum transmission rate in kbps */
	private int rate = 0;
	
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
	
	public int getRate() {
		return rate;
	}

	public void setRate(int rate) {
		this.rate = rate;
	}

	public String toString(){
		String result = "";
		result = result + "SDU size in bytes: " + this.getSduSize() + "\n";
		result = result + "Test duration in seconds: " + this.getTime() + "\n";
		result = result + "Maximum sending rate in kbps: " + this.getRate() + "\n";
		
		return result;
	}
}