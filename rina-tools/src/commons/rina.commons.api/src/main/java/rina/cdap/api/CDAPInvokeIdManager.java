package rina.cdap.api;

/**
 * Manages the invoke ids of a session.
 * @author eduardgrasa
 *
 */
public interface CDAPInvokeIdManager {
	
	/**
	 * Obtains a valid invoke id for this session
	 * @return
	 */
	public int getInvokeId();
	
	/**
	 * Allows an invoke id to be reused for this session
	 * @param invokeId
	 */
	public void freeInvokeId(int invokeId);
	
	/**
	 * Mark an invokeId as reserved (don't use it)
	 * @param invokeId
	 */
	public void reserveInvokeId(int invokeId);
}
