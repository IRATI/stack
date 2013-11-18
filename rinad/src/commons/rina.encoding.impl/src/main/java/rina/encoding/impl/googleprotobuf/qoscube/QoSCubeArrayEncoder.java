package rina.encoding.impl.googleprotobuf.qoscube;

import java.util.List;

import eu.irati.librina.QoSCube;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeArrayMessage;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeArrayMessage.qosCubes_t.Builder;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeMessage.qosCube_t;

public class QoSCubeArrayEncoder implements Encoder{

	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception{
		if (objectClass == null || !(objectClass.equals(QoSCube[].class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}

		List<qosCube_t> gpbQoSSet = null;
		QoSCube[] result = null;

		try{
			gpbQoSSet = QoSCubeArrayMessage.qosCubes_t.parseFrom(encodedObject).getQosCubeList();
			result = new QoSCube[gpbQoSSet.size()];
			for(int i=0; i<gpbQoSSet.size(); i++){
				result[i] = QoSCubeEncoder.convertGPBToModel(gpbQoSSet.get(i));
			}
		}catch(NullPointerException ex){
			result = new QoSCube[0];
		}


		return result;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof QoSCube[])){
			throw new Exception("This is not the encoder for objects of type " + QoSCube[].class.toString());
		}
		
		QoSCube[] qosCubes = (QoSCube[]) object;
		
		Builder builder = QoSCubeArrayMessage.qosCubes_t.newBuilder();
		for(int i=0; i<qosCubes.length; i++){
			builder.addQosCube(QoSCubeEncoder.convertModelToGPB(qosCubes[i]));
		}
		
		return builder.build().toByteArray();
	}

}
