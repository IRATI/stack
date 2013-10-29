package rina.utils.apps.rinaband.utils;

import eu.irati.librina.AllocateFlowRequestResultEvent;

public interface FlowAllocationListener {
	public void dispatchAllocateFlowRequestResultEvent(AllocateFlowRequestResultEvent event);
}
