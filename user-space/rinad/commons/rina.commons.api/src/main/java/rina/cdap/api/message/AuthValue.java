package rina.cdap.api.message;

/**
 * Encapsulates the data of an AuthValue
 * @author eduardgrasa
 *
 */
public class AuthValue {
	
	/**
	 * Authentication name
	 */
	private String authName = null;
	
	/**
	 * Authentication password
	 */
	private String authPassword = null;
	
	/**
	 * Additional authentication information
	 */
	private byte[] authOther = null;
	
	public String getAuthName() {
		return authName;
	}
	public void setAuthName(String authName) {
		this.authName = authName;
	}
	public String getAuthPassword() {
		return authPassword;
	}
	public void setAuthPassword(String authPassword) {
		this.authPassword = authPassword;
	}
	public byte[] getAuthOther() {
		return authOther;
	}
	public void setAuthOther(byte[] authOther) {
		this.authOther = authOther;
	}
}
