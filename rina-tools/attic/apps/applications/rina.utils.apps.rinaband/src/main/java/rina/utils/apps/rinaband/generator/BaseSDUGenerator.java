package rina.utils.apps.rinaband.generator;

/**
 * Base abstract class
 * @author eduardgrasa
 *
 */
public abstract class BaseSDUGenerator implements SDUGenerator{

	private String pattern = null;
	private int sduSize = 0;
	
	public BaseSDUGenerator(String pattern, int sduSize){
		this.pattern = pattern;
		this.sduSize = sduSize;
	}

	public String getPattern() {
		return this.pattern;
	}

	public int getSDUSizeInBytes() {
		return this.sduSize;
	}

}
