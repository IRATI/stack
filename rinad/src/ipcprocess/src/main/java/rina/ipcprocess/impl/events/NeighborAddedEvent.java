package rina.ipcprocess.impl.events;

import eu.irati.librina.Neighbor;
import rina.events.api.BaseEvent;
import rina.events.api.Event;

public class NeighborAddedEvent extends BaseEvent {

	/**
	 * The new neighbor
	 */
	private Neighbor neighbor = null;
	
	/**
	 * True if this IPC Process requested the enrollment operation,
	 * false if it was its neighbor.
	 */
	private boolean enrollee = false;
	
	public NeighborAddedEvent(Neighbor neighbor, boolean enrollee) {
		super(Event.NEIGHBOR_ADDED);
		this.neighbor = neighbor;
		this.enrollee = enrollee;
	}
	
	public Neighbor getNeighbor() {
		return neighbor;
	}
	
	public boolean getEnrollee(){
		return enrollee;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Neighbor: "+this.getNeighbor() + "\n";
		result = result + "Enrollee?: "+ this.getEnrollee() + "\n";
		
		return result;
	}
}
