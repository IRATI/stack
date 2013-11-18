package rina.encoding.impl.googleprotobuf.whatevercast;

import java.util.List;

import rina.applicationprocess.api.WhatevercastName;
import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.whatevercast.WhatevercastNameArrayMessage.whatevercastNames_t.Builder;
import rina.encoding.impl.googleprotobuf.whatevercast.WhatevercastNameMessage.whatevercastName_t;

public class WhatevercastNameArrayEncoder implements Encoder{

	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception{
		if (objectClass == null || !(objectClass.equals(WhatevercastName[].class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}

		List<whatevercastName_t> gpbWhatnameSet = null;
		WhatevercastName[] result = null;

		try{
			gpbWhatnameSet = WhatevercastNameArrayMessage.whatevercastNames_t.
			parseFrom(encodedObject).getWhatevercastNameList();
			result = new WhatevercastName[gpbWhatnameSet.size()];
			for(int i=0; i<gpbWhatnameSet.size(); i++){
				result[i] = WhatevercastNameEncoder.convertGPBToModel(gpbWhatnameSet.get(i));
			} 
		}catch(NullPointerException ex){
			result = new WhatevercastName[0];
		}

		return result;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof WhatevercastName[])){
			throw new Exception("This is not the encoder for objects of type " + 
					WhatevercastName[].class.toString());
		}
		
		WhatevercastName[] whatevercastNames = (WhatevercastName[]) object;
		
		Builder builder = WhatevercastNameArrayMessage.whatevercastNames_t.newBuilder();
		for(int i=0; i<whatevercastNames.length; i++){
			builder.addWhatevercastName(WhatevercastNameEncoder.convertModelToGPB(whatevercastNames[i]));
		}
		
		return builder.build().toByteArray();
	}

}
