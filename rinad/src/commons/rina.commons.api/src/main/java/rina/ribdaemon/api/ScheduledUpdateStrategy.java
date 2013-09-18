package rina.ribdaemon.api;

import java.util.Date;

/**
 * Update in the future, maybe periodically
 * @author eduardgrasa
 */
public class ScheduledUpdateStrategy {
	
	/**
	 * The first time to make the udpate
	 */
	private Date startTime =null;
	
	/**
	 * Period in ms. If the update is not periodic, period is 0.
	 */
	private long period = 0;

	public Date getStartTime() {
		return startTime;
	}

	public void setStartTime(Date startTime) {
		this.startTime = startTime;
	}

	public long getPeriod() {
		return period;
	}

	public void setPeriod(long period) {
		this.period = period;
	}
}
