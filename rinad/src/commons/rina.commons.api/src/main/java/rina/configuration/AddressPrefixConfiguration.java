package rina.configuration;

/**
 * Assigns an address prefix to a certain substring (the organization)
 * that is part of the application process name
 * @author eduardgrasa
 *
 */
public class AddressPrefixConfiguration {

	public static final int MAX_ADDRESSES_PER_PREFIX = 16;
	
	/**
	 * The address prefix (it is the first valid address 
	 * for the given subdomain)
	 */
	private long addressPrefix = 0;
	
	/**
	 * The organization whose addresses start by the prefix
	 */
	private String organization = null;

	public long getAddressPrefix() {
		return addressPrefix;
	}

	public void setAddressPrefix(long addressPrefix) {
		this.addressPrefix = addressPrefix;
	}

	public String getOrganization() {
		return organization;
	}

	public void setOrganization(String organization) {
		this.organization = organization;
	} 
}
