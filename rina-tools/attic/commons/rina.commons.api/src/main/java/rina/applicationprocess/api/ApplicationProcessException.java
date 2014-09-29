package rina.applicationprocess.api;

/**
 * General exceptions thrown by Application Processes
 * @author eduardgrasa
 *
 */
public class ApplicationProcessException extends Exception{
	
	private static final long serialVersionUID = -8027759403935705744L;
	
	/** Error codes **/
	public static final int UNEXISTING_SYNOYM = 1;
	public static final int WRONG_APPLICATION_PROCES_NAME = 2;
	public static final int NULL_OR_MALFORMED_SYNONYM = 3;
	public static final int ALREADY_EXISTING_SYNOYM = 4;
	public static final int NULL_OR_MALFORMED_WHATEVERCAST_NAME = 5;
	public static final int ALREADY_EXISTING_WHATEVERCAST_NAME = 6;
	public static final int UNEXISTING_WHATEVERCAST_NAME = 7;
	
	private int errorCode = 0;
	
	public ApplicationProcessException(int errorCode){
		super();
		this.errorCode = errorCode;
	}
	
	public ApplicationProcessException(int errorCode, String message){
		super(message);
		this.errorCode = errorCode;
	
	}

	public int getErrorCode() {
		return errorCode;
	}

	public void setErrorCode(int errorCode) {
		this.errorCode = errorCode;
	}
}
