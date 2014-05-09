package rina.utils.apps.echo.utils;

import eu.irati.librina.AllocateFlowRequestResultEvent;

public interface FlowAllocationListener {
	public void dispatchAllocateFlowRequestResultEvent(AllocateFlowRequestResultEvent event);
}
