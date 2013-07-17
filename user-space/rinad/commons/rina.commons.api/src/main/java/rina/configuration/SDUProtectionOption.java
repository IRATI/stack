package rina.configuration;

/**
 * Specifies what SDU Protection Module should be used for each possible 
 * N-1 DIF (being NULL the default one)
 * @author eduardgrasa
 *
 */
public class SDUProtectionOption {
	
	private String nMinus1DIFName = null;
	private String sduProtectionType = null;
	
	public String getnMinus1DIFName() {
		return nMinus1DIFName;
	}
	public void setnMinus1DIFName(String nMinus1DIFName) {
		this.nMinus1DIFName = nMinus1DIFName;
	}
	public String getSduProtectionType() {
		return sduProtectionType;
	}
	public void setSduProtectionType(String sduProtectionType) {
		this.sduProtectionType = sduProtectionType;
	}

}
