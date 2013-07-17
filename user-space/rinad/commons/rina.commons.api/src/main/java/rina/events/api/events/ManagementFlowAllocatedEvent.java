package rina.events.api.events;

import rina.events.api.BaseEvent;
import rina.events.api.Event;

/**
 * Event that signals the deallocation of an 
 * N-1 Flow.
 * @author eduardgrasa
 *
 */
public class ManagementFlowAllocatedEvent extends BaseEvent{

	/** The portId of the allocated flow **/
	private int portId = 0;
	
	public ManagementFlowAllocatedEvent(int portId) {
		super(Event.MANAGEMENT_FLOW_ALLOCATED);
		this.portId = portId;
	}

	public int getPortId() {
		return this.portId;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Port id: "+this.getPortId() + "\n";
		
		return result;
	}

}
