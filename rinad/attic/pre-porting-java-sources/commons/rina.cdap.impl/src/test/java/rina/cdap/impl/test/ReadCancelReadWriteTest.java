package rina.cdap.impl.test;

import junit.framework.Assert;

import org.junit.Before;
import org.junit.Test;

import rina.cdap.api.CDAPException;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;

public class ReadCancelReadWriteTest extends BaseCDAPTest{
	
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
	public void testSingleWriteWithResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId = 0;
		
		cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, new ObjectValue(), "123", 0, true);
		invokeId = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getWriteObjectResponseMessage(32769, null, 0, null, invokeId);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testSingleWriteWithoutResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;

		cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, new ObjectValue(), "123", 0, false);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);

		receivingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testMultipleWriteWithResponses() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;

		cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, new ObjectValue(), "123", 0, true);
		int invokeId1 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, new ObjectValue(), "789", 0, true);
		int invokeId2 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getWriteObjectResponseMessage(32769, null, 0, null, invokeId1);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getWriteObjectResponseMessage(32769, null, 0, null, invokeId2);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testWriteDisconnected() throws CDAPException{
		this.disconnectWithoutResponse();
		CDAPMessage cdapMessage = null;

		boolean failed = false;
		cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", 0, new ObjectValue(), "123", 0, true);
		try{
			sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		}catch(CDAPException ex){
			System.out.println(ex.getMessage());
			failed = true;
		}

		Assert.assertTrue(failed);
	}

}
