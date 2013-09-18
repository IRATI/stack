package rina.utils.apps.echo.utils;

import eu.irati.librina.FlowDeallocatedEvent;

public interface FlowDeallocationListener {
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event);
}
