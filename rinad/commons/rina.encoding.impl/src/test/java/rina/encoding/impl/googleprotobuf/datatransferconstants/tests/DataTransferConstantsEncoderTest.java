package rina.encoding.impl.googleprotobuf.datatransferconstants.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.efcp.api.DataTransferConstants;
import rina.encoding.impl.googleprotobuf.datatransferconstants.DataTransferConstantsEncoder;

/**
 * Test if the serialization/deserialization mechanisms for the Flow object work
 * @author eduardgrasa
 *
 */
public class DataTransferConstantsEncoderTest {
	
	private DataTransferConstants dataTransferConstants = null;
	private DataTransferConstantsEncoder dataTransferConstantsEncoder = null;
	
	@Before
	public void setup(){
		dataTransferConstants = new DataTransferConstants();
		dataTransferConstantsEncoder = new DataTransferConstantsEncoder();
	}
	
	@Test
	public void testSerilalization() throws Exception{
		byte[] serializedDTC = dataTransferConstantsEncoder.encode(dataTransferConstants);
		for(int i=0; i<serializedDTC.length; i++){
			System.out.print(serializedDTC[i] + " ");
		}
		
		DataTransferConstants recoveredDataTransferConstants = 
			(DataTransferConstants) dataTransferConstantsEncoder.decode(serializedDTC, DataTransferConstants.class);
		Assert.assertEquals(dataTransferConstants.getAddressLength(), recoveredDataTransferConstants.getAddressLength());
		Assert.assertEquals(dataTransferConstants.getCepIdLength(), recoveredDataTransferConstants.getCepIdLength());
		Assert.assertEquals(dataTransferConstants.getLengthLength(), recoveredDataTransferConstants.getLengthLength());
		Assert.assertEquals(dataTransferConstants.getMaxPDULifetime(), recoveredDataTransferConstants.getMaxPDULifetime());
		Assert.assertEquals(dataTransferConstants.getMaxTimeToACK(), recoveredDataTransferConstants.getMaxTimeToACK());
		Assert.assertEquals(dataTransferConstants.getMaxTimeToKeepRetransmitting(), recoveredDataTransferConstants.getMaxTimeToKeepRetransmitting());
		Assert.assertEquals(dataTransferConstants.getMaxPDUSize(), recoveredDataTransferConstants.getMaxPDUSize());
		Assert.assertEquals(dataTransferConstants.isDIFConcatenation(), recoveredDataTransferConstants.isDIFConcatenation());
		Assert.assertEquals(dataTransferConstants.isDIFFragmentation(), recoveredDataTransferConstants.isDIFFragmentation());
		Assert.assertEquals(dataTransferConstants.isDIFIntegrity(), recoveredDataTransferConstants.isDIFIntegrity());
		Assert.assertEquals(dataTransferConstants.getPortIdLength(), recoveredDataTransferConstants.getPortIdLength());
		Assert.assertEquals(dataTransferConstants.getQosIdLength(), recoveredDataTransferConstants.getQosIdLength());
		Assert.assertEquals(dataTransferConstants.getSequenceNumberLength(), recoveredDataTransferConstants.getSequenceNumberLength());
	}

}
