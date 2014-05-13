package rina.utils.apps.rinaperf.protobuf;

import rina.utils.apps.rinaperf.TestInformation;
import rina.utils.apps.rinaperf.protobuf.EchoTestMessage.Echo_test_t;

/**
 * Encodes and decodes RINABand Test messages
 * @author eduardgrasa
 *
 */
public class EchoTestMessageEncoder {

	public static TestInformation decode(byte[] encodedObject) throws Exception {
		Echo_test_t gpbEchoTest = Echo_test_t.parseFrom(encodedObject);
		
		TestInformation testInformation = new TestInformation();
		testInformation.setSdusSent(gpbEchoTest.getSDUcount());
		testInformation.setSduSize(gpbEchoTest.getSDUsize());
		testInformation.setTime(gpbEchoTest.getTimeout());
		
		return testInformation;
	}
	
	public static byte[] encode(TestInformation testInformation) throws Exception {
		Echo_test_t gpbEchoTest = Echo_test_t.newBuilder().
				setSDUcount(testInformation.getSdusSent()).
				setSDUsize(testInformation.getSduSize()).
				setTimeout(testInformation.getTime()).
				build();

		return gpbEchoTest.toByteArray();
	}
}
