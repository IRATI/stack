package rina.utils.apps.echo.utils;

import eu.irati.librina.FlowRequestEvent;

public interface FlowAcceptor {
	public void dispatchFlowRequestEvent(FlowRequestEvent event);
}
