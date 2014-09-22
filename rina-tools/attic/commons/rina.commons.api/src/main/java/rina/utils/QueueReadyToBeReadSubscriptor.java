package rina.utils;

/**
 * An interface of a class that is subscribed to queue ready to be read events
 * @author eduardgrasa
 *
 */
public interface QueueReadyToBeReadSubscriptor {

	/**
	 * Called when the queue has data to be read
	 * @param queueId the identifier of the queue
	 * @param inputOutput true if it is an input queue, false if it is an output queue
	 */
	public void queueReadyToBeRead(int queueId, boolean inputOutput);
}
