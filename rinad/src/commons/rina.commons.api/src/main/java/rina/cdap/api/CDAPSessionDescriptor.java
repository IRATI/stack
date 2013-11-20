package rina.cdap.api;

import eu.irati.librina.ApplicationProcessNamingInformation;
import rina.cdap.api.message.AuthValue;
import rina.cdap.api.message.CDAPMessage.AuthTypes;

/**
 * Describes a CDAPSession, by identifying the source and destination application processes.
 * Note that "source" and "destination" are just placeholders, as both parties in a CDAP exchange have 
 * the same role, because the CDAP session is bidirectional. 
 * @author eduardgrasa
 *
 */
public class CDAPSessionDescriptor {
	
	/** 
	 * AbstractSyntaxID (int32), mandatory. The specific version of the 
	 * CDAP protocol message declarations that the message conforms to 
	 */
	private int absSyntax = -1;
	
	/**
	 * AuthenticationMechanismName (authtypes), optional, not validated by CDAP. 
	 * Identification of the method to be used by the destination application to
	 * authenticate the source application
	 */
	private AuthTypes authMech = null;
	
	/**
	 * AuthenticationValue (authvalue), optional, not validated by CDAP.
	 * Authentication information accompanying authMech, format and value 
	 * appropiate to the selected authMech
	 */
	private AuthValue authValue = null;
	
	/**
	 * DestinationApplication-Entity-Instance-Id (string), optional, not validated by CDAP.
	 * Specific instance of the Application Entity that the source application
	 * wishes to connect to in the destination application.
	 */
	private String destAEInst = null;
	
	/**
	 * DestinationApplication-Entity-Name (string), mandatory (optional for the response).
	 * Name of the Application Entity that the source application wishes
	 * to connect to in the destination application.
	 */
	private String destAEName = null;
	
	/**
	 * DestinationApplication-Process-Instance-Id (string), optional, not validated by CDAP.
	 * Name of the Application Process Instance that the source wishes to
	 * connect to a the destination.
	 */
	private String destApInst = null;
	
	/**
	 * DestinationApplication-Process-Name (string), mandatory (optional for the response).
	 * Name of the application process that the source application wishes to connect to 
	 * in the destination application
	 */
	private String destApName = null;
	
	/**
	 * SourceApplication-Entity-Instance-Id (string).
	 * AE instance within the application originating the message
	 */
	private String srcAEInst = null;
	
	/**
	 * SourceApplication-Entity-Name (string).
	 * Name of the AE within the application originating the message
	 */
	private String srcAEName = null;
	
	/**
	 * SourceApplication-Process-Instance-Id (string), optional, not validated by CDAP.
	 * Application instance originating the message
	 */
	private String srcApInst = null;
	
	/**
	 * SourceApplicatio-Process-Name (string), mandatory (optional in the response).
	 * Name of the application originating the message
	 */
	private String srcApName = null;
	
	/**
	 * Version (int32). Mandatory in connect request and response, optional otherwise.
	 * Version of RIB and object set to use in the conversation. Note that the 
	 * abstract syntax refers to the CDAP message syntax, while version refers to the 
	 * version of the AE RIB objects, their values, vocabulary of object id's, and 
	 * related behaviors that are subject to change over time. See text for details
	 * of use.
	 */
	private long version = -1;
	
	/**
	 * Uniquely identifies this CDAP session in this IPC process. It matches the portId
	 * of the (N-1) flow that supports the CDAP Session
	 */
	private int portId = 0;

	public int getAbsSyntax() {
		return absSyntax;
	}

	public void setAbsSyntax(int absSyntax) {
		this.absSyntax = absSyntax;
	}

	public AuthTypes getAuthMech() {
		return authMech;
	}

	public void setAuthMech(AuthTypes authMech) {
		this.authMech = authMech;
	}

	public AuthValue getAuthValue() {
		return authValue;
	}

	public void setAuthValue(AuthValue authValue) {
		this.authValue = authValue;
	}

	public String getDestAEInst() {
		return destAEInst;
	}

	public void setDestAEInst(String destAEInst) {
		this.destAEInst = destAEInst;
	}

	public String getDestAEName() {
		return destAEName;
	}

	public void setDestAEName(String destAEName) {
		this.destAEName = destAEName;
	}

	public String getDestApInst() {
		return destApInst;
	}

	public void setDestApInst(String destApInst) {
		this.destApInst = destApInst;
	}

	public String getDestApName() {
		return destApName;
	}

	public void setDestApName(String destApName) {
		this.destApName = destApName;
	}

	public String getSrcAEInst() {
		return srcAEInst;
	}

	public void setSrcAEInst(String srcAEInst) {
		this.srcAEInst = srcAEInst;
	}

	public String getSrcAEName() {
		return srcAEName;
	}

	public void setSrcAEName(String srcAEName) {
		this.srcAEName = srcAEName;
	}

	public String getSrcApInst() {
		return srcApInst;
	}

	public void setSrcApInst(String srcApInst) {
		this.srcApInst = srcApInst;
	}

	public String getSrcApName() {
		return srcApName;
	}

	public void setSrcApName(String srcApName) {
		this.srcApName = srcApName;
	}

	public long getVersion() {
		return version;
	}

	public void setVersion(long version) {
		this.version = version;
	}

	public int getPortId() {
		return portId;
	}

	public void setPortId(int portId) {
		this.portId = portId;
	}
	
	/**
	 * The source naming information is always the naming information of the local Application process
	 * @return
	 */
	public ApplicationProcessNamingInformation getSourceApplicationProcessNamingInfo(){
		ApplicationProcessNamingInformation apNamingInfo = 
				new ApplicationProcessNamingInformation(this.getSrcApName(), this.getSrcApInst());
		if (this.getSrcAEName() != null){
			apNamingInfo.setEntityName(this.getSrcAEName());
		}
		
		if (this.getSrcAEInst() != null){
			apNamingInfo.setEntityInstance(this.getSrcAEInst());
		}
		
		return apNamingInfo;
	}
	
	/**
	 * The destination naming information is always the naming information of the remote application process
	 * @return
	 */
	public ApplicationProcessNamingInformation getDestinationApplicationProcessNamingInfo(){
		ApplicationProcessNamingInformation apNamingInfo = 
				new ApplicationProcessNamingInformation(this.getDestApName(), this.getDestApInst());
		if (this.getDestAEName() != null){
			apNamingInfo.setEntityName(this.getDestAEName());
		}
		
		if (this.getDestAEInst() != null) {
			apNamingInfo.setEntityInstance(this.getDestAEInst());
		}
		
		return apNamingInfo;
	}
}