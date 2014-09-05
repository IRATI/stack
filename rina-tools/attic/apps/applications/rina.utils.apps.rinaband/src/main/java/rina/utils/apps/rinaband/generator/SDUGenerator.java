package rina.utils.apps.rinaband.generator;

/**
 * Generates an SDU following a certain pattern
 * @author eduardgrasa
 *
 */
public interface SDUGenerator {
	
	public static final String NONE_PATTERN = "none";
	public static final String INCREMENT_PATTERN  = "increment";
	
	/**
	 * Return the next SDU
	 * @return
	 */
	public byte[] getNextSDU();
	
	/**
	 * Return the pattern that the SDUGenerator follows
	 * @return
	 */
	public String getPattern();
	
	/**
	 * Return the size of an SDU
	 */
	public int getSDUSizeInBytes();
}
