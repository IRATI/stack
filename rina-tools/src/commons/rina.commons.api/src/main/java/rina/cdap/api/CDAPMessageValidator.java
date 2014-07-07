package rina.cdap.api;

import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.Opcode;


/**
 * Validates that a CDAP message is well formed
 */
public class CDAPMessageValidator{
	
	/**
	 * Validates a CDAP message
	 * @param message
	 * @throws CDAPException thrown if the CDAP message is not valid, indicating the reason
	 */
	public static void validate(CDAPMessage message) throws CDAPException{
		validateAbsSyntax(message);
		validateAuthMech(message);
		validateAuthValue(message);
		validateDestAEInst(message);
		validateDestAEName(message);
		validateDestApInst(message);
		validateDestApName(message);
		validateFilter(message);
		validateInvokeID(message);
		validateObjClass(message);
		validateObjInst(message);
		validateObjName(message);
		validateObjValue(message);
		validateOpcode(message);
		validateResult(message);
		validateResultReason(message);
		validateScope(message);
		validateSrcAEInst(message);
		validateSrcAEName(message);
		validateSrcApInst(message);
		validateSrcApName(message);
		validateVersion(message);
	}
	
	private static void validateAbsSyntax(CDAPMessage message) throws CDAPException{
		if (message.getAbsSyntax() == 0){
			if (message.getOpCode().equals(Opcode.M_CONNECT) || message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("AbsSyntax must be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}else{
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("AbsSyntax can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateAuthMech(CDAPMessage message) throws CDAPException{
		if (message.getAuthMech()!=null && !message.getAuthMech().equals(CDAPMessage.AuthTypes.AUTH_NONE)){
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("AuthMech can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateAuthValue(CDAPMessage message) throws CDAPException{
		if (message.getAuthValue() != null){
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("AuthValue can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateDestAEInst(CDAPMessage message) throws CDAPException{
		if (message.getDestAEInst() != null){
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("DestAEInst can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateDestAEName(CDAPMessage message) throws CDAPException{
		if (message.getDestAEName() != null){
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("DestAEName can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateDestApInst(CDAPMessage message) throws CDAPException{
		if (message.getDestApInst() != null){
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("DestApInst can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateDestApName(CDAPMessage message) throws CDAPException{
		if (message.getDestApName() == null){
			if (message.getOpCode().equals(Opcode.M_CONNECT)){ 
				throw new CDAPException("DestApName must be set for the M_CONNECT message", message);
			}else if (message.getOpCode().equals(Opcode.M_CONNECT_R)){
				//TODO not sure what to do
			}
		}else{
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("DestApName can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateFilter(CDAPMessage message) throws CDAPException{
		if (message.getFilter() != null){
			if (!message.getOpCode().equals(Opcode.M_CREATE) && !message.getOpCode().equals(Opcode.M_DELETE)
					&& !message.getOpCode().equals(Opcode.M_READ) && !message.getOpCode().equals(Opcode.M_WRITE)
					&& !message.getOpCode().equals(Opcode.M_START) && !message.getOpCode().equals(Opcode.M_STOP)){
				throw new CDAPException("The filter parameter can only be set for M_CREATE, M_DELETE, M_READ, " +
						"M_WRITE, M_START or M_STOP messages", message);
			}
		}
	}
	
	private static void validateInvokeID(CDAPMessage message) throws CDAPException{
		if (message.getInvokeID() == 0){
			if (message.getOpCode().equals(Opcode.M_CONNECT) || message.getOpCode().equals(Opcode.M_CONNECT_R) || 
					message.getOpCode().equals(Opcode.M_RELEASE_R)  || message.getOpCode().equals(Opcode.M_CREATE_R) ||
					message.getOpCode().equals(Opcode.M_DELETE_R) || message.getOpCode().equals(Opcode.M_READ_R) || 
					message.getOpCode().equals(Opcode.M_CANCELREAD) || message.getOpCode().equals(Opcode.M_CANCELREAD_R) || 
					message.getOpCode().equals(Opcode.M_WRITE_R) || message.getOpCode().equals(Opcode.M_START_R) ||
					message.getOpCode().equals(Opcode.M_STOP_R)){
			throw new CDAPException("The invoke id parameter cannot be 0", message);
			}
		}
	}
	
	private static void validateObjClass(CDAPMessage message) throws CDAPException{
		if (message.getObjClass() != null){
			if (message.getObjName() == null){
				throw new CDAPException("If the objClass parameter is set, the objName parameter also has to be set", message);
			}
			if (!message.getOpCode().equals(Opcode.M_CREATE) && !message.getOpCode().equals(Opcode.M_CREATE_R)
					&& !message.getOpCode().equals(Opcode.M_DELETE) && !message.getOpCode().equals(Opcode.M_DELETE_R)
					&& !message.getOpCode().equals(Opcode.M_READ) && !message.getOpCode().equals(Opcode.M_READ_R)
					&& !message.getOpCode().equals(Opcode.M_WRITE) && !message.getOpCode().equals(Opcode.M_WRITE_R) 
					&& !message.getOpCode().equals(Opcode.M_START) && !message.getOpCode().equals(Opcode.M_STOP)
					&& !message.getOpCode().equals(Opcode.M_START_R) && !message.getOpCode().equals(Opcode.M_STOP_R)){
				throw new CDAPException("The objClass parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, " +
				"M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R, M_STOP_R messages", message);
			}
		}
	}
	
	private static void validateObjInst(CDAPMessage message) throws CDAPException{
		if (message.getObjInst() != 0){
			if (!message.getOpCode().equals(Opcode.M_CREATE) && !message.getOpCode().equals(Opcode.M_CREATE_R)
				&& !message.getOpCode().equals(Opcode.M_DELETE) && !message.getOpCode().equals(Opcode.M_DELETE_R)
				&& !message.getOpCode().equals(Opcode.M_READ) && !message.getOpCode().equals(Opcode.M_READ_R)
				&& !message.getOpCode().equals(Opcode.M_WRITE) && !message.getOpCode().equals(Opcode.M_WRITE_R) 
				&& !message.getOpCode().equals(Opcode.M_START) && !message.getOpCode().equals(Opcode.M_STOP)
				&& !message.getOpCode().equals(Opcode.M_START_R) && !message.getOpCode().equals(Opcode.M_STOP_R)){
					throw new CDAPException("The objInst parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, " +
							"M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_START_R, M_STOP and M_STOP_R messages", message);
			}
		}
	}
	
	private static void validateObjName(CDAPMessage message) throws CDAPException{
		if (message.getObjName() != null){
			if (message.getObjClass() == null){
				throw new CDAPException("If the objName parameter is set, the objClass parameter also has to be set");
			}
			if (!message.getOpCode().equals(Opcode.M_CREATE) && !message.getOpCode().equals(Opcode.M_CREATE_R)
					&& !message.getOpCode().equals(Opcode.M_DELETE) && !message.getOpCode().equals(Opcode.M_DELETE_R)
					&& !message.getOpCode().equals(Opcode.M_READ) && !message.getOpCode().equals(Opcode.M_READ_R)
					&& !message.getOpCode().equals(Opcode.M_WRITE) && !message.getOpCode().equals(Opcode.M_WRITE_R) 
					&& !message.getOpCode().equals(Opcode.M_START) && !message.getOpCode().equals(Opcode.M_STOP)
					&& !message.getOpCode().equals(Opcode.M_START_R) && !message.getOpCode().equals(Opcode.M_STOP_R)){
				throw new CDAPException("The objName parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, " +
				"M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R and M_STOP_R messages", message);
			}
		}
	}
	
	private static void validateObjValue(CDAPMessage message) throws CDAPException{
		if (message.getObjValue() == null){
			if (message.getOpCode().equals(Opcode.M_WRITE)){
				throw new CDAPException("The objValue parameter must be set for M_WRITE messages", message);
			}
		}else{
			if (!message.getOpCode().equals(Opcode.M_CREATE) && !message.getOpCode().equals(Opcode.M_CREATE_R)
					&& !message.getOpCode().equals(Opcode.M_READ_R) && !message.getOpCode().equals(Opcode.M_WRITE)
					&& !message.getOpCode().equals(Opcode.M_START) && !message.getOpCode().equals(Opcode.M_STOP)
					&& !message.getOpCode().equals(Opcode.M_START_R) && !message.getOpCode().equals(Opcode.M_STOP_R)
					&& !message.getOpCode().equals(Opcode.M_WRITE_R) && !message.getOpCode().equals(Opcode.M_DELETE)
					&& !message.getOpCode().equals(Opcode.M_READ)){
				throw new CDAPException("The objValue parameter can only be set for M_CREATE, M_DELETE, M_CREATE_R, M_READ_R, " +
						"M_WRITE, M_START, M_START_R, M_STOP, M_STOP_R and M_WRITE_R messages", message);
			}
		}
	}
	
	private static void validateOpcode(CDAPMessage message) throws CDAPException{
		if (message.getOpCode() == null){
			throw new CDAPException("The opcode must be set for all the messages", message);
		}
	}
	
	private static void validateResult(CDAPMessage message) throws CDAPException{
	}
	
	private static void validateResultReason(CDAPMessage message) throws CDAPException{
		if (message.getResultReason() != null){
			if (!message.getOpCode().equals(Opcode.M_CREATE_R) && !message.getOpCode().equals(Opcode.M_DELETE_R)
					&& !message.getOpCode().equals(Opcode.M_READ_R) && !message.getOpCode().equals(Opcode.M_WRITE_R)
					&& !message.getOpCode().equals(Opcode.M_CONNECT_R) && !message.getOpCode().equals(Opcode.M_RELEASE_R)
					&& !message.getOpCode().equals(Opcode.M_CANCELREAD) && !message.getOpCode().equals(Opcode.M_CANCELREAD_R)
					&& !message.getOpCode().equals(Opcode.M_START_R) && !message.getOpCode().equals(Opcode.M_STOP_R)){
				throw new CDAPException("The resultReason parameter can only be set for M_CREATE_R, M_DELETE_R, M_READ_R, " +
						"M_WRITE_R, M_START_R, M_STOP_R, M_CONNECT_R, M_RELEASE_R, M_CANCELREAD and M_CANCELREAD_R messages", message);
			}
		}
	}
	
	private static void validateScope(CDAPMessage message) throws CDAPException{
		if (message.getScope() != 0){
			if (!message.getOpCode().equals(Opcode.M_CREATE) && !message.getOpCode().equals(Opcode.M_DELETE)
					&& !message.getOpCode().equals(Opcode.M_READ) && !message.getOpCode().equals(Opcode.M_WRITE)
					&& !message.getOpCode().equals(Opcode.M_START) && !message.getOpCode().equals(Opcode.M_STOP)){
				throw new CDAPException("The scope parameter can only be set for M_CREATE, M_DELETE, M_READ, " +
						"M_WRITE, M_START or M_STOP messages", message);
			}
		}
	}
	
	private static void validateSrcAEInst(CDAPMessage message) throws CDAPException{
		if (message.getSrcAEInst() != null){
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("SrcAEInst can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateSrcAEName(CDAPMessage message) throws CDAPException{
		if (message.getSrcAEName() == null){
			
		}else{
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateSrcApInst(CDAPMessage message) throws CDAPException{
		if (message.getSrcApInst() != null){
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("SrcApInst can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateSrcApName(CDAPMessage message) throws CDAPException{
		if (message.getSrcApName() == null){
			if (message.getOpCode().equals(Opcode.M_CONNECT)){ 
				throw new CDAPException("SrcApName must be set for the M_CONNECT message", message);
			}else if (message.getOpCode().equals(Opcode.M_CONNECT_R)){
				//TODO not sure what to do
			}
		}else{
			if (!message.getOpCode().equals(Opcode.M_CONNECT) && !message.getOpCode().equals(Opcode.M_CONNECT_R)){
				throw new CDAPException("SrcApName can only be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
	
	private static void validateVersion(CDAPMessage message) throws CDAPException{
		if (message.getVersion() == 0){
			if (message.getOpCode().equals(Opcode.M_CONNECT) || message.getOpCode().equals(Opcode.M_CONNECT_R)){ 
				throw new CDAPException("Version must be set for M_CONNECT and M_CONNECT_R messages", message);
			}
		}
	}
}
