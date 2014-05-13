package rina.enrollment.api;

import eu.irati.librina.EnrollToDIFRequestEvent;
import eu.irati.librina.Neighbor;

public class EnrollmentRequest {
	
	private Neighbor neighbor = null;
	private EnrollToDIFRequestEvent event = null;
	
	public EnrollmentRequest(Neighbor neighbor, EnrollToDIFRequestEvent event){
		this.neighbor = neighbor;
		this.event = event;
	}
	
	public Neighbor getNeighbor() {
		return neighbor;
	}
	public void setNeighbor(Neighbor neighbor) {
		this.neighbor = neighbor;
	}
	public EnrollToDIFRequestEvent getEvent() {
		return event;
	}
	public void setEvent(EnrollToDIFRequestEvent event) {
		this.event = event;
	}
}
