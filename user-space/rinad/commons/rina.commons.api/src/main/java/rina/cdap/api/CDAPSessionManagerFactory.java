package rina.cdap.api;

/**
 * Create instances of CDAPSessionManagers
 * @author eduardgrasa
 *
 */
public interface CDAPSessionManagerFactory {
	
	public CDAPSessionManager createCDAPSessionManager();
}
