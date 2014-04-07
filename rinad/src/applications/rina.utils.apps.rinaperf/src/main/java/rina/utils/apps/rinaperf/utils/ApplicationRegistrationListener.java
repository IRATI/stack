package rina.utils.apps.rinaperf.utils;

import eu.irati.librina.RegisterApplicationResponseEvent;
import eu.irati.librina.UnregisterApplicationResponseEvent;

public interface ApplicationRegistrationListener {
	public void dispatchApplicationRegistrationResponseEvent(RegisterApplicationResponseEvent event);
	public void dispatchApplicationUnregistrationResponseEvent(UnregisterApplicationResponseEvent event);
}
