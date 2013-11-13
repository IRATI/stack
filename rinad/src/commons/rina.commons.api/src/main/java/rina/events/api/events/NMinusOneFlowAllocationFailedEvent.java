package rina.events.api.events;

import eu.irati.librina.FlowInformation;
import rina.events.api.BaseEvent;
import rina.events.api.Event;

/**
 * Event that signals the deallocation of an 
 * N-1 Flow.
 * @author eduardgrasa
 *
 */
public class NMinusOneFlowAllocationFailedEvent extends BaseEvent{

	/** The portId of the denied flow **/
	private int portId = 0;
	
	/** The FlowService object describing the flow **/
	private FlowInformation flowInformation = null;
	
	/**
	 * The reason why the allocation failed
	 */
	private String resultReason = null;
	
	public NMinusOneFlowAllocationFailedEvent(int portId, 
			FlowInformation flowInformation, String resultReason) {
		super(Event.N_MINUS_1_FLOW_ALLOCATION_FAILED);
		this.portId = portId;
		this.flowInformation = flowInformation;
	}

	public int getPortId() {
		return this.portId;
	}
	
	public FlowInformation getFlowInformation() {
		return this.flowInformation;
	}
	
	public String getResultReason(){
		return this.resultReason;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Port id: "+this.getPortId() + "\n";
		result = result + "Flow description: " + this.getFlowInformation() + "\n";
		result = result + "Result reason: " + this.getResultReason() + "\n";
		
		return result;
	}

}
