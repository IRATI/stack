package rina.ribdaemon.api;

/**
 * Exceptions thrown by the RIB Daemon
 * TODO finish this class
 * @author eduardgrasa
 *
 */
public class RIBDaemonException extends Exception{

	private static final long serialVersionUID = 2358998897959323817L;
	
	/** Error codes **/
	public static final int UNKNOWN_OBJECT_CLASS = 1;
	public static final int MALFORMED_MESSAGE_SUBSCRIPTION_REQUEST = 2;
	public static final int MALFORMED_MESSAGE_UNSUBSCRIPTION_REQUEST = 3;
	public static final int SUBSCRIBER_WAS_NOT_SUBSCRIBED = 4;
	public static final int OBJECTCLASS_AND_OBJECT_NAME_OR_OBJECT_INSTANCE_NOT_SPECIFIED = 5;
	public static final int OBJECTNAME_NOT_PRESENT_IN_THE_RIB = 6;
	public static final int RESPONSE_REQUIRED_BUT_MESSAGE_HANDLER_IS_NULL = 7;
	public static final int PROBLEMS_SENDING_CDAP_MESSAGE = 8;
	public static final int OPERATION_NOT_ALLOWED_AT_THIS_OBJECT = 9;
	public static final int UNRECOGNIZED_OBJECT_NAME = 10;
	public static final int OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME = 11;
	public static final int OBJECT_ALREADY_HAS_THIS_CHILD = 12;
	public static final int CHILD_NOT_FOUND = 13;
	public static final int OBJECT_ALREADY_EXISTS = 14;
	public static final int RIB_OBJECT_AND_OBJECT_NAME_NULL = 15;
	public static final int PROBLEMS_DECODING_OBJECT = 16;
	public static final int OBJECT_VALUE_IS_NULL = 17;
	
	private int errorCode = 0;
	
	public RIBDaemonException(int errorCode){
		super();
		this.errorCode = errorCode;
	}
	
	public RIBDaemonException(int errorCode, String message){
		super(message);
		this.errorCode = errorCode;
	
	}
	
	public RIBDaemonException(int errorCode, Exception ex){
		super(ex);
		this.errorCode = errorCode;
	}

	public int getErrorCode() {
		return errorCode;
	}

	public void setErrorCode(int errorCode) {
		this.errorCode = errorCode;
	}
}
