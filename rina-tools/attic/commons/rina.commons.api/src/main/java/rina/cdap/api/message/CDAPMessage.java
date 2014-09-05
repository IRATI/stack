package rina.cdap.api.message;

import java.io.Serializable;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPMessageValidator;


/**
 * CDAP message. Depending on the opcode, the following messages are possible:
 * M_CONNECT -> Common Connect Request. Initiate a connection from a source application to a destination application
 * M_CONNECT_R -> Common Connect Response. Response to M_CONNECT, carries connection information or an error indication
 * M_RELEASE -> Common Release Request. Orderly close of a connection
 * M_RELEASE_R -> Common Release Response. Response to M_RELEASE, carries final resolution of close operation
 * M_CREATE -> Create Request. Create an application object
 * M_CREATE_R -> Create Response. Response to M_CREATE, carries result of create request, including identification of the created object
 * M_DELETE -> Delete Request. Delete a specified application object
 * M_DELETE_R -> Delete Response. Response to M_DELETE, carries result of a deletion attempt
 * M_READ -> Read Request. Read the value of a specified application object
 * M_READ_R -> Read Response. Response to M_READ, carries part of all of object value, or error indication
 * M_CANCELREAD -> Cancel Read Request. Cancel a prior read issued using the M_READ for which a value has not been completely returned
 * M_CANCELREAD_R -> Cancel Read Response. Response to M_CANCELREAD, indicates outcome of cancellation
 * M_WRITE -> Write Request. Write a specified value to a specified application object
 * M_WRITE_R -> Write Response. Response to M_WRITE, carries result of write operation
 * M_START -> Start Request. Start the operation of a specified application object, used when the object has operational and non operational states
 * M_START_R -> Start Response. Response to M_START, indicates the result of the operation
 * M_STOP -> Stop Request. Stop the operation of a specified application object, used when the object has operational an non operational states
 * M_STOP_R -> Stop Response. Response to M_STOP, indicates the result of the operation.
 */
public class CDAPMessage implements Serializable{
	
	private static final long serialVersionUID = 1L;
	
	private static final int ABSTRACT_SYNTAX_VERSION = 0x0073;

	public enum Opcode {M_CONNECT, M_CONNECT_R, M_RELEASE, M_RELEASE_R, M_CREATE, M_CREATE_R, 
		M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_CANCELREAD, M_CANCELREAD_R, M_WRITE, 
		M_WRITE_R, M_START, M_START_R, M_STOP, M_STOP_R};
		
	public enum AuthTypes {AUTH_NONE, AUTH_PASSWD, AUTH_SSHRSA, AUTH_SSHDSA};
		
	public enum Flags {F_SYNC, F_RD_INCOMPLETE};
	
	/** 
	 * AbstractSyntaxID (int32), mandatory. The specific version of the 
	 * CDAP protocol message declarations that the message conforms to 
	 */
	private int absSyntax = 0;
	
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
	 * Filter (bytes). Filter predicate function to be used to determine whether an operation
	 * is to be applied to the designated object (s).
	 */
	private byte[] filter = null;
		
	/**
	 * flags (enm, int32), conditional, may be required by CDAP.
	 * Set of Boolean values that modify the meaning of a 
	 * message in a uniform way when true.
	 */
	private Flags flags = null;
	
	/**
	 * InvokeID, (int32). Unique identifier provided to identify a request, used to
	 * identify subsequent associated messages.
	 */
	private int invokeID = 0;
	
	/**
	 * ObjectClass (string). Identifies the object class definition of the 
	 * addressed object.
	 */
	private String objClass = null;
	
	/**
	 * ObjectInstance (int64). Object instance uniquely identifies a single object
	 * with a specific ObjectClass and ObjectName in an application's RIB. Either 
	 * the ObjectClass and ObjectName or this value may be used, if the ObjectInstance
	 * value is known. If a class and name are supplied in an operation, 
	 * an ObjectInstance value may be returned, and that may be used in future operations
	 * in lieu of objClass and objName for the duration of this connection.
	 */
	private long objInst = 0;
	
	/**
	 * ObjectName (string). Identifies a named object that the operation is 
	 * to be applied to. Object names identify a unique object of the designated 
	 * ObjectClass within an application.
	 */
	private String objName = null;
	
	/**
	 * ObjectValue (ObjectValue). The value of the object.
	 */
	private ObjectValue objValue = null;
		
	/**
	 * Opcode (enum, int32), mandatory.
	 * Message type of this message.
	 */
	private Opcode opCode = null;
	
	/**
	 * Result (int32). Mandatory in the responses, forbidden in the requests
	 * The result of an operation, indicating its success (which has the value zero, 
	 * the default for this field), partial success in the case of 
	 * synchronized operations, or reason for failure
	 */
	private int result = 0;
	
	/**
	 * Result-Reason (string), optional in the responses, forbidden in the requests
	 * Additional explanation of the result
	 */
	private String resultReason = null;
	
	/**
	 * Scope (int32). Specifies the depth of the object tree at 
	 * the destination application to which an operation (subject to filtering) 
	 * is to apply (if missing or present and having the value 0, the default, 
	 * only the targeted application's objects are addressed)
	 */
	private int scope = 0;
	
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
	private long version = 0;
	
	public CDAPMessage(){
	}
	
	public static CDAPMessage getOpenConnectionRequestMessage(AuthTypes authMech, 
			AuthValue authValue, String destAEInst, String destAEName, String destApInst,
			String destApName, String srcAEInst, String srcAEName, String srcApInst,
			String srcApName, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setAbsSyntax(ABSTRACT_SYNTAX_VERSION);
		cdapMessage.setAuthMech(authMech);
		cdapMessage.setAuthValue(authValue);
		cdapMessage.setDestAEInst(destAEInst);
		cdapMessage.setDestAEName(destAEName);
		cdapMessage.setDestApInst(destApInst);
		cdapMessage.setDestApInst(destApInst);
		cdapMessage.setDestApName(destApName);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setOpCode(Opcode.M_CONNECT);
		cdapMessage.setSrcAEInst(srcAEInst);
		cdapMessage.setSrcAEName(srcAEName);
		cdapMessage.setSrcApInst(srcApInst);
		cdapMessage.setSrcApName(srcApName);
		cdapMessage.setVersion(1);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getOpenConnectionResponseMessage(AuthTypes authMech, 
			AuthValue authValue, String destAEInst, String destAEName, String destApInst,
			String destApName, int result, String resultReason, String srcAEInst, String srcAEName, 
			String srcApInst, String srcApName, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setAbsSyntax(ABSTRACT_SYNTAX_VERSION);
		cdapMessage.setAuthMech(authMech);
		cdapMessage.setAuthValue(authValue);
		cdapMessage.setDestAEInst(destAEInst);
		cdapMessage.setDestAEName(destAEName);
		cdapMessage.setDestApInst(destApInst);
		cdapMessage.setDestApInst(destApInst);
		cdapMessage.setDestApName(destApName);
		cdapMessage.setOpCode(Opcode.M_CONNECT_R);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		cdapMessage.setSrcAEInst(srcAEInst);
		cdapMessage.setSrcAEName(srcAEName);
		cdapMessage.setSrcApInst(srcApInst);
		cdapMessage.setSrcApName(srcApName);
		cdapMessage.setVersion(1);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getReleaseConnectionRequestMessage(Flags flags) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setOpCode(Opcode.M_RELEASE);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getReleaseConnectionResponseMessage(Flags flags, int result, String resultReason, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setOpCode(Opcode.M_RELEASE_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getCreateObjectRequestMessage(byte[] filter, Flags flags, 
			String objClass, long objInst, String objName, ObjectValue objValue, int scope) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFilter(filter);
		cdapMessage.setFlags(flags);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objValue);
		cdapMessage.setOpCode(Opcode.M_CREATE);
		cdapMessage.setScope(scope);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getCreateObjectResponseMessage(Flags flags, String objClass, long objInst, String objName, ObjectValue objValue, int result,
			String resultReason, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objValue);
		cdapMessage.setOpCode(Opcode.M_CREATE_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getDeleteObjectRequestMessage(byte[] filter, Flags flags, String objClass, long objInst, String objName, 
			ObjectValue objectValue, int scope) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFilter(filter);
		cdapMessage.setFlags(flags);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objectValue);
		cdapMessage.setOpCode(Opcode.M_DELETE);
		cdapMessage.setScope(scope);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getDeleteObjectResponseMessage(Flags flags, String objClass, long objInst, String objName, int result, String resultReason, 
			int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setOpCode(Opcode.M_DELETE_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getStartObjectRequestMessage(byte[] filter, Flags flags, String objClass, ObjectValue objValue, long objInst, String objName, 
			int scope) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFilter(filter);
		cdapMessage.setFlags(flags);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objValue);
		cdapMessage.setOpCode(Opcode.M_START);
		cdapMessage.setScope(scope);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getStartObjectResponseMessage(Flags flags, int result, String resultReason, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setOpCode(Opcode.M_START_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getStartObjectResponseMessage(Flags flags, String objectClass, ObjectValue objectValue, long objInst, 
			String objName, int result, String resultReason, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setOpCode(Opcode.M_START_R);
		cdapMessage.setObjClass(objectClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objectValue);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getStopObjectRequestMessage(byte[] filter, Flags flags,
			String objClass, ObjectValue objValue, long objInst, String objName, int scope) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFilter(filter);
		cdapMessage.setFlags(flags);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objValue);
		cdapMessage.setOpCode(Opcode.M_STOP);
		cdapMessage.setScope(scope);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getStopObjectResponseMessage(Flags flags, int result, String resultReason, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setOpCode(Opcode.M_STOP_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getReadObjectRequestMessage(byte[] filter, Flags flags, String objClass, long objInst, String objName, int scope) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFilter(filter);
		cdapMessage.setFlags(flags);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setOpCode(Opcode.M_READ);
		cdapMessage.setScope(scope);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getReadObjectResponseMessage(Flags flags, String objClass, long objInst, String objName, ObjectValue objValue, 
			int result, String resultReason, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objValue);
		cdapMessage.setOpCode(Opcode.M_READ_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getWriteObjectRequestMessage(byte[] filter, Flags flags, String objClass, long objInst, ObjectValue objValue, String objName, 
			int scope) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFilter(filter);
		cdapMessage.setFlags(flags);
		cdapMessage.setObjClass(objClass);
		cdapMessage.setObjInst(objInst);
		cdapMessage.setObjName(objName);
		cdapMessage.setObjValue(objValue);
		cdapMessage.setOpCode(Opcode.M_WRITE);
		cdapMessage.setScope(scope);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getWriteObjectResponseMessage(Flags flags, int result, int invokeID, String resultReason) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setOpCode(Opcode.M_WRITE_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		cdapMessage.setInvokeID(invokeID);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getCancelReadRequestMessage(Flags flags, int invokeID) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setOpCode(Opcode.M_CANCELREAD);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	public static CDAPMessage getCancelReadResponseMessage(Flags flags, int invokeID, int result, String resultReason) throws CDAPException{
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setFlags(flags);
		cdapMessage.setInvokeID(invokeID);
		cdapMessage.setOpCode(Opcode.M_CANCELREAD_R);
		cdapMessage.setResult(result);
		cdapMessage.setResultReason(resultReason);
		CDAPMessageValidator.validate(cdapMessage);
		return cdapMessage;
	}
	
	/**
	 * Returns a reply message from the request message, copying all the fields except for: Opcode (it will be the 
	 * request message counterpart), result (it will be 0) and resultReason (it will be null)
	 * @param requestMessage
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getReplyMessage(){
		Opcode opcode = null;
		switch(this.getOpCode()){
		case M_CREATE:
			opcode = Opcode.M_CREATE_R;
			break;
		case M_CONNECT:
			opcode = Opcode.M_CONNECT_R;
			break;
		case M_DELETE:
			opcode = Opcode.M_DELETE_R;
			break;
		case M_RELEASE:
			opcode = Opcode.M_RELEASE_R;
			break;
		case M_START:
			opcode = Opcode.M_START_R;
			break;
		case M_STOP:
			opcode = Opcode.M_STOP_R;
			break;
		case M_READ:
			opcode = Opcode.M_READ_R;
			break;
		case M_WRITE:
			opcode = Opcode.M_WRITE_R;
			break;
		case M_CANCELREAD:
			opcode = Opcode.M_CANCELREAD_R;
			break;
		default:
			opcode = this.getOpCode();
		}
		
		CDAPMessage cdapMessage = new CDAPMessage();
		cdapMessage.setAbsSyntax(this.getAbsSyntax());
		cdapMessage.setAuthMech(this.getAuthMech());
		cdapMessage.setAuthValue(this.getAuthValue());
		cdapMessage.setDestAEInst(this.getDestAEInst());
		cdapMessage.setDestAEName(this.getDestAEName());
		cdapMessage.setDestApInst(this.getDestApInst());
		cdapMessage.setDestApName(this.getDestApName());
		cdapMessage.setFilter(this.getFilter());
		cdapMessage.setFlags(this.getFlags());
		cdapMessage.setInvokeID(this.getInvokeID());
		cdapMessage.setObjClass(this.getObjClass());
		cdapMessage.setObjInst(this.getObjInst());
		cdapMessage.setObjName(this.getObjName());
		cdapMessage.setObjValue(this.getObjValue());
		cdapMessage.setOpCode(opcode);
		cdapMessage.setResult(0);
		cdapMessage.setResultReason(null);
		cdapMessage.setScope(this.getScope());
		cdapMessage.setSrcAEInst(this.getSrcAEInst());
		cdapMessage.setSrcAEName(this.getSrcAEName());
		cdapMessage.setSrcApInst(this.getSrcApInst());
		cdapMessage.setSrcApName(this.getSrcApName());
		cdapMessage.setVersion(this.getVersion());
		
		return cdapMessage;
	}

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

	public byte[] getFilter() {
		return filter;
	}

	public void setFilter(byte[] filter) {
		this.filter = filter;
	}

	public Flags getFlags() {
		return flags;
	}

	public void setFlags(Flags flags) {
		this.flags = flags;
	}

	public int getInvokeID() {
		return invokeID;
	}

	public void setInvokeID(int invokeID) {
		this.invokeID = invokeID;
	}

	public String getObjClass() {
		return objClass;
	}

	public void setObjClass(String objClass) {
		this.objClass = objClass;
	}

	public long getObjInst() {
		return objInst;
	}

	public void setObjInst(long objInst) {
		this.objInst = objInst;
	}

	public String getObjName() {
		return objName;
	}

	public void setObjName(String objName) {
		this.objName = objName;
	}

	public ObjectValue getObjValue() {
		return objValue;
	}

	public void setObjValue(ObjectValue objValue) {
		this.objValue = objValue;
	}

	public Opcode getOpCode() {
		return opCode;
	}

	public void setOpCode(Opcode opCode) {
		this.opCode = opCode;
	}

	public int getResult() {
		return result;
	}

	public void setResult(int result) {
		this.result = result;
	}

	public String getResultReason() {
		return resultReason;
	}

	public void setResultReason(String resultReason) {
		this.resultReason = resultReason;
	}

	public int getScope() {
		return scope;
	}

	public void setScope(int scope) {
		this.scope = scope;
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
	
	public String toString(){
		String result = "\n"+this.getOpCode()+"\n";
		if (this.getOpCode() == Opcode.M_CONNECT|| this.getOpCode() == Opcode.M_CONNECT_R){
			result = result + "Abstract syntax: "+ this.getAbsSyntax() + "\n";
			result = result + "Authentication mechanism: "+ this.getAuthMech() + "\n";
			if (this.getAuthValue() != null){
				result = result + "Authentication value: "+ this.getAuthValue() + "\n";
			}
			if (this.getSrcApName() != null){
				result = result + "Source AP name: "+ this.getSrcApName() + "\n";
			}
			if (this.getSrcApInst() != null){
				result = result + "Source AP instance: "+ this.getSrcApInst() + "\n";
			}
			if (this.getSrcAEName() != null){
				result = result + "Source AE name: "+ this.getSrcAEName() + "\n";
			}
			if (this.getSrcAEInst() != null){
				result = result + "Source AE instance: "+ this.getSrcAEInst() + "\n";
			}
			if (this.getDestApName() != null){
				result = result + "Destination AP name: "+ this.getDestApName() + "\n";
			}
			if (this.getDestApInst() != null){
				result = result + "Destination AP instance: "+ this.getDestApInst() + "\n";
			}
			if (this.getDestAEName() != null){
				result = result + "Destination AE name: "+ this.getDestAEName() + "\n";
			}
			if (this.getDestAEInst() != null){
				result = result + "Destination AE instance: "+ this.getDestAEInst() + "\n";
			}
		}

		if (this.getFilter() != null){
			result = result + "Filter: "+ this.getFilter() + "\n";
		}

		if (this.getFlags() != null){
			result = result + "Flags: "+ this.getFlags() + "\n";
		}

		if (this.getInvokeID() != 0){
			result = result + "Invoke id: "+ this.getInvokeID() + "\n";
		}

		if (this.getObjClass() != null){
			result = result + "Object class: "+ this.getObjClass()+ "\n";
		}

		if (this.getObjName() != null){
			result = result + "Object name: "+ this.getObjName()+ "\n";
		}

		if (this.getObjInst() != 0){
			result = result + "Object instance: "+ this.getObjInst()+ "\n";
		}

		if (this.getObjValue() != null){
			result = result + "Object value: "+ this.getObjValue()+ "\n";
		}

		if (this.getOpCode() == Opcode.M_CONNECT_R || this.getOpCode() == Opcode.M_RELEASE_R 
				|| this.getOpCode() == Opcode.M_READ_R || this.getOpCode() == Opcode.M_WRITE_R 
				|| this.getOpCode() == Opcode.M_CANCELREAD_R || this.getOpCode() == Opcode.M_START_R 
				|| this.getOpCode() == Opcode.M_STOP_R || this.getOpCode() == Opcode.M_CREATE_R
				|| this.getOpCode() == Opcode.M_DELETE_R){
			result = result + "Result: "+ this.getResult()+ "\n";
			if (this.getResultReason() != null){
				result = result + "Result Reason: "+ this.getResultReason()+ "\n";
			}
		}

		if (this.getOpCode() == Opcode.M_READ || this.getOpCode() == Opcode.M_WRITE
				|| this.getOpCode() == Opcode.M_CANCELREAD || this.getOpCode() == Opcode.M_START
				|| this.getOpCode() == Opcode.M_STOP || this.getOpCode() == Opcode.M_CREATE
				|| this.getOpCode() == Opcode.M_DELETE){
			result = result + "Scope: "+ this.getScope()+ "\n";
		}
		
		if (this.getVersion() != 0){
			result = result + "Version: "+ this.getVersion() + "\n";
		}

		return result;
	}
}