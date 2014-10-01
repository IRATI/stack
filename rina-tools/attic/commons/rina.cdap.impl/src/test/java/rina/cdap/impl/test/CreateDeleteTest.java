package rina.cdap.impl.test;

import org.junit.Before;
import org.junit.Test;

import rina.cdap.api.CDAPException;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;

public class CreateDeleteTest extends BaseCDAPTest{
	
	@Before
	public void setup(){
		super.setup();
		try{
			connect();
		}catch(CDAPException ex){
			ex.printStackTrace();
		}
	}
	
	@Test
	public void testSingleCreateWithResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId = 0;
		
		cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "123", null, 0, true);
		invokeId = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getCreateObjectResponseMessage(32769, null, "org.pouzinsociety.flow.Flow", 0, "123", new ObjectValue(), 0, null, invokeId);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testSingleCreateWithoutResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;

		cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "123", null, 0, false);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);

		receivingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testMultipleCreateWithResponses() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId1 = 0;
		int invokeId2 = 0;

		cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "123", null, 0, true);
		invokeId1 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "976", null, 0, true);
		invokeId2 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getCreateObjectResponseMessage(32769, null, "org.pouzinsociety.flow.Flow", 0, "123", new ObjectValue(), 0, null, invokeId1);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getCreateObjectResponseMessage(32769, null, "org.pouzinsociety.flow.Flow", 0, "976", new ObjectValue(), 0, null, invokeId2);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testCreateDisconnected() throws CDAPException{
		this.disconnectWithoutResponse();
	}
	
	@Test
	public void testSingleDeleteWithResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		
		cdapMessage = cdapSessionManager.getDeleteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "123", null, 0, true);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getDeleteObjectResponseMessage(32769, null, "org.pouzinsociety.flow.Flow", 0, "123", 0, null, cdapMessage.getInvokeID());
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testSingleDeleteWithoutResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;

		cdapMessage = cdapSessionManager.getDeleteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "123", null, 0, false);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);

		receivingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testMultipleDeleteWithResponses() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId1 = 0;
		int invokeId2 = 0;

		cdapMessage = cdapSessionManager.getDeleteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "123", null, 0, true);
		invokeId1 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getDeleteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, "789", null, 0, true);
		invokeId2 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getDeleteObjectResponseMessage(32769, null, "org.pouzinsociety.flow.Flow", 0, "123", 0, null, invokeId1);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getDeleteObjectResponseMessage(32769, null, "org.pouzinsociety.flow.Flow", 0, "789", 0, null, invokeId2);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testDeleteDisconnected() throws CDAPException{
		this.disconnectWithoutResponse();
	}
}
