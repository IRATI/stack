package rina.encoding.impl.googleprotobuf.whatevercast;

import java.util.ArrayList;
import java.util.List;

import com.google.protobuf.ByteString;

import rina.applicationprocess.api.WhatevercastName;
import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.whatevercast.WhatevercastNameMessage.whatevercastName_t;

public class WhatevercastNameEncoder implements Encoder{
	
	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(WhatevercastName.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		whatevercastName_t gpbWhatevercastName = 
				WhatevercastNameMessage.whatevercastName_t.parseFrom(serializedObject);
		
		return convertGPBToModel(gpbWhatevercastName);
	}
	
	public static WhatevercastName convertGPBToModel(whatevercastName_t gpbWhatevercastName){
		List<byte[]> setMembers = new ArrayList<byte[]>();
		for(int i=0; i<gpbWhatevercastName.getSetMembersList().size(); i++){
			setMembers.add(GPBUtils.getByteArray(gpbWhatevercastName.getSetMembersList().get(i)));
		}
		
		WhatevercastName whatevercastName = new WhatevercastName();
		whatevercastName.setName(gpbWhatevercastName.getName());
		whatevercastName.setRule(gpbWhatevercastName.getRule());
		whatevercastName.setSetMembers(setMembers);
		
		return whatevercastName;
	}
	
	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof WhatevercastName)){
			throw new Exception("This is not the encoder for objects of type " + WhatevercastName.class.toString());
		}
		
		return convertModelToGPB((WhatevercastName) object).toByteArray();
	}
	
	public static whatevercastName_t convertModelToGPB(WhatevercastName whatevercastName){
		List<ByteString> gpbSetMembers = new ArrayList<ByteString>();
		for(int i=0; i<whatevercastName.getSetMembers().size(); i++){
			gpbSetMembers.add(GPBUtils.getByteString(whatevercastName.getSetMembers().get(i)));
		}
		
		WhatevercastNameMessage.whatevercastName_t gpbWhatevercastName = WhatevercastNameMessage.whatevercastName_t.newBuilder().
													setName(whatevercastName.getName()).
													setRule(whatevercastName.getRule()).
													addAllSetMembers(gpbSetMembers).
													build();
		
		return gpbWhatevercastName;
	}

}