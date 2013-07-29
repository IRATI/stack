package rina.encoding.impl.googleprotobuf.datatransferconstants;

import rina.efcp.api.DataTransferConstants;
import rina.encoding.api.BaseEncoder;
import rina.ipcprocess.api.IPCProcess;

public class DataTransferConstantsEncoder extends BaseEncoder{
	
	public void setIPCProcess(IPCProcess ipcProcess) {
	}

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(DataTransferConstants.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		DataTransferConstantsMessage.dataTransferConstants_t gpbDataTransferConstants = 
			DataTransferConstantsMessage.dataTransferConstants_t.parseFrom(serializedObject);
		
		DataTransferConstants dataTransferConstants = new DataTransferConstants();
		dataTransferConstants.setAddressLength(gpbDataTransferConstants.getAddressLength());
		dataTransferConstants.setCepIdLength(gpbDataTransferConstants.getCepIdLength());
		dataTransferConstants.setDIFConcatenation(gpbDataTransferConstants.getDIFConcatenation());
		dataTransferConstants.setDIFFragmentation(gpbDataTransferConstants.getDIFFragmentation());
		dataTransferConstants.setDIFIntegrity(gpbDataTransferConstants.getDIFIntegrity());
		dataTransferConstants.setLengthLength(gpbDataTransferConstants.getLengthLength());
		dataTransferConstants.setMaxPDULifetime(gpbDataTransferConstants.getMaxPDULifetime());
		dataTransferConstants.setMaxPDUSize(gpbDataTransferConstants.getMaxPDUSize());
		dataTransferConstants.setMaxTimeToKeepRetransmitting(gpbDataTransferConstants.getMaxTimeToKeepRetransmitting());
		dataTransferConstants.setMaxTimeToACK(gpbDataTransferConstants.getMaxTimeToACK());
		dataTransferConstants.setPortIdLength(gpbDataTransferConstants.getPortIdLength());
		dataTransferConstants.setQosIdLength(gpbDataTransferConstants.getQosidLength());
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
										setDIFConcatenation(dataTransferConstants.isDIFConcatenation()).
										setDIFFragmentation(dataTransferConstants.isDIFFragmentation()).
										setDIFIntegrity(dataTransferConstants.isDIFIntegrity()).
										setLengthLength(dataTransferConstants.getLengthLength()).
										setMaxPDULifetime(dataTransferConstants.getMaxPDULifetime()).
										setMaxTimeToKeepRetransmitting(dataTransferConstants.getMaxTimeToKeepRetransmitting()).
										setMaxTimeToACK(dataTransferConstants.getMaxTimeToACK()).
										setMaxPDUSize(dataTransferConstants.getMaxPDUSize()).
										setPortIdLength(dataTransferConstants.getPortIdLength()).
										setQosidLength(dataTransferConstants.getQosIdLength()).
										setSequenceNumberLength(dataTransferConstants.getSequenceNumberLength()).
										build();
		
		return gpbDataTransferConstants.toByteArray();
	}

}
