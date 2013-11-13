package rina.encoding.api;

/**
 * Encodes and Decodes an object to/from bytes)
 * @author eduardgrasa
 *
 */
public interface Encoder {
	
	/**
	 * Converts an object to a byte array, if this object is recognized by the encoder
	 * @param object
	 * @throws exception if the object is not recognized by the encoder
	 * @return
	 */
	public byte[] encode(Object object) throws Exception;
	
	/**
	 * Converts a byte array to an object of the type specified by "className"
	 * @param byte[] serializedObject
	 * @param objectClass The type of object to be decoded
	 * @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the 
	 * byte array value doesn't correspond to an object of the type "className"
	 * @return
	 */
	public Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception;
}
