package rina.utils.apps.rinaperf.utils;

import eu.irati.librina.FlowRequestEvent;

public interface FlowAcceptor {
	public void dispatchFlowRequestEvent(FlowRequestEvent event);
}
