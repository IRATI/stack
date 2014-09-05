package rina.configuration;

public class NeighborData {

	private String apName = null;
	private String apInstance = null;
	private String supportingDifName = null;
	private String difName = null;
	
	public NeighborData(){
	}
	
	public String getApName() {
		return apName;
	}
	public void setApName(String apName) {
		this.apName = apName;
	}
	public String getApInstance() {
		return apInstance;
	}
	public void setApInstance(String apInstance) {
		this.apInstance = apInstance;
	}

	public String getSupportingDifName() {
		return supportingDifName;
	}

	public void setSupportingDifName(String supportingDifName) {
		this.supportingDifName = supportingDifName;
	}

	public String getDifName() {
		return difName;
	}

	public void setDifName(String difName) {
		this.difName = difName;
	}
	
}
