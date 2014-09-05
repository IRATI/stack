package rina.utils.apps.rinaband.utils;

import eu.irati.librina.FlowDeallocatedEvent;

public interface FlowDeallocationListener {
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event);
}
