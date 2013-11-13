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
public class NMinusOneFlowAllocatedEvent extends BaseEvent{

	private FlowInformation flowInformation = null;
	
	public NMinusOneFlowAllocatedEvent(FlowInformation flowInformation) {
		super(Event.N_MINUS_1_FLOW_ALLOCATED);
		this.flowInformation = flowInformation;
	}

	public FlowInformation getFlowInformation(){
		return this.flowInformation;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Flow information: "+this.getFlowInformation()+ "\n";
		
		return result;
	}

}
