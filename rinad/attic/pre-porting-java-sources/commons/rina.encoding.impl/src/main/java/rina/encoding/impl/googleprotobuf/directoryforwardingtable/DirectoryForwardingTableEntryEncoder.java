package rina.encoding.impl.googleprotobuf.directoryforwardingtable;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.directoryforwardingtable.DirectoryForwardingTableEntryMessage.directoryForwardingTableEntry_t;
import rina.flowallocator.api.DirectoryForwardingTableEntry;

public class DirectoryForwardingTableEntryEncoder implements Encoder{
	
	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(DirectoryForwardingTableEntry.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		directoryForwardingTableEntry_t gpbDirectoryForwardingTable = 
				DirectoryForwardingTableEntryMessage.directoryForwardingTableEntry_t.parseFrom(encodedObject);
		
		return convertGPBToModel(gpbDirectoryForwardingTable);
	}
	
	public static DirectoryForwardingTableEntry convertGPBToModel(directoryForwardingTableEntry_t gpbDirectoryForwardingTable){		
		DirectoryForwardingTableEntry directoryForwardingTableEntry = new DirectoryForwardingTableEntry();
		directoryForwardingTableEntry.setAddress(gpbDirectoryForwardingTable.getIpcProcessSynonym());
		directoryForwardingTableEntry.setTimestamp(gpbDirectoryForwardingTable.getTimestamp());
		directoryForwardingTableEntry.setApNamingInfo(
				GPBUtils.getApplicationProcessNamingInfo(gpbDirectoryForwardingTable.getApplicationName()));
		
		return directoryForwardingTableEntry;
	}
	
	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof DirectoryForwardingTableEntry)){
			throw new Exception("This is not the encoder for objects of type " + DirectoryForwardingTableEntry.class.toString());
		}
		
		return convertModelToGPB((DirectoryForwardingTableEntry) object).toByteArray();
	}
	
	public static directoryForwardingTableEntry_t convertModelToGPB(DirectoryForwardingTableEntry directoryForwardingTableEntry){
		DirectoryForwardingTableEntryMessage.directoryForwardingTableEntry_t gpbDirectoryForwardingTableEntry = 
			DirectoryForwardingTableEntryMessage.directoryForwardingTableEntry_t.newBuilder().
				setApplicationName(GPBUtils.getApplicationProcessNamingInfoT(directoryForwardingTableEntry.getApNamingInfo())).
				setIpcProcessSynonym(directoryForwardingTableEntry.getAddress()).
				setTimestamp(directoryForwardingTableEntry.getTimestamp()).build();
		
		return gpbDirectoryForwardingTableEntry;
	}

}