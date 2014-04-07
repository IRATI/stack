package rina.utils.apps.rinaperf.utils;

import eu.irati.librina.FlowDeallocatedEvent;

public interface FlowDeallocationListener {
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event);
}
