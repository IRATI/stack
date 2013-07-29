package rina.events.api;

/**
 * Base class common to all events
 * @author eduardgrasa
 *
 */
public class BaseEvent implements Event{

	/**
	 * The identity of the event
	 */
	private String id = null;
	
	public BaseEvent(String id){
		this.id = id;
	}
	
	public String getId() {
		return id;
	}

}
