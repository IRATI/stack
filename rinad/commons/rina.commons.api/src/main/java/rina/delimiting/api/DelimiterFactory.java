package rina.delimiting.api;

/**
 * Creates delimiter instances
 * @author eduardgrasa
 *
 */
public interface DelimiterFactory {
	/**
	 * Constant for DIF type delimiters
	 */
	public static final String DIF = "DIF";
	
	/**
	 * Constant for Public Internet type delimiters
	 */
	public static final String PUBLIC_INTERNET = "Public Internet";
	
	/**
	 * Create a delimiter of a given type (either DIF or Public Internet).
	 * @param delimiterType
	 * @return
	 * @throws RuntimeException IF the delimiter type does not exist
	 */
	public Delimiter createDelimiter(String delimiterType);
}
