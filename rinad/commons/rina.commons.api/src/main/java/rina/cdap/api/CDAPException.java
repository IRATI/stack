package rina.cdap.api;

import rina.cdap.api.message.CDAPMessage;

public class CDAPException extends Exception{
	private static final long serialVersionUID = 1L;

	/**
	 * Name of the operation that failed
	 */
	private String operation = null;
	
	/**
	 * Operation result code
	 */
	private int result = 1;
	
	/**
	 * Result reason
	 */
	private String resultReason = null;
	
	/**
	 * The CDAPMessage that caused the exception
	 */
	private CDAPMessage cdapMessage = null;
	
	public CDAPException(Exception ex){
		super(ex);
	}
	
	public CDAPException(String operation, int result, String resultReason){
		super(resultReason);
		this.operation = operation;
		this.result = result;
		this.resultReason = resultReason;
	}
	
	public CDAPException(int result, String resultReason){
		this (null, result, resultReason);
	}
	
	public CDAPException(String resultReason){
		this(null, 1, resultReason);
	}
	
	public CDAPException(String resultReason, CDAPMessage cdapMessage){
		this(null, 1, resultReason);
		this.setCDAPMessage(cdapMessage);
	}

	public String getOperation() {
		return operation;
	}

	public void setOperation(String operation) {
		this.operation = operation;
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

	public CDAPMessage getCDAPMessage() {
		return cdapMessage;
	}

	public void setCDAPMessage(CDAPMessage cdapMessage) {
		this.cdapMessage = cdapMessage;
	}
}
