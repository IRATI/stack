package rina.encoding.impl.googleprotobuf.adataunit;

import rina.adataunit.api.ADataUnitPDU;
import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.adataunit.ADataUnitMessage.adataunit_t;

import com.google.protobuf.ByteString;

public class ADataUnitPDUEncoder implements Encoder{

	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(ADataUnitPDU.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		adataunit_t gpbADataUnit = ADataUnitMessage.adataunit_t.parseFrom(encodedObject);
		return convertGPBToModel(gpbADataUnit);
	}
	
	public static ADataUnitPDU convertGPBToModel(adataunit_t gpbADataUnit){
		ADataUnitPDU adata = new ADataUnitPDU();
		adata.setDestinationAddress(gpbADataUnit.getDestinationAddress());
		adata.setSourceAddresss(gpbADataUnit.getSourceAddress());
		if (!gpbADataUnit.getPayload().equals(ByteString.EMPTY)) {
			adata.setPayload(gpbADataUnit.getPayload().toByteArray());
		}
		
		return adata;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof ADataUnitPDU)){
			throw new Exception("This is not the encoder for objects of type " 
					+ ADataUnitPDU.class.toString());
		}
		
		return convertModelToGPB((ADataUnitPDU)object).toByteArray();
	}
	
	public static adataunit_t convertModelToGPB(ADataUnitPDU adata){
		ByteString payload = null;
		if (adata.getPayload() != null){
			payload = ByteString.copyFrom(adata.getPayload());
		}else{
			payload = ByteString.EMPTY;
		}
		
		adataunit_t gpbNeighbor = ADataUnitMessage.adataunit_t.newBuilder().
			setSourceAddress(adata.getSourceAddresss()).
			setDestinationAddress(adata.getDestinationAddress()).
			setPayload(payload).
			build();
		
		return gpbNeighbor;
	}

}
