package rina.cdap.impl.test;

import junit.framework.Assert;

import org.junit.Before;
import org.junit.Test;

import rina.cdap.api.CDAPException;
import rina.cdap.api.message.CDAPMessage;

public class StartStopTest extends BaseCDAPTest{
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
	public void testSingleStartWithResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId = 0;
		
		cdapMessage = cdapSessionManager.getStartObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, true);
		invokeId = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStartObjectResponseMessage(32769, null, 0, null, invokeId);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testSingleStartWithoutResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;

		cdapMessage = cdapSessionManager.getStartObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, false);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);

		receivingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testMultipleStartWithResponses() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId1 = 0;
		int invokeId2 = 0;

		cdapMessage = cdapSessionManager.getStartObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, true);
		invokeId1 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStartObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "789", 0, true);
		invokeId2 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStartObjectResponseMessage(32769, null, 0, null, invokeId1);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStartObjectResponseMessage(32769, null, 0, null, invokeId2);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testStartDisconnected() throws CDAPException{
		this.disconnectWithoutResponse();
		CDAPMessage cdapMessage = null;

		boolean failed = false;
		cdapMessage = cdapSessionManager.getStartObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, true);
		try{
			sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		}catch(CDAPException ex){
			System.out.println(ex.getMessage());
			failed = true;
		}

		Assert.assertTrue(failed);
	}
	
	@Test
	public void testSingleStopWithResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId = 0;
		
		cdapMessage = cdapSessionManager.getStopObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, true);
		invokeId = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStopObjectResponseMessage(32769, null, 0, null, invokeId);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testSingleStopWithoutResponse() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;

		cdapMessage = cdapSessionManager.getStopObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, false);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);

		receivingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testMultipleStopWithResponses() throws CDAPException{
		CDAPMessage cdapMessage = null;
		byte[] message = null;
		int invokeId1 = 0;
		int invokeId2 = 0;

		cdapMessage = cdapSessionManager.getStopObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, true);
		invokeId1 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStopObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "789", 0, true);
		invokeId2 = cdapMessage.getInvokeID();
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		receivingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStopObjectResponseMessage(32769, null, 0, null, invokeId1);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
		
		cdapMessage = cdapSessionManager.getStopObjectResponseMessage(32769, null, 0, null, invokeId2);
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		sendingCDAPSession.messageReceived(message);
	}
	
	@Test
	public void testStopDisconnected() throws CDAPException{
		this.disconnectWithoutResponse();
		CDAPMessage cdapMessage = null;

		boolean failed = false;
		cdapMessage = cdapSessionManager.getStopObjectRequestMessage(32768, null, null, "org.pouzinsociety.flow.Flow", null, 0, "123", 0, true);
		try{
			sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		}catch(CDAPException ex){
			System.out.println(ex.getMessage());
			failed = true;
		}

		Assert.assertTrue(failed);
	}
}
