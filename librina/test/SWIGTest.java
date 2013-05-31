import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowSpecification;

public class SWIGTest {

	static {
              System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));
              System.loadLibrary("rina_java");
	  }

	public static void main(String[] args) {		
		System.out.println("************ TESTING LIBRINA-APPLICATION ************");
		ApplicationProcessNamingInformation sourceNamingInfo = new ApplicationProcessNamingInformation();
		sourceNamingInfo.setProcessName("test/application/name/source");
		sourceNamingInfo.setProcessInstance("12");
		sourceNamingInfo.setEntityName("main");
		sourceNamingInfo.setEntityInstance("1");
		
		ApplicationProcessNamingInformation destNamingInfo = new ApplicationProcessNamingInformation();
		destNamingInfo.setProcessName("test/application/name/destination");
		destNamingInfo.setProcessInstance("23");
		destNamingInfo.setEntityName("main");
		destNamingInfo.setEntityInstance("1");
		
		FlowSpecification flowSpecification = new FlowSpecification();
		flowSpecification.setAverageBandwidth(2000);
		flowSpecification.setAverageSduBandwidth(2500);
		flowSpecification.setDelay(100);
		flowSpecification.setJitter(50);
		flowSpecification.setPartialDelivery(false);
		flowSpecification.setOrderedDelivery(true);
		flowSpecification.setMaxAllowableGap(0);
		flowSpecification.setMaxSDUSize(3000);
		flowSpecification.setPeakSduBandwidthDuration(4000);
		flowSpecification.setPeakBandwidthDuration(200);
		
		System.out.println("\nCALLING ALLOCATE FLOW REQUEST");
		Flow flow = new Flow(sourceNamingInfo, destNamingInfo, flowSpecification);
		flow.allocate();
		System.out.println("Flow allocated, port id is "+flow.getPortId());
		
		System.out.println("\nCALLING DEALLOCATE FLOW");
		flow.deallocate();
		System.out.println("Flow deallocated");
	}
}