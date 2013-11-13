package rina.encoding.impl;

import java.util.HashMap;
import java.util.Map;

import rina.encoding.api.Encoder;

/**
 * Implements an encoder that delegates the encoding/decoding
 * tasks to different subencoders. A different encoder is registered 
 * by each type of object
 * @author eduardgrasa
 *
 */
public class EncoderImpl implements Encoder{
	
	private Map<String, Encoder> encoders = null;
	
	public EncoderImpl(){
		encoders = new HashMap<String, Encoder>();
	}
	
	/**
	 * Set the class that serializes/unserializes an object class
	 * @param objectClass The object class
	 * @param serializer
	 */
	public void addEncoder(String objectClass, Encoder encoder){
		encoders.put(objectClass, encoder);
	}

	/**
	 * Converts a byte array to an object of the type specified by "className"
	 * @param serializedObject
	 * @param The type of object to be decoded
	 * @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the 
	 * byte array value doesn't correspond to an object of the type "className"
	 * @return
	 */
	public Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception{
		return getEncoder(objectClass).decode(serializedObject, objectClass);
	}

	public byte[] encode(Object object) throws Exception {
		return getEncoder(object.getClass()).encode(object);
	}
	
	private Encoder getEncoder(Class<?> objectClass) throws Exception{
		Encoder encoder = encoders.get(objectClass.getName());
		if (encoder == null){
			throw new Exception("No encoders found for class "+objectClass.getName());
		}
		
		return encoder;
	}

}
