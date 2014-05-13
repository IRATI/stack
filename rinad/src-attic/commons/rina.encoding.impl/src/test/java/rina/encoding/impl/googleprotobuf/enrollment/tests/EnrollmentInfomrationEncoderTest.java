package rina.encoding.impl.googleprotobuf.enrollment.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.enrollment.EnrollmentInformationEncoder;
import rina.enrollment.api.EnrollmentInformationRequest;

/**
 * Test if the serialization/deserialization mechanisms for the Flow object work
 * @author eduardgrasa
 *
 */
public class EnrollmentInfomrationEncoderTest {
	
	private EnrollmentInformationRequest eiRequest = null;
	private EnrollmentInformationEncoder eiEncoder = null;
	
	@Before
	public void setup(){
		eiRequest = new EnrollmentInformationRequest();
		eiRequest.setAddress(23452);
		eiEncoder = new EnrollmentInformationEncoder();
	}
	
	@Test
	public void testEncoding() throws Exception{
		byte[] encodedEiRequest = eiEncoder.encode(eiRequest);
		for(int i=0; i<encodedEiRequest.length; i++){
			System.out.print(encodedEiRequest[i] + " ");
		}
		System.out.println();
		
		EnrollmentInformationRequest recoveredEiRequest = 
			(EnrollmentInformationRequest) eiEncoder.decode(encodedEiRequest, EnrollmentInformationRequest.class);
		Assert.assertEquals(eiRequest.getAddress(), recoveredEiRequest.getAddress());
	}

}
