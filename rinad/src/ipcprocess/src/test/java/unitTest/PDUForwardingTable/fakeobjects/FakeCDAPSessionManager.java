package unitTest.PDUForwardingTable.fakeobjects;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSession;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.AuthValue;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.AuthTypes;
import rina.cdap.api.message.CDAPMessage.Flags;
import rina.cdap.api.message.CDAPMessage.Opcode;
import rina.cdap.api.message.ObjectValue;

public class FakeCDAPSessionManager implements CDAPSessionManager{

	@Override
	public CDAPMessage decodeCDAPMessage(byte[] arg0) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public byte[] encodeCDAPMessage(CDAPMessage arg0) throws CDAPException {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		ObjectOutput out = null;
		try {
			out = new ObjectOutputStream(bos);   
			out.writeObject(arg0);
		} catch (IOException e) {
			e.printStackTrace();
		}
		byte[] yourBytes = bos.toByteArray();
		return yourBytes;
	}

	@Override
	public byte[] encodeNextMessageToBeSent(CDAPMessage arg0, int arg1)
			throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public int[] getAllCDAPSessionIds() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPSession getCDAPSession(int arg0) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getCancelReadRequestMessage(int arg0, Flags arg1,
			int arg2) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getCancelReadResponseMessage(int arg0, Flags arg1,
			int arg2, int arg3, String arg4) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getCreateObjectRequestMessage(int arg0, byte[] arg1,
			Flags arg2, String arg3, long arg4, String arg5, ObjectValue arg6,
			int arg7, boolean arg8) throws CDAPException {
		
			return null;
	}

	@Override
	public CDAPMessage getCreateObjectResponseMessage(int arg0, Flags arg1,
			String arg2, long arg3, String arg4, ObjectValue arg5, int arg6,
			String arg7, int arg8) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getDeleteObjectRequestMessage(int arg0, byte[] arg1,
			Flags arg2, String arg3, long arg4, String arg5, ObjectValue arg6,
			int arg7, boolean arg8) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getDeleteObjectResponseMessage(int arg0, Flags arg1,
			String arg2, long arg3, String arg4, int arg5, String arg6, int arg7)
			throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getOpenConnectionRequestMessage(int arg0,
			AuthTypes arg1, AuthValue arg2, String arg3, String arg4,
			String arg5, String arg6, String arg7, String arg8, String arg9,
			String arg10) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getOpenConnectionResponseMessage(int arg0,
			AuthTypes arg1, AuthValue arg2, String arg3, String arg4,
			String arg5, String arg6, int arg7, String arg8, String arg9,
			String arg10, String arg11, String arg12, int arg13)
			throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public int getPortId(String arg0) throws CDAPException {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public CDAPMessage getReadObjectRequestMessage(int arg0, byte[] arg1,
			Flags arg2, String arg3, long arg4, String arg5, int arg6,
			boolean arg7) throws CDAPException {
		
			CDAPMessage message = new CDAPMessage();
			message.setObjClass(arg3);
			message.setObjName(arg5);
			message.setOpCode(Opcode.M_READ);
			return message;
	}

	@Override
	public CDAPMessage getReadObjectResponseMessage(int arg0, Flags arg1,
			String arg2, long arg3, String arg4, ObjectValue arg5, int arg6,
			String arg7, int arg8) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getReleaseConnectionRequestMessage(int arg0, Flags arg1,
			boolean arg2) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getReleaseConnectionResponseMessage(int arg0,
			Flags arg1, int arg2, String arg3, int arg4) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getStartObjectRequestMessage(int arg0, byte[] arg1,
			Flags arg2, String arg3, ObjectValue arg4, long arg5, String arg6,
			int arg7, boolean arg8) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getStartObjectResponseMessage(int arg0, Flags arg1,
			int arg2, String arg3, int arg4) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getStartObjectResponseMessage(int arg0, Flags arg1,
			String arg2, ObjectValue arg3, long arg4, String arg5, int arg6,
			String arg7, int arg8) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getStopObjectRequestMessage(int arg0, byte[] arg1,
			Flags arg2, String arg3, ObjectValue arg4, long arg5, String arg6,
			int arg7, boolean arg8) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getStopObjectResponseMessage(int arg0, Flags arg1,
			int arg2, String arg3, int arg4) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage getWriteObjectRequestMessage(int arg0, byte[] arg1,
			Flags arg2, String arg3, long arg4, ObjectValue arg5, String arg6,
			int arg7, boolean arg8) throws CDAPException {
		 
			CDAPMessage message = new CDAPMessage();
			message.setObjValue(arg5);
			message.setOpCode(Opcode.M_WRITE);
			message.setObjName(arg6);
			message.setObjClass(arg3);
			return message;
	}

	@Override
	public CDAPMessage getWriteObjectResponseMessage(int arg0, Flags arg1,
			int arg2, String arg3, int arg4) throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPMessage messageReceived(byte[] arg0, int arg1)
			throws CDAPException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void messageSent(CDAPMessage arg0, int arg1) throws CDAPException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void removeCDAPSession(int arg0) {
		// TODO Auto-generated method stub
		
	}

}
