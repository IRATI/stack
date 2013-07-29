package rina.enrollment.api;

import com.google.common.primitives.UnsignedLongs;

import rina.ribdaemon.api.RIBObjectNames;

/**
 * A synonim for an application process name
 * @author eduardgrasa
 *
 */
public class Neighbor {
	
	public static final String NEIGHBOR_SET_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DAF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + RIBObjectNames.NEIGHBORS;
	
	public static final String NEIGHBOR_SET_RIB_OBJECT_CLASS = "neighbor set";
	
	public static final String NEIGHBOR_RIB_OBJECT_CLASS = "neighbor";

	/**
	 * The application process name that is synonym refers to
	 */
	private String applicationProcessName = null;
	
	/**
	 * The application process instance that this synonym refers to
	 */
	private String applicationProcessInstance = null;
	
	/**
	 * The address
	 */
	private long address = 0;
	
	/**
	 * Tells if it is enrolled or not
	 */
	private boolean enrolled = false;
	
	/**
	 * The average RTT in ms
	 */
	private long averageRTTInMs = 0L;
	
	/**
	 * The underlying portId used to communicate with this neighbor
	 */
	private int underlyingPortId = 0;
	
	/**
	 * The last time a KeepAlive message was received from
	 * that neighbor, in ms
	 */
	private long lastHeardFromTimeInMs = 0L;

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

	public long getAddress() {
		return address;
	}

	public void setAddress(long address) {
		this.address = address;
	}
	
	public boolean isEnrolled() {
		return enrolled;
	}

	public void setEnrolled(boolean enrolled) {
		this.enrolled = enrolled;
	}

	public long getAverageRTTInMs() {
		return averageRTTInMs;
	}

	public void setAverageRTTInMs(long averageRTTInMs) {
		this.averageRTTInMs = averageRTTInMs;
	}

	public int getUnderlyingPortId() {
		return underlyingPortId;
	}

	public void setUnderlyingPortId(int underlyingPortId) {
		this.underlyingPortId = underlyingPortId;
	}

	public long getLastHeardFromTimeInMs() {
		return lastHeardFromTimeInMs;
	}

	public void setLastHeardFromTimeInMs(long lastHeardFromTimeInMs) {
		this.lastHeardFromTimeInMs = lastHeardFromTimeInMs;
	}

	public String getKey(){
		String key = this.applicationProcessName+".";
		if (this.applicationProcessInstance != null){
			key = key + this.applicationProcessInstance;
		}else{
			key = key + "*";
		}
		
		return key;
	}
	
	@Override
	public String toString(){
		String result = "Application Process Name: " + this.applicationProcessName + "\n";
		result = result + "Application Process Instance: " + this.applicationProcessInstance + "\n";
		result = result + "Address: " + UnsignedLongs.toString(this.address) + "\n";
		result = result + "Enrolled: " + this.enrolled + "\n";
		result = result + "Underlying portId: " + this.underlyingPortId + "\n";
		result = result + "Average RTT: " + this.averageRTTInMs + " ms \n";
		result = result + "Last heard from time: " + this.lastHeardFromTimeInMs + " ms";
		
		return result;
	}
}
