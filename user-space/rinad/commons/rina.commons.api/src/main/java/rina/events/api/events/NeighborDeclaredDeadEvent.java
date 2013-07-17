package rina.events.api.events;

import rina.enrollment.api.Neighbor;
import rina.events.api.BaseEvent;
import rina.events.api.Event;

/**
 * Called when all the connectivity to a neighbor has been lost
 * @author eduardgrasa
 *
 */
public class NeighborDeclaredDeadEvent extends BaseEvent{

	/**
	 * The neighbor to whom we've lost connectivity
	 */
	private Neighbor neighbor = null;
	
	public NeighborDeclaredDeadEvent(Neighbor neighbor) {
		super(Event.NEIGHBOR_DECLARED_DEAD);
		this.neighbor = neighbor;
	}

	public Neighbor getNeighbor() {
		return neighbor;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Neighbor: "+this.getNeighbor() + "\n";
		
		return result;
	}
}
