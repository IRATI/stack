package rina.cdap.api.message;

/**
 * Encapsulates the data to set an object value
 * @author eduardgrasa
 *
 */
public class ObjectValue {

	private int intval = 0;
	private int sintval = 0;
	private long int64val = 0;
	private long sint64val = 0;
	private String strval = null;
	private byte[] byteval = null;
	private int floatval = 0;
	private long doubleval = 0;
	private boolean booleanval = false;
	
	public int getIntval() {
		return intval;
	}
	
	public void setIntval(int intval) {
		this.intval = intval;
	}
	
	public int getSintval() {
		return sintval;
	}
	
	public void setSintval(int sintval) {
		this.sintval = sintval;
	}
	
	public long getInt64val() {
		return int64val;
	}
	
	public void setInt64val(long int64val) {
		this.int64val = int64val;
	}
	
	public long getSint64val() {
		return sint64val;
	}
	
	public void setSint64val(long sint64val) {
		this.sint64val = sint64val;
	}
	
	public String getStrval() {
		return strval;
	}
	
	public void setStrval(String strval) {
		this.strval = strval;
	}
	
	public byte[] getByteval() {
		return byteval;
	}
	
	public void setByteval(byte[] buteval) {
		this.byteval = buteval;
	}
	
	public int getFloatval() {
		return floatval;
	}
	
	public void setFloatval(int floatval) {
		this.floatval = floatval;
	}
	
	public long getDoubleval() {
		return doubleval;
	}
	
	public void setDoubleval(long doubleval) {
		this.doubleval = doubleval;
	}

	public void setBooleanval(boolean booleanval) {
		this.booleanval = booleanval;
	}

	public boolean isBooleanval() {
		return booleanval;
	}
}