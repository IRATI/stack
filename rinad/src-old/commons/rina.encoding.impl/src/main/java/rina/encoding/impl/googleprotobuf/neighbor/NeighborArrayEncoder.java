package rina.encoding.impl.googleprotobuf.neighbor;

import java.util.List;

import eu.irati.librina.Neighbor;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.neighbor.NeighborArrayMessage.neighbors_t.Builder;
import rina.encoding.impl.googleprotobuf.neighbor.NeighborMessage.neighbor_t;

public class NeighborArrayEncoder implements Encoder{

	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception{
		if (objectClass == null || !(objectClass.equals(Neighbor[].class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}

		List<neighbor_t> gpbNeighborSet = null;
		Neighbor[] result = null;

		try{
			gpbNeighborSet = NeighborArrayMessage.neighbors_t.parseFrom(encodedObject).getNeighborList();
			result = new Neighbor[gpbNeighborSet.size()];
			for(int i=0; i<gpbNeighborSet.size(); i++){
				result[i] = NeighborEncoder.convertGPBToModel(gpbNeighborSet.get(i));
			} 
		}catch(NullPointerException ex){
			result = new Neighbor[0];
		}

		return result;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof Neighbor[])){
			throw new Exception("This is not the encoder for objects of type " + Neighbor[].class.toString());
		}
		
		Neighbor[] neighbors = (Neighbor[]) object;
		
		Builder builder = NeighborArrayMessage.neighbors_t.newBuilder();
		for(int i=0; i<neighbors.length; i++){
			builder.addNeighbor(NeighborEncoder.convertModelToGPB(neighbors[i]));
		}
		
		return builder.build().toByteArray();
	}

}
