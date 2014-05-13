package rina.events.api;

import java.util.List;

/**
 * Manages subscriptions to events
 * @author eduardgrasa
 *
 */
public interface EventManager {
	
	/**
	 * Subscribe to a single event
	 * @param eventId The id of the event
	 * @param eventListener The event listener
	 */
	public void subscribeToEvent(String eventId, EventListener eventListener);
	
	/**
	 * Subscribes to a list of events
	 * @param eventIds the list of event ids
	 * @param eventListener The event listener
	 */
	public void subscribeToEvents(List<String> eventIds, EventListener eventListener);
	
	/**
	 * Unubscribe from a single event
	 * @param eventId The id of the event
	 * @param eventListener The event listener
	 */
	public void unsubscribeFromEvent(String eventId, EventListener eventListener);
	
	/**
	 * Unsubscribe from a list of events
	 * @param eventIds the list of event ids
	 * @param eventListener The event listener
	 */
	public void unsubscribeFromEvents(List<String> eventIds, EventListener eventListener);
	
	/**
	 * Invoked when a certain event has happened
	 * @param event
	 */
	public void deliverEvent(Event event);
}
