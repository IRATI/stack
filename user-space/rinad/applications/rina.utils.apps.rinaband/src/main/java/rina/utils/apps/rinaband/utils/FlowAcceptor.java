package rina.utils.apps.rinaband.utils;

import eu.irati.librina.FlowRequestEvent;

public interface FlowAcceptor {
	public void dispatchFlowRequestEvent(FlowRequestEvent event);
}
