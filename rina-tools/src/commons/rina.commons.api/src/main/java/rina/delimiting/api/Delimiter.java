package rina.delimiting.api;

import java.util.List;

/**
 * Delimits and undelimits SDUs, to allow multiple SDUs to be concatenated in the same PDU
 * @author eduardgrasa
 *
 */
public interface Delimiter {
	
	/**
	 * Takes a single rawSdu and produces a single delimited byte array, consisting in 
	 * [length][sdu]
	 * @param rawSdus
	 * @return
	 */
	public byte[] getDelimitedSdu(byte[] rawSdu);
	
	/**
	 * Takes a list of raw sdus and produces a single delimited byte array, consisting in 
	 * the concatenation of the sdus followed by their encoded length: [length][sdu][length][sdu] ...
	 * @param rawSdus
	 * @return
	 */
	public byte[] getDelimitedSdus(List<byte[]> rawSdus);
	
	/**
	 * Assumes that the first length bytes of the "byteArray" are an encoded varint (encoding an integer of 32 bytes max), and returns the value 
	 * of this varint. If the byteArray is still not a complete varint, doesn't start with a varint or it is encoding an 
	 * integer of more than 4 bytes the function will return -1. 
	 * @param byteArray
	 * @return the value of the integer encoded as a varint, or -1 if there is not a valid encoded varint32, or -2 if 
	 * this may be a complete varint32 but still more bytes are needed
	 */
	public int readVarint32(byte[] byteArray, int length);
	
	/**
	 * Takes a delimited byte array ([length][sdu][length][sdu] ..) and extracts the sdus
	 * @param delimitedSdus
	 * @return
	 */
	public List<byte[]> getRawSdus(byte[] delimitedSdus);
}
