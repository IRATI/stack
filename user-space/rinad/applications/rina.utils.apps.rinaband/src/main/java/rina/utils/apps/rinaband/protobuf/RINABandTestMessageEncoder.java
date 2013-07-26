package rina.utils.apps.rinaband.protobuf;

import rina.utils.apps.rinaband.TestInformation;
import rina.utils.apps.rinaband.protobuf.RINABandTestMessage.RINAband_test_t;

/**
 * Encodes and decodes RINABand Test messages
 * @author eduardgrasa
 *
 */
public class RINABandTestMessageEncoder {

	public static TestInformation decode(byte[] encodedObject) throws Exception {
		RINAband_test_t gpbRinaBandTest = RINAband_test_t.parseFrom(encodedObject);
		
		TestInformation testInformation = new TestInformation();
		testInformation.setAei(RINABandTestMessageEncoder.getString(gpbRinaBandTest.getAEI()));
		testInformation.setClientSendsSDUs(gpbRinaBandTest.getClient());
		testInformation.setNumberOfFlows(gpbRinaBandTest.getFlows());
		testInformation.setNumberOfSDUs(gpbRinaBandTest.getSDUcount());
		testInformation.setPattern(gpbRinaBandTest.getPattern());
		testInformation.setQos(gpbRinaBandTest.getQos());
		testInformation.setSduSize(gpbRinaBandTest.getSDUsize());
		testInformation.setServerSendsSDUs(gpbRinaBandTest.getServer());
		
		return testInformation;
	}
	
	public static byte[] encode(TestInformation testInformation) throws Exception {
		RINAband_test_t gpbRinaBandTest = RINAband_test_t.newBuilder().
			setAEI(RINABandTestMessageEncoder.getGPBString(testInformation.getAei())).
			setClient(testInformation.isClientSendsSDUs()).
			setFlows(testInformation.getNumberOfFlows()).
			setPattern(testInformation.getPattern()).
			setQos(testInformation.getQos()).
			setSDUcount(testInformation.getNumberOfSDUs()).
			setSDUsize(testInformation.getSduSize()).
			setServer(testInformation.isServerSendsSDUs()).
			setServer(testInformation.isServerSendsSDUs()).
			build();
		
		return gpbRinaBandTest.toByteArray();
	}
	
	private static String getGPBString(String string){
		if (string == null){
			return "";
		}
		
		return string;
	}
	
	public static String getString(String string){
		if (!string.equals("")){
			return string;
		}
		
		return null;
	}
}
