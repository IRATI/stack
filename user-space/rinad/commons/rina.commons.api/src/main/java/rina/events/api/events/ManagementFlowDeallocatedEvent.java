package rina.events.api.events;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.events.api.BaseEvent;
import rina.events.api.Event;

/**
 * Event that signals the deallocation of an 
 * N-1 Flow.
 * @author eduardgrasa
 *
 */
public class ManagementFlowDeallocatedEvent extends BaseEvent{

	/** The portId of the deallocated flow **/
	private int portId = 0;
	
	/** The descriptor of the CDAP session running over the flow that has been deallocated**/
	private CDAPSessionDescriptor cdapSessionDescriptor = null;
	
	public ManagementFlowDeallocatedEvent(int portId, CDAPSessionDescriptor cdapSessionDescriptor) {
		super(Event.MANAGEMENT_FLOW_DEALLOCATED);
		this.portId = portId;
		this.cdapSessionDescriptor = cdapSessionDescriptor;
	}

	public int getPortId() {
		return this.portId;
	}

	public CDAPSessionDescriptor getCDAPSessionDescriptor() {
		return this.cdapSessionDescriptor;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Port id: "+this.getPortId() + "\n";
		result = result + "CDAP Session Descriptor: "+this.cdapSessionDescriptor.toString();
		
		return result;
	}

}
