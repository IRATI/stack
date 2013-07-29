package rina.events.api.events;

import rina.events.api.BaseEvent;
import rina.events.api.Event;

/**
 * Event that signals the creation of an EFCP Connection
 * @author eduardgrasa
 *
 */
public class EFCPConnectionCreatedEvent extends BaseEvent{

	/** The portId of the allocated flow **/
	private long connectionEndpointId = -1;
	
	public EFCPConnectionCreatedEvent(long connectionEndpointId) {
		super(Event.EFCP_CONNECTION_CREATED);
		this.connectionEndpointId = connectionEndpointId;
	}

	public long getConnectionEndpointId() {
		return this.connectionEndpointId;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Connection endpoint id: "+this.getConnectionEndpointId() + "\n";
		
		return result;
	}

}
