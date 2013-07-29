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
public class NMinusOneFlowDeallocatedEvent extends BaseEvent{

	/** The portId of the deallocated flow **/
	private int portId = 0;
	
	/**
	 * If the N-1 flow deallocated was a management flow, this is 
	 * the CDAP Session descriptor associated to it
	 */
	private CDAPSessionDescriptor cdapSessionDescriptor = null;
	
	public NMinusOneFlowDeallocatedEvent(int portId, CDAPSessionDescriptor cdapSessionDescriptor) {
		super(Event.N_MINUS_1_FLOW_DEALLOCATED);
		this.portId = portId;
		this.cdapSessionDescriptor = cdapSessionDescriptor;
	}

	public int getPortId() {
		return this.portId;
	}
	
	public CDAPSessionDescriptor getCdapSessionDescriptor() {
		return cdapSessionDescriptor;
	}

	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Port id: "+this.getPortId() + "\n";
		result = result + "CDAP Session descriptor: " + this.getCdapSessionDescriptor() + "\n";
		
		return result;
	}

}
