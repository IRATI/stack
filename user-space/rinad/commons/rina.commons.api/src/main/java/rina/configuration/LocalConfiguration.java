package rina.configuration;

/**
 * Configuration of the local RINA Software instantiation
 * @author eduardgrasa
 *
 */
public class LocalConfiguration {

	/**
	 * The port where the IPC Manager is listening for incoming local TCP connections from administrators
	 */
	private int consolePort = 32766;
	
	/**
	 * The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	 */
	private int cdapTimeoutInMs = 10000;
	
	/**
	 * The maximum time to wait between steps of the enrollment sequence (in ms)
	 */
	private int enrollmentTimeoutInMs = 10000;
	
	/**
	 * The maximum time to wait to complete the flow allocation request 
	 * once the process has been initiated (in ms)
	 */
	private int flowAllocatorTimeoutInMs = 15000;
	
	/**
	 * The period of execution of the watchdog. The watchdog send an M_READ message over 
	 * all the active CDAP connections to make sure they are still alive.
	 */
	private int watchdogPeriodInMs = 60000;
	
	/**
	 * The period after which, if no keepAlive message from a neighbor IPC process has
	 * been received, it will be declared dead (and adequate action will be taken)
	 */
	private int declaredDeadIntervalInMs = 120000;
	
	/**
	 * The period of execution of the neighbors enroller. This task looks for known 
	 * neighbors in the RIB. If we're not enrolled to them, he is going to try to 
	 * initiate the enrollment.
	 */
	private int neighborsEnrollerPeriodInMs = 10000;
	
	/**
	 * The length of Flow queues
	 */
	private int lengthOfFlowQueues = 10;

	public int getConsolePort() {
		return consolePort;
	}

	public void setConsolePort(int consolePort) {
		this.consolePort = consolePort;
	}

	public int getCdapTimeoutInMs() {
		return cdapTimeoutInMs;
	}

	public void setCdapTimeoutInMs(int cdapTimeoutInMs) {
		this.cdapTimeoutInMs = cdapTimeoutInMs;
	}

	public int getEnrollmentTimeoutInMs() {
		return enrollmentTimeoutInMs;
	}

	public void setEnrollmentTimeoutInMs(int enrollmentTimeoutInMs) {
		this.enrollmentTimeoutInMs = enrollmentTimeoutInMs;
	}

	public int getFlowAllocatorTimeoutInMs() {
		return flowAllocatorTimeoutInMs;
	}

	public void setFlowAllocatorTimeoutInMs(int flowAllocatorTimeoutInMs) {
		this.flowAllocatorTimeoutInMs = flowAllocatorTimeoutInMs;
	}

	public int getWatchdogPeriodInMs() {
		return watchdogPeriodInMs;
	}

	public void setWatchdogPeriodInMs(int watchdogPeriodInMs) {
		this.watchdogPeriodInMs = watchdogPeriodInMs;
	}

	public int getDeclaredDeadIntervalInMs() {
		return declaredDeadIntervalInMs;
	}

	public void setDeclaredDeadIntervalInMs(int declaredDeadIntervalInMs) {
		this.declaredDeadIntervalInMs = declaredDeadIntervalInMs;
	}

	public int getNeighborsEnrollerPeriodInMs() {
		return neighborsEnrollerPeriodInMs;
	}

	public void setNeighborsEnrollerPeriodInMs(int neighborsEnrollerPeriodInMs) {
		this.neighborsEnrollerPeriodInMs = neighborsEnrollerPeriodInMs;
	}

	public int getLengthOfFlowQueues() {
		return lengthOfFlowQueues;
	}

	public void setLengthOfFlowQueues(int lengthOfFlowQueues) {
		this.lengthOfFlowQueues = lengthOfFlowQueues;
	}
}
