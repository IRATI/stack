package rina.cdap.impl.test;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.cdap.api.CDAPException;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.impl.CDAPSessionImpl;


public class ConnectDisconnectTest extends BaseCDAPTest{
	
	@Before
	public void setup(){
		super.setup();
	}
	
	@Test
	public void testConnectDisconnectWithResponse() throws CDAPException{	
		Assert.assertFalse(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertFalse(((CDAPSessionImpl)receivingCDAPSession).isConnected());
		
		connect();
		
		Assert.assertTrue(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertTrue(((CDAPSessionImpl)receivingCDAPSession).isConnected());
		
		disconnectWithResponse();
		
		Assert.assertFalse(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertFalse(((CDAPSessionImpl)receivingCDAPSession).isConnected());
	}
	
	@Test
	public void testConnectDisconnectWithoutResponse() throws CDAPException{
		Assert.assertFalse(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertFalse(((CDAPSessionImpl)receivingCDAPSession).isConnected());
		
		connect();
		
		Assert.assertTrue(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertTrue(((CDAPSessionImpl)receivingCDAPSession).isConnected());
		
		disconnectWithoutResponse();
		
		Assert.assertFalse(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertFalse(((CDAPSessionImpl)receivingCDAPSession).isConnected());
	}
	
	@Test
	public void testDisconnectWithoutConnecting() throws CDAPException{
		Assert.assertFalse(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertFalse(((CDAPSessionImpl)receivingCDAPSession).isConnected());

		boolean failed = false;

		try{
			disconnectWithoutResponse();
		}catch(Exception ex){
			System.out.println(ex.getMessage());
			failed = true;
		}

		Assert.assertTrue(failed);
	}
	
	@Test
	public void testConnectTwoTimesWithoutDisconnecting() throws CDAPException{
		Assert.assertFalse(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertFalse(((CDAPSessionImpl)receivingCDAPSession).isConnected());
		
		connect();
		
		Assert.assertTrue(((CDAPSessionImpl)sendingCDAPSession).isConnected());
		Assert.assertTrue(((CDAPSessionImpl)receivingCDAPSession).isConnected());
		
		boolean failed = false;
		
		try{
			connect();
		}catch(Exception ex){
			System.out.println(ex.getMessage());
			failed = true;
		}

		Assert.assertTrue(failed);
	}
	
	@Test
	public void testConnectDisconnectWithoutResponseSendingResponse() throws CDAPException{
		testConnectDisconnectWithoutResponse();
		
		boolean failed = false;
		CDAPMessage cdapMessage = null;
		
		cdapMessage = cdapSessionManager.getReleaseConnectionResponseMessage(32769, null, 0, null, 1);
		try{
			receivingCDAPSession.encodeNextMessageToBeSent(cdapMessage);
		}catch(Exception ex){
			System.out.println(ex.getMessage());
			failed = true;
		}
		Assert.assertTrue(failed);
		
		failed = false;
		try{
			receivingCDAPSession.messageSent(cdapMessage);
		}catch(Exception ex){
			System.out.println(ex.getMessage());
			failed = true;
		}
		Assert.assertTrue(failed);
	}
}
