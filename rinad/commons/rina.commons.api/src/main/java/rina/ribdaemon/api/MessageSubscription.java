package rina.ribdaemon.api;

import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.api.message.CDAPMessage.Opcode;

/**
 * Subscription filter. The message subscriber will receive the CDAP messages that comply with the 
 * filter defined by the non-default attributes of this class. The default values are "null" for 
 * objects and '0' for numeric types.
 * @author eduardgrasa
 *
 */
public class MessageSubscription {

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
	
	public boolean equals(Object object){
		if (object == null){
			return false;
		}
		
		if (!(object instanceof MessageSubscription)){
			return false;
		}
		
		MessageSubscription subscription = (MessageSubscription) object;
		if (objectsAreDifferent(this.getDestAEInst(), subscription.getDestAEInst())){
			return false;
		}
		if (objectsAreDifferent(this.getDestAEName(), subscription.getDestAEName())){
			return false;
		}
		if (objectsAreDifferent(this.getDestApInst(), subscription.getDestApInst())){
			return false;
		}
		if (objectsAreDifferent(this.getDestApName(), subscription.getDestApName())){
			return false;
		}
		if (objectsAreDifferent(this.getObjClass(), subscription.getObjClass())){
			return false;
		}
		if (objectsAreDifferent(this.getObjName(), subscription.getObjName())){
			return false;
		}
		if (this.getObjInst() != subscription.getObjInst()){
			return false;
		}
		if (objectsAreDifferent(this.getObjValue(), subscription.getObjValue())){
			return false;
		}
		if (objectsAreDifferent(this.getOpCode(), subscription.getOpCode())){
			return false;
		}
		if (objectsAreDifferent(this.getSrcAEInst(), subscription.getSrcAEInst())){
			return false;
		}
		if (objectsAreDifferent(this.getSrcAEName(), subscription.getSrcAEName())){
			return false;
		}
		if (objectsAreDifferent(this.getSrcApInst(), subscription.getSrcApInst())){
			return false;
		}
		if (objectsAreDifferent(this.getSrcApName(), subscription.getSrcApName())){
			return false;
		}
		
		return true;
	}
	
	private boolean objectsAreDifferent(Object o1, Object o2){
		if (o1 == null && o2 != null){
			return true;
		}
		if (o1 != null && o2 == null){
			return true;
		}
		if (o1 != null && o2 != null && !o1.equals(o2)){
			return true;
		}

		return false;
	}
	
	/**
	 * Return true if this subscription matches the message, false otherwise
	 * @param cdapMessage
	 * @return
	 */
	public boolean isSubscribedToMessage(CDAPMessage cdapMessage){
		if (!filterMatches(this.destAEInst, cdapMessage.getDestAEInst())){
			return false;
		}
		if (!filterMatches(this.destAEName, cdapMessage.getDestAEName())){
			return false;
		}
		if (!filterMatches(this.destApInst, cdapMessage.getDestApInst())){
			return false;
		}
		if (!filterMatches(this.destApName, cdapMessage.getDestApName())){
			return false;
		}
		if (!filterMatches(this.objClass, cdapMessage.getObjClass())){
			return false;
		}
		if (!filterMatches(this.objValue, cdapMessage.getObjValue())){
			return false;
		}
		if (!filterMatches(this.objInst, cdapMessage.getObjInst())){
			return false;
		}
		if (!filterMatches(this.objName, cdapMessage.getObjName())){
			return false;
		}
		if (!filterMatches(this.opCode, cdapMessage.getOpCode())){
			return false;
		}
		if (!filterMatches(this.srcAEInst, cdapMessage.getSrcAEInst())){
			return false;
		}
		if (!filterMatches(this.srcAEName, cdapMessage.getSrcAEName())){
			return false;
		}
		if (!filterMatches(this.srcApInst, cdapMessage.getSrcApInst())){
			return false;
		}
		if (!filterMatches(this.srcApName, cdapMessage.getSrcApName())){
			return false;
		}
		
		return true;
	}
	
	/**
	 * If the filter field is not null, return if it equals to the 
	 * message field
	 * @param filterField
	 * @param messageField
	 * @return
	 */
	private boolean filterMatches(Object filterField, Object messageField){
		if (filterField == null){
			return true;
		}
		
		return filterField.equals(messageField);
	}
	
	public String toString(){
		String result = "Subscription: \n";
		
		if (this.getDestAEInst() != null){
			result = result + "Dest AE Inst: "+this.getDestAEInst() + "\n";
		}
		
		if (this.getDestAEName() != null){
			result = result + "Dest AE Name: "+this.getDestAEName() + "\n";
		}
		
		if (this.getDestApInst() != null){
			result = result + "Dest AP Inst: "+this.getDestApInst() + "\n";
		}
		
		if (this.getDestApName() != null){
			result = result + "Dest Ap Name: "+this.getDestApName() + "\n";
		}
		
		if (this.getObjClass() != null){
			result = result + "Obj class: "+this.getObjClass() + "\n";
		}
		
		if (this.getObjName() != null){
			result = result + "Obj name: "+this.getObjName() + "\n";
		}
		
		if (this.getObjValue() != null){
			result = result + "Obj value: "+this.getObjValue() + "\n";
		}
		
		if (this.getObjInst() != 0){
			result = result + "Obj inst: "+this.getObjInst() + "\n";
		}
		
		if (this.getOpCode() != null){
			result = result + "Opcode: "+this.getOpCode() + "\n";
		}
		
		if (this.getSrcAEInst() != null){
			result = result + "Src AE Inst: "+this.getSrcAEInst() + "\n";
		}
		
		if (this.getSrcAEName() != null){
			result = result + "Src AE Name: "+this.getSrcAEName() + "\n";
		}
		
		if (this.getSrcApInst() != null){
			result = result + "Src AP Inst: "+this.getSrcApInst() + "\n";
		}
		
		if (this.getSrcApName() != null){
			result = result + "Src AP Name: "+this.getSrcApName() + "\n";
		}
		
		return result;
	}
}
