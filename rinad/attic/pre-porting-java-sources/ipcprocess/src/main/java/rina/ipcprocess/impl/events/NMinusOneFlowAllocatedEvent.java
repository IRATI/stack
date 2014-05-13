package rina.ipcprocess.impl.events;

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
	private long handle = -1;
	
	public NMinusOneFlowAllocatedEvent(FlowInformation flowInformation, long handle) {
		super(Event.N_MINUS_1_FLOW_ALLOCATED);
		this.flowInformation = flowInformation;
		this.handle = handle;
	}

	public FlowInformation getFlowInformation(){
		return this.flowInformation;
	}
	
	public long getHandle(){
		return handle;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Flow information: "+this.getFlowInformation()+ "\n";
		result = result + "Handle: " + this.getHandle() + "\n";
		
		return result;
	}

}
