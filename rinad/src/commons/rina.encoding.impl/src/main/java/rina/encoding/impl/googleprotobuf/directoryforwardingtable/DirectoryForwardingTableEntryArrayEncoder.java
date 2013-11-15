package rina.encoding.impl.googleprotobuf.directoryforwardingtable;

import java.util.List;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.directoryforwardingtable.DirectoryForwardingTableEntryArrayMessage.directoryForwardingTableEntrySet_t.Builder;
import rina.encoding.impl.googleprotobuf.directoryforwardingtable.DirectoryForwardingTableEntryMessage.directoryForwardingTableEntry_t;
import rina.flowallocator.api.DirectoryForwardingTableEntry;

public class DirectoryForwardingTableEntryArrayEncoder implements Encoder{

	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception{
		if (objectClass == null || !(objectClass.equals(DirectoryForwardingTableEntry[].class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}

		List<directoryForwardingTableEntry_t> gpbDirectoryForwardingTableEntrySet = null;
		DirectoryForwardingTableEntry[] result = null;

		try{
			gpbDirectoryForwardingTableEntrySet = DirectoryForwardingTableEntryArrayMessage.
			directoryForwardingTableEntrySet_t.parseFrom(encodedObject).getDirectoryForwardingTableEntryList();
			result = new DirectoryForwardingTableEntry[gpbDirectoryForwardingTableEntrySet.size()];
			for(int i=0; i<gpbDirectoryForwardingTableEntrySet.size(); i++){
				result[i] = DirectoryForwardingTableEntryEncoder.convertGPBToModel(gpbDirectoryForwardingTableEntrySet.get(i));
			} 
		}catch(NullPointerException ex){
			result = new DirectoryForwardingTableEntry[0];
		}
		
		return result;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof DirectoryForwardingTableEntry[])){
			throw new Exception("This is not the encoder for objects of type " + DirectoryForwardingTableEntry[].class.toString());
		}
		
		DirectoryForwardingTableEntry[] qosCubes = (DirectoryForwardingTableEntry[]) object;
		
		Builder builder = DirectoryForwardingTableEntryArrayMessage.directoryForwardingTableEntrySet_t.newBuilder();
		for(int i=0; i<qosCubes.length; i++){
			builder.addDirectoryForwardingTableEntry(DirectoryForwardingTableEntryEncoder.convertModelToGPB(qosCubes[i]));
		}
		
		return builder.build().toByteArray();
	}

}
