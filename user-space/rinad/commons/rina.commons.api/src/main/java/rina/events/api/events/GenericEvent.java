package rina.events.api.events;

import java.util.HashMap;
import java.util.Map;

import rina.events.api.BaseEvent;

/**
 * An event that can be used for different types 
 * of situations. Mainly thought for extensibility purposes
 * @author eduardgrasa
 *
 */
public class GenericEvent extends BaseEvent{

	/**
	 * Contains a set of key-value pairs that 
	 * provide information about the event
	 */
	private Map<String, Object> eventInformation = null;
	
	public GenericEvent(String id) {
		super(id);
		eventInformation = new HashMap<String, Object>();
	}

	public Map<String, Object> getEventInformation() {
		return eventInformation;
	}

	public void setEventInformation(Map<String, Object> eventInformation) {
		this.eventInformation = eventInformation;
	}
}
