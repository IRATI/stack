package rina.events.api;

/**
 * It is subscribed to events of certain type
 * @author eduardgrasa
 *
 */
public interface EventListener {
	/**
	 * Called when a acertain event has happened
	 * @param event
	 */
	public void eventHappened(Event event);
}
