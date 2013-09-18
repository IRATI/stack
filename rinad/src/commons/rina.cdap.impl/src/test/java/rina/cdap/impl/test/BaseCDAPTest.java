package rina.cdap.impl.test;

import org.junit.Before;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.AuthTypes;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.CDAPSessionImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;

public abstract class BaseCDAPTest {
	
	protected CDAPSessionImpl sendingCDAPSession = null;
	protected CDAPSessionImpl receivingCDAPSession = null;
	protected CDAPSessionManager cdapSessionManager = new CDAPSessionManagerImpl(new GoogleProtocolBufWireMessageProviderFactory());
	
	@Before
	public void setup(){
		sendingCDAPSession = (CDAPSessionImpl) ((CDAPSessionManagerImpl)cdapSessionManager).createCDAPSession(32768);
		receivingCDAPSession = (CDAPSessionImpl) ((CDAPSessionManagerImpl)cdapSessionManager).createCDAPSession(32769);
	}
	
	protected void connect() throws CDAPException{
		byte[] message = null;
		CDAPMessage cdapMessage = null;
		
		cdapMessage = cdapSessionManager.getOpenConnectionRequestMessage(32768, AuthTypes.AUTH_NONE, null, null, "mock", null, "B", "234", "mock", "123", "A");
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		cdapMessage = receivingCDAPSession.messageReceived(message);
		cdapMessage = cdapSessionManager.getOpenConnectionResponseMessage(32769, AuthTypes.AUTH_NONE, null, "234", "mock", "123", "A", 0, null, "899", "mock", "677", "B", cdapMessage.getInvokeID());
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		cdapMessage = sendingCDAPSession.messageReceived(message);
	}
	
	protected void disconnectWithResponse() throws CDAPException{
		byte[] message = null;
		CDAPMessage cdapMessage = null;
		
		cdapMessage = cdapSessionManager.getReleaseConnectionRequestMessage(32768, null, true);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		cdapMessage = receivingCDAPSession.messageReceived(message);
		cdapMessage = cdapSessionManager.getReleaseConnectionResponseMessage(32769, null, 0, null, cdapMessage.getInvokeID());
		message = receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		receivingCDAPSession.messageSent(cdapMessage);
		
		cdapMessage = sendingCDAPSession.messageReceived(message);
	}
	
	protected void disconnectWithoutResponse() throws CDAPException{
		byte[] message = null;
		CDAPMessage cdapMessage = null;
		
		cdapMessage = cdapSessionManager.getReleaseConnectionRequestMessage(32768, null, false);
		message = sendingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		sendingCDAPSession.messageSent(cdapMessage);
		
		cdapMessage = receivingCDAPSession.messageReceived(message);
	}

}
