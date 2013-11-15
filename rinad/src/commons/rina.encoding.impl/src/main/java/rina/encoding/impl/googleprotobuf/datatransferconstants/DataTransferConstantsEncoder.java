package rina.encoding.impl.googleprotobuf.datatransferconstants;

import eu.irati.librina.DataTransferConstants;
import rina.encoding.api.Encoder;


public class DataTransferConstantsEncoder implements Encoder{
	
	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(DataTransferConstants.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		DataTransferConstantsMessage.dataTransferConstants_t gpbDataTransferConstants = 
			DataTransferConstantsMessage.dataTransferConstants_t.parseFrom(serializedObject);
		
		DataTransferConstants dataTransferConstants = new DataTransferConstants();
		dataTransferConstants.setAddressLength(gpbDataTransferConstants.getAddressLength());
		dataTransferConstants.setCepIdLength(gpbDataTransferConstants.getCepIdLength());
		dataTransferConstants.setDifIntegrity(gpbDataTransferConstants.getDIFIntegrity());
		dataTransferConstants.setLengthLength(gpbDataTransferConstants.getLengthLength());
		dataTransferConstants.setMaxPduLifetime(gpbDataTransferConstants.getMaxPDULifetime());
		dataTransferConstants.setMaxPduSize(gpbDataTransferConstants.getMaxPDUSize());
		dataTransferConstants.setPortIdLength(gpbDataTransferConstants.getPortIdLength());
		dataTransferConstants.setQosIdLenght(gpbDataTransferConstants.getQosidLength());
		dataTransferConstants.setSequenceNumberLength(gpbDataTransferConstants.getSequenceNumberLength());
		
		return dataTransferConstants;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof DataTransferConstants)){
			throw new Exception("This is not the encoder for objects of type " + DataTransferConstants.class.toString());
		}
		
		DataTransferConstants dataTransferConstants = (DataTransferConstants) object;
		
		DataTransferConstantsMessage.dataTransferConstants_t gpbDataTransferConstants = 
			DataTransferConstantsMessage.dataTransferConstants_t.newBuilder().
										setAddressLength(dataTransferConstants.getAddressLength()).
										setCepIdLength(dataTransferConstants.getCepIdLength()).
										setDIFConcatenation(false).
										setDIFFragmentation(false).
										setDIFIntegrity(dataTransferConstants.isDifIntegrity()).
										setLengthLength(dataTransferConstants.getLengthLength()).
										setMaxPDULifetime((int)dataTransferConstants.getMaxPduLifetime()).
										setMaxPDUSize((int)dataTransferConstants.getMaxPduSize()).
										setPortIdLength(dataTransferConstants.getPortIdLength()).
										setQosidLength(dataTransferConstants.getQosIdLenght()).
										setSequenceNumberLength(dataTransferConstants.getSequenceNumberLength()).
										build();
		
		return gpbDataTransferConstants.toByteArray();
	}

}
