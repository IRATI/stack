package rina.encoding.impl.googleprotobuf.neighbor;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.Neighbor;
import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.neighbor.NeighborMessage.neighbor_t;

public class NeighborEncoder implements Encoder{

	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(Neighbor.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		neighbor_t gpbNeighbor = NeighborMessage.neighbor_t.parseFrom(encodedObject);
		return convertGPBToModel(gpbNeighbor);
	}
	
	public static Neighbor convertGPBToModel(neighbor_t gpbNeighbor){
		Neighbor neighbor = new Neighbor();
		ApplicationProcessNamingInformation namingInfo = 
				new ApplicationProcessNamingInformation(
						gpbNeighbor.getApplicationProcessName(),
						gpbNeighbor.getApplicationProcessInstance()); 
		neighbor.setName(namingInfo);
		neighbor.setAddress(gpbNeighbor.getAddress());

		return neighbor;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof Neighbor)){
			throw new Exception("This is not the encoder for objects of type " + Neighbor.class.toString());
		}
		
		return convertModelToGPB((Neighbor)object).toByteArray();
	}
	
	public static neighbor_t convertModelToGPB(Neighbor neighbor){
		neighbor_t gpbNeighbor = NeighborMessage.neighbor_t.newBuilder().
			setApplicationProcessName(neighbor.getName().getProcessName()).
			setApplicationProcessInstance(neighbor.getName().getProcessInstance()).
			setAddress(neighbor.getAddress()).
			build();
		
		return gpbNeighbor;
	}

}
