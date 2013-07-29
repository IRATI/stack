package rina.ipcservice.api;

import java.util.Hashtable;
import java.util.Map;

/**
 * Encapsulates the parameters that define the level of service that this 
 * flow must accomplish
 * @author eduardgrasa
 *
 */
public class QualityOfServiceSpecification {
	
	/**
	 * The name of the qos cube, if known
	 */
	private String name = null;
	
	/**
	 * The id of the requested QoS cube, if known
	 */
	private int qosCubeId = -1;
	
	/**
	 * Average bandwidth in bytes/s, a value of 0 indicates 'don't care'
	 */
	private long averageBandwidth = 0;
	
	/**
	 * Average bandwidth in SDUs/s, a value of 0 indicates 'don't care'
	 */
	private long averageSDUBandwidth = 0;
	
	/**
	 * In milliseconds, a value of 0 indicates 'don't care'
	 */
	private int peakBandwidthDuration = 0;
	
	/**
	 * In milliseconds, a value of 0 indicates 'don't care'
	 */
	private int peakSDUBandwidthDuration = 0;
	
	/**
	 * A value of 0 indicates 'don't care'
	 */
	private double undetectedBitErrorRate = 0;
	
	/**
	 * Indicates if partial delivery of SDUs is allowed or not
	 */
	private boolean partialDelivery = true;

	/**
	 * Indicates if SDUs have to be delivered in order
	 */
	private boolean order = false;
	
	/**
	 * Indicates the maximum gap allowed in SDUs, a gap of N SDUs is considered the same as 
	 * all SDUs delivered. A value of -1 indicates 'Any'
	 */
	private int maxAllowableGapSDU = -1;
	
	/**
	 * In milliseconds, indicates the maximum delay allowed in this flow. A value of 
	 * 0 indicates 'don't care'
	 */
	private int delay = 0;
	
	/**
	 * In milliseconds, indicates the maximum jitter allowed in this flow. A value of 
	 * 0 indicates 'don't care'
	 */
	private int jitter = 0;
	
	/**
	 * Used to add extra parameters
	 */
	private Map<String, String> extendedPrameters = new Hashtable<String, String>();

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public int getQosCubeId() {
		return qosCubeId;
	}

	public void setQosCubeId(int qosCubeId) {
		this.qosCubeId = qosCubeId;
	}

	public Map<String, String> getExtendedPrameters() {
		return extendedPrameters;
	}

	public long getAverageBandwidth() {
		return averageBandwidth;
	}

	public void setAverageBandwidth(long averageBandwidth) {
		this.averageBandwidth = averageBandwidth;
	}

	public long getAverageSDUBandwidth() {
		return averageSDUBandwidth;
	}

	public void setAverageSDUBandwidth(long averageSDUBandwidth) {
		this.averageSDUBandwidth = averageSDUBandwidth;
	}

	public int getPeakBandwidthDuration() {
		return peakBandwidthDuration;
	}

	public void setPeakBandwidthDuration(int peakBandwidthDuration) {
		this.peakBandwidthDuration = peakBandwidthDuration;
	}

	public int getPeakSDUBandwidthDuration() {
		return peakSDUBandwidthDuration;
	}

	public void setPeakSDUBandwidthDuration(int peakSDUBandwidthDuration) {
		this.peakSDUBandwidthDuration = peakSDUBandwidthDuration;
	}

	public double getUndetectedBitErrorRate() {
		return undetectedBitErrorRate;
	}

	public void setUndetectedBitErrorRate(double undetectedBitErrorRate) {
		this.undetectedBitErrorRate = undetectedBitErrorRate;
	}

	public boolean isPartialDelivery() {
		return partialDelivery;
	}

	public void setPartialDelivery(boolean partialDelivery) {
		this.partialDelivery = partialDelivery;
	}

	public boolean isOrder() {
		return order;
	}

	public void setOrder(boolean order) {
		this.order = order;
	}

	public int getMaxAllowableGapSDU() {
		return maxAllowableGapSDU;
	}

	public void setMaxAllowableGapSDU(int maxAllowableGapSDU) {
		this.maxAllowableGapSDU = maxAllowableGapSDU;
	}

	public int getDelay() {
		return delay;
	}

	public void setDelay(int delay) {
		this.delay = delay;
	}

	public int getJitter() {
		return jitter;
	}

	public void setJitter(int jitter) {
		this.jitter = jitter;
	}
	
	public String toString(){
		String result = "Quality of service required:\n";
		if (this.getQosCubeId() != -1){
			result = result + "QoS cube id: " + this.getQosCubeId() + "\n";
		}
		if (this.getName() != null){
			result = result + "Name: " + this.getName() + "\n";
		}
		if (averageBandwidth != 0){
			result = result + "Average bandwidth: "+ averageBandwidth + " bytes/second \n";
		}
		if (averageSDUBandwidth != 0){
			result = result + "Average SDU bandwidth: "+ averageSDUBandwidth + " SDUs/second \n";
		}
		if (peakBandwidthDuration != 0){
			result = result + "Peak bandwidth duration: "+ peakBandwidthDuration + " miliseconds \n";
		}
		if (peakSDUBandwidthDuration != 0){
			result = result + "Peak SDU bandwidth duration: "+ peakSDUBandwidthDuration + " miliseconds \n";
		}
		if (undetectedBitErrorRate != 0){
			result = result + "Undetected bit error rate: "+ undetectedBitErrorRate + "\n";
		}
		result = result + "Partial delivery of SDUs accepted? " + partialDelivery + "\n";
		result = result + "SDUs delivered in order? " + order + "\n";
		if (maxAllowableGapSDU != -1){
			result = result + "Maximum gap allowed between SDUS: " + maxAllowableGapSDU + "\n";
		}else{
			result = result + "Maximum gap allowed between SDUS: Any \n";
		}
		if (delay != 0){
			result = result + "Maximum delayed allowed: "+ delay + " miliseconds \n";
		}
		if (jitter != 0){
			result = result + "Maximum jitter allowed: "+ jitter + " miliseconds \n";
		}
		
		return result;
	}
}
