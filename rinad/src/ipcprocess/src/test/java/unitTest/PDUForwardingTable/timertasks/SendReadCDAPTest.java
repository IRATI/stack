package unitTest.PDUForwardingTable.timertasks;

import java.util.Timer;

import junit.framework.Assert;

import org.junit.Test;

import rina.ipcprocess.impl.PDUForwardingTable.timertasks.SendReadCDAP;
import unitTest.PDUForwardingTable.fakeobjects.FakeCDAPSessionManager;
import unitTest.PDUForwardingTable.fakeobjects.FakeRIBDaemon;

public class SendReadCDAPTest {

	@Test
	public void Run_SendCDAP_True() {
		FakeRIBDaemon rib = new FakeRIBDaemon();
		Timer timer = new Timer();
		
		timer.schedule(new SendReadCDAP(3, new FakeCDAPSessionManager(), rib, 1), 1);
		try {
		    Thread.sleep(1000);
		} catch(InterruptedException ex) {
		    Thread.currentThread().interrupt();
		}
		Assert.assertTrue(rib.waitingResponse);
	}

}
