package rina.flowallocator.api;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;

import com.google.common.primitives.UnsignedLongs;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.Connection;
import eu.irati.librina.FlowSpecification;

import rina.ribdaemon.api.RIBObjectNames;

/**
 * Encapsulates all the information required to manage a Flow
 * @author eduardgrasa
 *
 */
public class Flow {
	
	public static final String FLOW_SET_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + 
			RIBObjectNames.DIF + RIBObjectNames.SEPARATOR + RIBObjectNames.RESOURCE_ALLOCATION 
			+ RIBObjectNames.SEPARATOR + RIBObjectNames.FLOW_ALLOCATOR + RIBObjectNames.SEPARATOR 
			+ RIBObjectNames.FLOWS;
	
	public static final String FLOW_SET_RIB_OBJECT_CLASS = "flow set";
	
	public static final String FLOW_RIB_OBJECT_CLASS = "flow";
	
	public enum State {NULL, ALLOCATION_IN_PROGRESS, ALLOCATED, WAITING_2_MPL_BEFORE_TEARING_DOWN, DEALLOCATED};

	/**
	 * The application that requested the flow
	 */
	private ApplicationProcessNamingInformation sourceNamingInfo = null;
	
	/**
	 * The destination application of the flow
	 */
	private ApplicationProcessNamingInformation destinationNamingInfo = null;
	
	/**
	 * The port-id returned to the Application process that requested the flow. This port-id is used for 
	 * the life of the flow.
	 */
	private int sourcePortId = 0;
	
	/**
	 * The port-id returned to the destination Application process. This port-id is used for 
	 * the life of the flow.
	 */
	private int destinationPortId = 0;
	
	/**
	 * The address of the IPC process that is the source of this flow
	 */
	private long sourceAddress = 0;
	
	/**
	 * The address of the IPC process that is the destination of this flow
	 */
	private long destinationAddress = 0;
	
	/**
	 * All the possible connections of this flow
	 */
	private List<Connection> connections = null;
	
	/**
	 * The index of the connection that is currently Active in this flow
	 */
	private int currentConnectionIndex = 0;
	
	/**
	 * The status of this flow
	 */
	private State state = State.NULL;
	
	/**
	 * The list of parameters from the AllocateRequest that generated this flow
	 */
	private FlowSpecification flowSpec = null;
	
	/**
	 * The list of policies that are used to control this flow. NOTE: Does this provide 
	 * anything beyond the list used within the QoS-cube? Can we override or specialize those, 
	 * somehow?
	 */
	private Map<String, String> policies = null;
	
	/**
	 * The merged list of parameters from QoS.policy-Default-Parameters and QoS-Params.
	 */
	private Map<String, String> policyParameters = null;
	
	/**
	 * TODO this is just a placeHolder for this piece of data
	 */
	private byte[] accessControl = null;
	
	/**
	 * Maximum number of retries to create the flow before giving up.
	 */
	private int maxCreateFlowRetries = 0;
	
	/**
	 * The current number of retries
	 */
	private int createFlowRetries = 0;
	
	/** 
	 * While the search rules that generate the forwarding table should allow for a 
	 * natural termination condition, it seems wise to have the means to enforce termination.
	 */  
	private int hopCount = 0;
	
	/**
	 * True if this IPC process is the source of the flow, false otherwise
	 */
	private boolean source = false;
	
	public Flow(){
		this.connections = new ArrayList<Connection>();
		this.policies = new ConcurrentHashMap<String, String>();
		this.policyParameters = new ConcurrentHashMap<String, String>();
	}

	public boolean isSource() {
		return source;
	}

	public void setSource(boolean source) {
		this.source = source;
	}
	
	public ApplicationProcessNamingInformation getSourceNamingInfo() {
		return sourceNamingInfo;
	}

	public void setSourceNamingInfo(ApplicationProcessNamingInformation sourceNamingInfo) {
		this.sourceNamingInfo = sourceNamingInfo;
	}

	public ApplicationProcessNamingInformation getDestinationNamingInfo() {
		return destinationNamingInfo;
	}

	public void setDestinationNamingInfo(
			ApplicationProcessNamingInformation destinationNamingInfo) {
		this.destinationNamingInfo = destinationNamingInfo;
	}

	public int getSourcePortId() {
		return sourcePortId;
	}

	public void setSourcePortId(int sourcePortId) {
		this.sourcePortId = sourcePortId;
		if (connections != null) {
			for(int i=0; i<connections.size(); i++){
				connections.get(i).setPortId((int)sourcePortId);
			}
		}
	}

	public int getDestinationPortId() {
		return destinationPortId;
	}

	public void setDestinationPortId(int destinationPortId) {
		this.destinationPortId = destinationPortId;
	}

	public long getSourceAddress() {
		return sourceAddress;
	}

	public void setSourceAddress(long sourceAddress) {
		this.sourceAddress = sourceAddress;
		if (connections != null) {
			for(int i=0; i<connections.size(); i++){
				connections.get(i).setSourceAddress(sourceAddress);
			}
		}
	}

	public long getDestinationAddress() {
		return destinationAddress;
	}

	public void setDestinationAddress(long destinationAddress) {
		this.destinationAddress = destinationAddress;
		if (connections != null) {
			for(int i=0; i<connections.size(); i++){
				connections.get(i).setDestAddress(destinationAddress);
			}
		}
	}

	public List<Connection> getConnections() {
		return this.connections;
	}

	public void setConnections(List<Connection> connections) {
		this.connections = connections;
	}

	public int getCurrentConnectionIndex() {
		return currentConnectionIndex;
	}

	public void setCurrentConnectionIndex(int currentConnectionIndex) {
		this.currentConnectionIndex = currentConnectionIndex;
	}

	public State getState() {
		return state;
	}

	public void setState(State state) {
		this.state = state;
	}

	public FlowSpecification getFlowSpecification() {
		return flowSpec;
	}

	public void setFlowSpecification(FlowSpecification flowSpec) {
		this.flowSpec = flowSpec;
	}

	public Map<String, String> getPolicies() {
		return policies;
	}

	public void setPolicies(Map<String, String> policies) {
		this.policies = policies;
	}

	public Map<String, String> getPolicyParameters() {
		return policyParameters;
	}

	public void setPolicyParameters(Map<String, String> policyParameters) {
		this.policyParameters = policyParameters;
	}

	public byte[] getAccessControl() {
		return accessControl;
	}

	public void setAccessControl(byte[] accessControl) {
		this.accessControl = accessControl;
	}

	public int getMaxCreateFlowRetries() {
		return maxCreateFlowRetries;
	}

	public void setMaxCreateFlowRetries(int maxCreateFlowRetries) {
		this.maxCreateFlowRetries = maxCreateFlowRetries;
	}

	public int getCreateFlowRetries() {
		return createFlowRetries;
	}

	public void setCreateFlowRetries(int createFlowRetries) {
		this.createFlowRetries = createFlowRetries;
	}

	public int getHopCount() {
		return hopCount;
	}

	public void setHopCount(int hopCount) {
		this.hopCount = hopCount;
	}

	public String toString(){
		String result = "";
		result = result + "* State: "+this.getState() + "\n";
		result = result + "* Is this IPC Process the requestor of the flow? " + this.isSource() + "\n";
		result = result + "* Max create flow retries: " + this.getMaxCreateFlowRetries() + "\n";
		result = result + "* Hop count: " + this.getHopCount() + "\n";
		result = result + "* Source AP Naming Info: "+this.sourceNamingInfo;
		result = result + "* Source address: " + UnsignedLongs.toString(this.getSourceAddress()) + "\n";
		result = result + "* Source port id: " + UnsignedLongs.toString(this.getSourcePortId()) + "\n";
		result = result + "* Destination AP Naming Info: "+ this.getDestinationNamingInfo();
		result = result + "* Destination addres: " + UnsignedLongs.toString(this.getDestinationAddress()) + "\n";
		result = result + "* Destination port id: "+ UnsignedLongs.toString(this.getDestinationPortId()) + "\n";
		if (connections.size() > 0){
			result = result + "* Connection ids of the connection supporting this flow: +\n";
			for(int i=0; i<connections.size(); i++){
				result = result + "Src CEP-id " +connections.get(i).getSourceCepId() 
						+ "; Dest CEP-id " + connections.get(i).getDestCepId() 
						+ "; Qos-id " + connections.get(i).getQosId() + "\n";
			}
		}
		result = result + "* Index of the current active connection for this flow: "+this.currentConnectionIndex +"\n";
		if (!this.policies.isEmpty()){
			result = result + "* Policies: \n";
			Iterator<Entry<String, String>> iterator = this.policies.entrySet().iterator();
			Entry<String, String> currentEntry = null;
			while(iterator.hasNext()){
				currentEntry= iterator.next();
				result = result + "   * " + currentEntry.getKey()+ " = "+currentEntry.getValue() + "\n";
			}
		}
		if (!this.policyParameters.isEmpty()){
			result = result + "* Policy parameters: \n";
			Iterator<Entry<String, String>> iterator = this.policyParameters.entrySet().iterator();
			Entry<String, String> currentEntry = null;
			while(iterator.hasNext()){
				currentEntry = iterator.next();
				result = result + "   * " + currentEntry.getKey()+ " = "+currentEntry.getValue() + "\n";
			}
		}
		return result;
	}
}
