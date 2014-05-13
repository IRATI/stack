package rina.utils.apps.echo.protobuf;

import rina.utils.apps.echo.TestInformation;
import rina.utils.apps.echo.protobuf.EchoTestMessage.Echo_test_t;

/**
 * Encodes and decodes RINABand Test messages
 * @author eduardgrasa
 *
 */
public class EchoTestMessageEncoder {

	public static TestInformation decode(byte[] encodedObject) throws Exception {
		Echo_test_t gpbEchoTest = Echo_test_t.parseFrom(encodedObject);
		
		TestInformation testInformation = new TestInformation();
		testInformation.setNumberOfSDUs(gpbEchoTest.getSDUcount());
		testInformation.setSduSize(gpbEchoTest.getSDUsize());
		testInformation.setTimeout(gpbEchoTest.getTimeout());
		
		return testInformation;
	}
	
	public static byte[] encode(TestInformation testInformation) throws Exception {
		Echo_test_t gpbEchoTest = Echo_test_t.newBuilder().
				setSDUcount(testInformation.getNumberOfSDUs()).
				setSDUsize(testInformation.getSduSize()).
				setTimeout(testInformation.getTimeout()).
				build();

		return gpbEchoTest.toByteArray();
	}
}
