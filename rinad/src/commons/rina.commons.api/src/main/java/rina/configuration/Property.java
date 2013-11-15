package rina.configuration;

/**
 * A name/value pair
 * @author eduardgrasa
 *
 */
public class Property {

	/**
	 * The property name
	 */
	private String name = null;
	
	/**
	 * The property value
	 */
	private String value = null;

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public String getValue() {
		return value;
	}

	public void setValue(String value) {
		this.value = value;
	}
}
