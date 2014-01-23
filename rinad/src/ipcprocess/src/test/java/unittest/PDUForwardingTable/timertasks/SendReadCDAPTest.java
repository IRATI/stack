package unittest.PDUForwardingTable.timertasks;

import java.util.Timer;

import junit.framework.Assert;

import org.junit.Test;

import eu.irati.librina.ApplicationProcessNamingInformation;

import rina.ipcprocess.impl.PDUForwardingTable.timertasks.SendReadCDAP;
import unitTest.PDUForwardingTable.FakeCDAPSessionManager;
import unitTest.PDUForwardingTable.FakeRIBDaemon;

public class SendReadCDAPTest {

//	@Test
	public void Run_SendCDAP_True() {
		FakeRIBDaemon rib = new FakeRIBDaemon();
		Timer timer = new Timer();
		
		timer.schedule(new SendReadCDAP(new ApplicationProcessNamingInformation(), 3, 2,
				new FakeCDAPSessionManager(), rib, 1), 1);
		
		Assert.assertNotNull(rib.recoveredObject);
	}

}
