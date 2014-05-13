package rina.utils.apps.rinaperf.utils;

import eu.irati.librina.AllocateFlowRequestResultEvent;

public interface FlowAllocationListener {
	public void dispatchAllocateFlowRequestResultEvent(AllocateFlowRequestResultEvent event);
}
