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
public class NMinusOneFlowAllocationFailedEvent extends BaseEvent{

	/** The portId of the denied flow **/
	private long handle = 0;
	
	/** The FlowService object describing the flow **/
	private FlowInformation flowInformation = null;
	
	/**
	 * The reason why the allocation failed
	 */
	private String resultReason = null;
	
	public NMinusOneFlowAllocationFailedEvent(long handle, 
			FlowInformation flowInformation, String resultReason) {
		super(Event.N_MINUS_1_FLOW_ALLOCATION_FAILED);
		this.handle = handle;
		this.flowInformation = flowInformation;
	}

	public long getHandle() {
		return handle;
	}
	
	public FlowInformation getFlowInformation() {
		return this.flowInformation;
	}
	
	public String getResultReason(){
		return this.resultReason;
	}
	
	@Override
	public String toString(){
		String result = "Event id: "+this.getId()+" \n";
		result = result + "Handle: "+this.getHandle() + "\n";
		result = result + "Flow description: " + this.getFlowInformation() + "\n";
		result = result + "Result reason: " + this.getResultReason() + "\n";
		
		return result;
	}

}
