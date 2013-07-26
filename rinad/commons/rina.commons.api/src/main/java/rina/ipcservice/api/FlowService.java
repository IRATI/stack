package rina.ipcservice.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

public class FlowService {

	public static final String OBJECT_NAME="/flowservice";
	public static final String OBJECT_CLASS="flow service object";
	
	private ApplicationProcessNamingInfo sourceAPNamingInfo = null;
	private ApplicationProcessNamingInfo destinationAPNamingInfo = null;
	private int portId = 0;
	private QualityOfServiceSpecification qosSpec = null;
	private boolean result = false;
	
	public FlowService(){
	}

	public FlowService(ApplicationProcessNamingInfo sourceAPNamingInfo, ApplicationProcessNamingInfo destinationAPNamingInfo, 
			QualityOfServiceSpecification qosSpec) {
		this.sourceAPNamingInfo = sourceAPNamingInfo;
		this.destinationAPNamingInfo = destinationAPNamingInfo;
		this.qosSpec = qosSpec;
	}
	
	public ApplicationProcessNamingInfo getSourceAPNamingInfo() {
		return sourceAPNamingInfo;
	}

	public void setSourceAPNamingInfo(ApplicationProcessNamingInfo sourceAPNamingInfo) {
		this.sourceAPNamingInfo = sourceAPNamingInfo;
	}

	public ApplicationProcessNamingInfo getDestinationAPNamingInfo() {
		return destinationAPNamingInfo;
	}

	public void setDestinationAPNamingInfo(ApplicationProcessNamingInfo destinationAPNamingInfo) {
		this.destinationAPNamingInfo = destinationAPNamingInfo;
	}

	public int getPortId() {
		return portId;
	}

	public void setPortId(int portId) {
		this.portId = portId;
	}

	public QualityOfServiceSpecification getQoSSpecification() {
		return qosSpec;
	}

	public void setQoSSpecification(QualityOfServiceSpecification qosSpec) {
		this.qosSpec = qosSpec;
	}

	public boolean isResult() {
		return result;
	}

	public void setResult(boolean result) {
		this.result = result;
	}
	
	public String toString(){
		String result = "";
		result = result + "Source AP naming info: " + this.getSourceAPNamingInfo();
		result = result + "Destination AP naming info: " + this.getDestinationAPNamingInfo();
		if (qosSpec != null){
			result = result + qosSpec.toString();
		}
		return result;
	}


}
