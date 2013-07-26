package rina.encoding.impl.googleprotobuf.applicationregistration.tests;

import java.util.ArrayList;
import java.util.List;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.applicationregistration.ApplicationRegistrationEncoder;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.ApplicationRegistration;

/**
 * Test if the serialization/deserialization mechanisms for the RegisterApplicationRequest object work
 * @author eduardgrasa
 *
 */
public class ApplicationRegistrationEncoderTest {
	
	private ApplicationRegistration applicationRegistration = null;
	private ApplicationRegistrationEncoder applicationRegistrationEncoder = null;
	
	@Before
	public void setup(){
		applicationRegistrationEncoder = new ApplicationRegistrationEncoder();
		
		applicationRegistration = new ApplicationRegistration();
		applicationRegistration.setApNamingInfo(new ApplicationProcessNamingInfo("a", "1"));
		applicationRegistration.setSocketNumber(23456);
		List<String> difNames = new ArrayList<String>();
		difNames.add("RINA-Demo.DIF");
		difNames.add("test.DIF");
		applicationRegistration.setDifNames(difNames);
	}
	
	@Test
	public void testSerilalization() throws Exception{
		byte[] encodedRequest = applicationRegistrationEncoder.encode(applicationRegistration);
		for(int i=0; i<encodedRequest.length; i++){
			System.out.print(encodedRequest[i] + " ");
		}
		System.out.println("");
		
		ApplicationRegistration recoveredRegistration= (ApplicationRegistration) applicationRegistrationEncoder.decode(encodedRequest, ApplicationRegistration.class);
		Assert.assertEquals(applicationRegistration.getApNamingInfo().getApplicationProcessName(), recoveredRegistration.getApNamingInfo().getApplicationProcessName());
		Assert.assertEquals(applicationRegistration.getApNamingInfo().getApplicationProcessInstance(), recoveredRegistration.getApNamingInfo().getApplicationProcessInstance());
		Assert.assertEquals(applicationRegistration.getSocketNumber(), recoveredRegistration.getSocketNumber());
		Assert.assertEquals(applicationRegistration.getDifNames().get(0), recoveredRegistration.getDifNames().get(0));
		Assert.assertEquals(applicationRegistration.getDifNames().get(1), recoveredRegistration.getDifNames().get(1));
	}

}
