package rina.cdap.impl.googleprotobuf;

import com.google.protobuf.ByteString;

import rina.cdap.api.CDAPException;
import rina.cdap.api.message.AuthValue;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.api.message.CDAPMessage.AuthTypes;
import rina.cdap.api.message.CDAPMessage.Flags;
import rina.cdap.api.message.CDAPMessage.Opcode;
import rina.cdap.impl.WireMessageProvider;
import rina.cdap.impl.googleprotobuf.CDAP.authTypes_t;
import rina.cdap.impl.googleprotobuf.CDAP.authValue_t;
import rina.cdap.impl.googleprotobuf.CDAP.flagValues_t;
import rina.cdap.impl.googleprotobuf.CDAP.objVal_t;
import rina.cdap.impl.googleprotobuf.CDAP.opCode_t;

public class GoogleProtocolBufWireMessageProvider implements WireMessageProvider{

	public CDAPMessage deserializeMessage(byte[] message) throws CDAPException {
		CDAP.CDAPMessage cdapMessage = null;
		
		try{
			cdapMessage = CDAP.CDAPMessage.parseFrom(message);
			AuthTypes authMech = getAuthMech(cdapMessage);
			AuthValue authValue = getAuthValue(cdapMessage);
			Flags flags = getFlags(cdapMessage);
			ObjectValue objValue = getObjValue(cdapMessage);
			Opcode opCode = getOpcode(cdapMessage);
			
			String destAEInst = cdapMessage.getDestAEInst();
			if (destAEInst.equals("")){
				destAEInst = null;
			}
			String destAEName = cdapMessage.getDestAEName();
			if (destAEName.equals("")){
				destAEName = null;
			}
			String destApInst = cdapMessage.getDestApInst();
			if (destApInst.equals("")){
				destApInst = null;
			}
			String destApName = cdapMessage.getDestApName();
			if (destApName.equals("")){
				destApName = null;
			}
			
			byte[] filter = null;
			if (!cdapMessage.getFilter().equals(ByteString.EMPTY)){
				filter = cdapMessage.getFilter().toByteArray();
			}
			
			String objClass = cdapMessage.getObjClass();
			if (objClass.equals("")){
				objClass = null;
			}
			String objName = cdapMessage.getObjName();
			if (objName.equals("")){
				objName = null;
			}
			String resultReason = cdapMessage.getResultReason();
			if (resultReason.equals("")){
				resultReason = null;
			}
			
			String srcAEInst = cdapMessage.getSrcAEInst();
			if (srcAEInst.equals("")){
				srcAEInst = null;
			}
			String srcAEName = cdapMessage.getSrcAEName();
			if (srcAEName.equals("")){
				srcAEName = null;
			}
			String srcApInst = cdapMessage.getSrcApInst();
			if (srcApInst.equals("")){
				srcApInst = null;
			}
			String srcApName = cdapMessage.getSrcApName();
			if (srcApName.equals("")){
				srcApName = null;
			}
			
			CDAPMessage response = new CDAPMessage();
			response.setAbsSyntax(cdapMessage.getAbsSyntax());
			response.setAuthMech(authMech);
			response.setAuthValue(authValue);
			response.setDestAEInst(destAEInst);
			response.setDestAEName(destAEName);
			response.setDestApInst(destApInst);
			response.setDestApName(destApName);
			response.setFilter(filter);
			response.setFlags(flags);
			response.setInvokeID(cdapMessage.getInvokeID());
			response.setObjClass(objClass);
			response.setObjInst(cdapMessage.getObjInst());
			response.setObjName(objName);
			response.setObjValue(objValue);
			response.setOpCode(opCode);
			response.setResult(cdapMessage.getResult());
			response.setResultReason(resultReason);
			response.setScope(cdapMessage.getScope());
			response.setSrcAEInst(srcAEInst);
			response.setSrcAEName(srcAEName);
			response.setSrcApInst(srcApInst);
			response.setSrcApName(srcApName);
			response.setVersion(cdapMessage.getVersion());

			return response;
		}catch(Exception ex){
			throw new CDAPException(ex);
		}
	}
	
	private AuthTypes getAuthMech(CDAP.CDAPMessage cdapMessage){
		AuthTypes authMech = null;
		
		if (cdapMessage.getAuthMech().equals(authTypes_t.AUTH_NONE)){
			authMech = AuthTypes.AUTH_NONE;
		}else if (cdapMessage.getAuthMech().equals(authTypes_t.AUTH_PASSWD)){
			authMech = AuthTypes.AUTH_PASSWD;
		}else if (cdapMessage.getAuthMech().equals(authTypes_t.AUTH_SSHDSA)){
			authMech = AuthTypes.AUTH_SSHDSA;
		}else if (cdapMessage.getAuthMech().equals(authTypes_t.AUTH_SSHRSA)){
			authMech = AuthTypes.AUTH_SSHRSA;
		}
		
		return authMech;
	}
	
	private AuthValue getAuthValue(CDAP.CDAPMessage cdapMessage){
		AuthValue authValue = null;
		String authName = null;
		String authPassword = null;
		
		if (cdapMessage.getAuthValue().equals(authValue_t.getDefaultInstance())){
			return null;
		}

		authValue = new AuthValue();
		authName = cdapMessage.getAuthValue().getAuthName();
		if (!authName.equals("")){
			authValue.setAuthName(authName);
		}
		if (!cdapMessage.getAuthValue().getAuthOther().equals(ByteString.EMPTY)){
			authValue.setAuthOther(cdapMessage.getAuthValue().getAuthOther().toByteArray());
		}
		authPassword = cdapMessage.getAuthValue().getAuthPassword();
		if (!authPassword.equals("")){
			authValue.setAuthPassword(authPassword);
		}

		return authValue;
	}
	
	private Flags getFlags(CDAP.CDAPMessage cdapMessage){
		Flags flags = null;
		
		if (cdapMessage.getFlags().equals(flagValues_t.F_RD_INCOMPLETE)){
			flags = Flags.F_RD_INCOMPLETE;
		}else if (cdapMessage.getFlags().equals(flagValues_t.F_SYNC)){
			flags = Flags.F_SYNC;
		}
		
		return flags;
	}
	
	private ObjectValue getObjValue(CDAP.CDAPMessage cdapMessage){
		ObjectValue objectValue = null;
		String strVal = null;
		
		if (cdapMessage.getObjValue().equals(objVal_t.getDefaultInstance())){
			return null;
		}
		
		objectValue = new ObjectValue();
		if (!cdapMessage.getObjValue().getByteval().equals(ByteString.EMPTY)){
			objectValue.setByteval(cdapMessage.getObjValue().getByteval().toByteArray());
		}
		objectValue.setDoubleval(cdapMessage.getObjValue().getDoubleval());
		objectValue.setFloatval(cdapMessage.getObjValue().getFloatval());
		objectValue.setInt64val(cdapMessage.getObjValue().getInt64Val());
		objectValue.setIntval(cdapMessage.getObjValue().getIntval());
		objectValue.setSint64val(cdapMessage.getObjValue().getSint64Val());
		objectValue.setSintval(cdapMessage.getObjValue().getSintval());
		objectValue.setBooleanval(cdapMessage.getObjValue().getBoolval());
		strVal = cdapMessage.getObjValue().getStrval();
		if (!strVal.equals("")){
			objectValue.setStrval(strVal);
		}
		
		return objectValue;
	}
	
	private Opcode getOpcode(CDAP.CDAPMessage cdapMessage){
		Opcode opcode = null;
		
		if (cdapMessage.getOpCode().equals(opCode_t.M_CANCELREAD)){
			opcode = Opcode.M_CANCELREAD;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_CANCELREAD_R)){
			opcode = Opcode.M_CANCELREAD_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_CONNECT)){
			opcode = Opcode.M_CONNECT;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_CONNECT_R)){
			opcode = Opcode.M_CONNECT_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_CREATE)){
			opcode = Opcode.M_CREATE;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_CREATE_R)){
			opcode = Opcode.M_CREATE_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_DELETE)){
			opcode = Opcode.M_DELETE;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_DELETE_R)){
			opcode = Opcode.M_DELETE_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_READ)){
			opcode = Opcode.M_READ;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_READ_R)){
			opcode = Opcode.M_READ_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_RELEASE)){
			opcode = Opcode.M_RELEASE;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_RELEASE_R)){
			opcode = Opcode.M_RELEASE_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_START)){
			opcode = Opcode.M_START;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_START_R)){
			opcode = Opcode.M_START_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_STOP)){
			opcode = Opcode.M_STOP;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_STOP_R)){
			opcode = Opcode.M_STOP_R;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_WRITE)){
			opcode = Opcode.M_WRITE;
		}else if (cdapMessage.getOpCode().equals(opCode_t.M_WRITE_R)){
			opcode = Opcode.M_WRITE_R;
		}
		
		return opcode;
	}

	public byte[] serializeMessage(CDAPMessage cdapMessage) throws CDAPException {
		try{
			authTypes_t authMech = getAuthMech(cdapMessage);
			authValue_t authValue = getAuthValue(cdapMessage);
			flagValues_t flags = getFlags(cdapMessage);
			objVal_t objValue = getObjValue(cdapMessage);
			opCode_t opCode = getOpCode(cdapMessage);
			ByteString filter = ByteString.EMPTY;
			if (cdapMessage.getFilter() != null){
				filter = ByteString.copyFrom(cdapMessage.getFilter());
			}
			String destAEInst = cdapMessage.getDestAEInst();
			if (destAEInst == null){
				destAEInst = "";
			}
			String destAEName = cdapMessage.getDestAEName();
			if (destAEName == null){
				destAEName = "";
			}
			String destApInst = cdapMessage.getDestApInst();
			if (destApInst == null){
				destApInst = "";
			}
			String destApName = cdapMessage.getDestApName();
			if (destApName == null){
				destApName = "";
			}
			String objClass = cdapMessage.getObjClass();
			if (objClass == null){
				objClass = "";
			}
			String objName = cdapMessage.getObjName();
			if (objName == null){
				objName = "";
			}
			String resultReason = cdapMessage.getResultReason();
			if (resultReason == null){
				resultReason = "";
			}
			String srcAEInst = cdapMessage.getSrcAEInst();
			if (srcAEInst == null){
				srcAEInst = "";
			}
			String srcAEName = cdapMessage.getSrcAEName();
			if (srcAEName == null){
				srcAEName = "";
			}
			String srcApInst = cdapMessage.getSrcApInst();
			if (srcApInst == null){
				srcApInst = "";
			}
			String srcApName = cdapMessage.getSrcApName();
			if (srcApName == null){
				srcApName = "";
			}

			CDAP.CDAPMessage response = null;

			response = CDAP.CDAPMessage.newBuilder().
			setAbsSyntax(cdapMessage.getAbsSyntax()).
			setAuthMech(authMech).
			setAuthValue(authValue).
			setDestAEInst(destAEInst).
			setDestAEName(destAEName).
			setDestApInst(destApInst).
			setDestApName(destApName).
			setFilter(filter).
			setFlags(flags).
			setInvokeID(cdapMessage.getInvokeID()).
			setObjClass(objClass).
			setObjInst(cdapMessage.getObjInst()).
			setObjName(objName).
			setObjValue(objValue).
			setOpCode(opCode).
			setResult(cdapMessage.getResult()).
			setResultReason(resultReason).
			setScope(cdapMessage.getScope()).
			setSrcAEInst(srcAEInst).
			setSrcAEName(srcAEName).
			setSrcApInst(srcApInst).
			setSrcApName(srcApName).
			setVersion(cdapMessage.getVersion()).
			build();

			return response.toByteArray();
		}catch(Exception ex){
			throw new CDAPException(ex);
		}
	}
	
	private authTypes_t getAuthMech(CDAPMessage cdapMessage){
		authTypes_t authMech = null;
		
		if (cdapMessage.getAuthMech() == null){
			authMech = authTypes_t.AUTH_NONE;
		}else if (cdapMessage.getAuthMech().equals(AuthTypes.AUTH_NONE)){
			authMech = authTypes_t.AUTH_NONE;
		}else if (cdapMessage.getAuthMech().equals(AuthTypes.AUTH_PASSWD)){
			authMech = authTypes_t.AUTH_PASSWD;
		}else if (cdapMessage.getAuthMech().equals(AuthTypes.AUTH_SSHDSA)){
			authMech = authTypes_t.AUTH_SSHDSA;
		}else if (cdapMessage.getAuthMech().equals(AuthTypes.AUTH_SSHRSA)){
			authMech = authTypes_t.AUTH_SSHRSA;
		}
		
		return authMech;
	}
	
	private authValue_t getAuthValue(CDAPMessage cdapMessage){
		authValue_t authValue = null;
		ByteString authOther = null;
		String authName = null;
		String authPassword = null;
		
		if (cdapMessage.getAuthValue() == null){
			return authValue_t.getDefaultInstance();
		}
		
		if (cdapMessage.getAuthValue().getAuthOther() != null){
			authOther = ByteString.copyFrom(cdapMessage.getAuthValue().getAuthOther());
		}else{
			authOther = ByteString.EMPTY;
		}
		
		authName = cdapMessage.getAuthValue().getAuthName();
		if (authName == null){
			authName = "";
		}
		
		authPassword = cdapMessage.getAuthValue().getAuthPassword();
		if (authPassword == null){
			authPassword = "";
		}
		
		authValue = CDAP.authValue_t.newBuilder().
			setAuthName(authName).
			setAuthPassword(authPassword).
			setAuthOther(authOther).
			build();
		
		return authValue;
	}
	
	private flagValues_t getFlags(CDAPMessage cdapMessage){
		flagValues_t flags = null;
		
		if (cdapMessage.getFlags() == null){
			flags = flagValues_t.F_NO_FLAGS;
		}else if (cdapMessage.getFlags().equals(Flags.F_RD_INCOMPLETE)){
			flags = flagValues_t.F_RD_INCOMPLETE;
		}else if (cdapMessage.getFlags().equals(Flags.F_SYNC)){
			flags = flagValues_t.F_SYNC;
		}
		
		return flags;
	}
	
	private objVal_t getObjValue(CDAPMessage cdapMessage){
		objVal_t objValue = null;
		ByteString byteVal = null;
		String strVal = null;
		
		if (cdapMessage.getObjValue() == null){
			return objVal_t.getDefaultInstance();
		}
		
		if (cdapMessage.getObjValue().getByteval() != null){
			byteVal = ByteString.copyFrom(cdapMessage.getObjValue().getByteval());
		}else{
			byteVal = ByteString.EMPTY;
		}
		
		strVal = cdapMessage.getObjValue().getStrval();
		if (strVal == null){
			strVal = "";
		}
		
		objValue = CDAP.objVal_t.newBuilder().setByteval(byteVal).
			setDoubleval(cdapMessage.getObjValue().getDoubleval()).
			setFloatval(cdapMessage.getObjValue().getFloatval()).
			setIntval(cdapMessage.getObjValue().getIntval()).
			setInt64Val(cdapMessage.getObjValue().getInt64val()).
			setSint64Val(cdapMessage.getObjValue().getSint64val()).
			setSintval(cdapMessage.getObjValue().getSintval()).
			setStrval(strVal).
			setBoolval(cdapMessage.getObjValue().isBooleanval()).
			build();
		
		return objValue;
	}
	
	private opCode_t getOpCode(CDAPMessage cdapMessage){
		opCode_t opCode = null;
		
		if (cdapMessage.getOpCode() == null){
			opCode = opCode_t.M_CONNECT;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_CANCELREAD)){
			opCode = opCode_t.M_CANCELREAD;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_CANCELREAD_R)){
			opCode = opCode_t.M_CANCELREAD_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_CONNECT)){
			opCode = opCode_t.M_CONNECT;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_CONNECT_R)){
			opCode = opCode_t.M_CONNECT_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_CREATE)){
			opCode = opCode_t.M_CREATE;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_CREATE_R)){
			opCode = opCode_t.M_CREATE_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_DELETE)){
			opCode = opCode_t.M_DELETE;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_DELETE_R)){
			opCode = opCode_t.M_DELETE_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_READ)){
			opCode = opCode_t.M_READ;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_READ_R)){
			opCode = opCode_t.M_READ_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_RELEASE)){
			opCode = opCode_t.M_RELEASE;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_RELEASE_R)){
			opCode = opCode_t.M_RELEASE_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_START)){
			opCode = opCode_t.M_START;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_START_R)){
			opCode = opCode_t.M_START_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_STOP)){
			opCode = opCode_t.M_STOP;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_STOP_R)){
			opCode = opCode_t.M_STOP_R;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_WRITE)){
			opCode = opCode_t.M_WRITE;
		}else if (cdapMessage.getOpCode().equals(Opcode.M_WRITE_R)){
			opCode = opCode_t.M_WRITE_R;
		}
		
		return opCode;
	}

}
