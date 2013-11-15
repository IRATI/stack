package rina.ipcprocess.impl.enrollment.ribobjects;

public class NeighborStatistics {
	
	/**
	 * The application process name of the neighbor
	 */
	private String apName = null;
	
	/**
	 * Time when the read message was sent
	 */
	private long messageSentTimeInMs = 0L;
	
	public NeighborStatistics(String apName, long messageSentTimeInMs){
		this.apName = apName;
		this.messageSentTimeInMs = messageSentTimeInMs;
	}
	
	public String getApName() {
		return apName;
	}
	public void setApName(String apName) {
		this.apName = apName;
	}
	public long getMessageSentTimeInMs() {
		return messageSentTimeInMs;
	}
	public void setMessageSentTimeInMs(long messageSentTimeInMs) {
		this.messageSentTimeInMs = messageSentTimeInMs;
	}
	
	
}
