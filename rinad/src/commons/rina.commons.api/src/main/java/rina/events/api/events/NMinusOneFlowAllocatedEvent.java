package rina.events.api.events;

import rina.events.api.BaseEvent;
import rina.events.api.Event;
import rina.resourceallocator.api.NMinus1FlowDescriptor;

/**
 * Event that signals the deallocation of an 
 * N-1 Flow.
 * @author eduardgrasa
 *
 */
public class NMinusOneFlowAllocatedEvent extends BaseEvent{

	private NMinus1FlowDescriptor nMinus1FlowDescriptor = null;
	
	public NMinusOneFlowAllocatedEvent(NMinus1FlowDescriptor nMinus1FlowDescriptor) {
		super(Event.N_MINUS_1_FLOW_ALLOCATED);
		this.nMinus1FlowDescriptor = nMinus1FlowDescriptor;
	}

	public NMinus1FlowDescriptor getNMinusOneFlowDescriptor(){
		return this.nMinus1FlowDescriptor;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "N-1 Flow Descriptor: "+this.getNMinusOneFlowDescriptor()+ "\n";
		
		return result;
	}

}
