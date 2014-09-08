package rina.ribdaemon.api;

/**
 * Part of the RIB Daemon API to control if the changes have to be notified
 * @author eduardgrasa
 *
 */
public class NotificationPolicy {
	
	/**
	 * The neighbors that we don't have to notify
	 */
	private int[] cdapSessionIds = null;
	
	public NotificationPolicy(int[] cdapSessionIds){
		this.cdapSessionIds = cdapSessionIds;
	}

	public int[] getCdapSessionIds() {
		return cdapSessionIds;
	}

}
