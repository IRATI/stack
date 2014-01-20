package rina.utils;

/**
 * An interface of a class that is subscribed to queue ready to be written events
 * @author eduardgrasa
 *
 */
public interface QueueReadyToBeWrittenSubscriptor {

	/**
	 * Called when the queue transitions from full to having some capacity
	 * @param queueId the identifier of the queue
	 * @param inputOutput true if it is an input queue, false if it is an output queue
	 */
	public void queueReadyToBeWritten(int queueId, boolean inputOutput);
}
