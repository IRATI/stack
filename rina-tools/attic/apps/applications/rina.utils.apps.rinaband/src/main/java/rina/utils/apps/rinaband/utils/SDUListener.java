package rina.utils.apps.rinaband.utils;

/**
 * Gets notified when a new SDU is delivered
 * @author eduardgrasa
 *
 */
public interface SDUListener {
	public void sduDelivered(byte[] sdu);
}
