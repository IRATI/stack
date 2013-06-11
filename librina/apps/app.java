import java.util.Iterator;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationVector;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowPointerVector;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCManager;
import eu.irati.librina.IPCManagerSingleton;
import eu.irati.librina.rina;

public class app {

	static {
              System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));
              System.loadLibrary("rina_java");
	  }
	
	public static final String HELP = "--help";
	public static final String VERSION = "--version";
	public static final String USAGE = "Usage: ipcmd [--<option1> <arg1> ...] ...";

	public static void main(String[] args) throws IPCException {		
		System.out.println("************ TESTING LIBRINA-APPLICATION ************");
		
		for(int i=0; i<args.length; i++){
			if(args[i].equals(HELP)){
				System.out.println(USAGE);
				System.out.println("Options: ");
				System.out.println("	help: Displays the program help. ");
				System.out.println("	version: Displays the library version number.");
				System.exit(0);
			}else if (args[i].equals(VERSION)){
				System.out.println("Librina version "+rina.getVersion());
				System.exit(0);
			}else{
				System.out.println(USAGE);
			}
		}
		
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
		
		System.out.println("\nALLOCATING A FLOW");
		IPCManagerSingleton ipcManager = rina.getIpcManager();
		Flow flow = ipcManager.allocateFlowRequest(sourceNamingInfo, destNamingInfo, flowSpecification);
		System.out.println("Flow allocated, port id is "+flow.getPortId());
		
		System.out.println("\nWRITING A SDU TO THE FLOW");
		byte[] sdu = "This is a test SDU".getBytes();
		flow.writeSDU(sdu, sdu.length);
		System.out.println("Wrote SDU");
		
		System.out.println("\nREADING AN SDU FROM THE FLOW");
		flow.readSDU(sdu);
		System.out.println("Read "+sdu.length+" bytes");
		
		System.out.println("\nLISTING ALLOCATED FLOWS");
		FlowPointerVector allocatedFlows = ipcManager.getAllocatedFlows();
		for(int i=0; i<allocatedFlows.size(); i++){
			System.out.println("Got flow with portid " +allocatedFlows.get(i).getPortId()+
					", using DIF "+allocatedFlows.get(i).getDIFName().getProcessName());
		}
		
		System.out.println("\nDEALLOCATING THE FLOW");
		ipcManager.deallocateFlow(flow.getPortId());
		System.out.println("Flow deallocated");
		try{
			ipcManager.deallocateFlow(430);
		}catch(IPCException ex){
			System.out.println("Caught expected exception: "+ex.getMessage());
		}
		
		System.out.println("\nREGISTERING APPLICATIONS");
		ApplicationProcessNamingInformation difName = new ApplicationProcessNamingInformation("/difs/Test.DIF", "");
		ApplicationProcessNamingInformation difName2 = new ApplicationProcessNamingInformation("/difs/Test2.DIF", "");
		ipcManager.registerApplication(sourceNamingInfo, difName);
		ipcManager.registerApplication(sourceNamingInfo, difName2);
		ipcManager.registerApplication(destNamingInfo, difName);
		
		System.out.println("\nLISTING REGISTERED APPLICATIONS");
		ApplicationRegistrationVector registeredApplications = ipcManager.getRegisteredApplications();
		Iterator<ApplicationProcessNamingInformation> iterator = null;
		for(int i=0; i<registeredApplications.size(); i++){
			System.out.println("Application "+registeredApplications.get(i).getApplicationName() + " registered in DIFs: ");
			iterator = registeredApplications.get(i).getDIFNames().iterator();
			while(iterator.hasNext()){
				System.out.println(iterator.next());
			}
			
		}
		
		System.out.println("\nUNREGISTERING APPLICATIONS");
		ipcManager.unregisterApplication(sourceNamingInfo, difName);
		ipcManager.unregisterApplication(sourceNamingInfo, difName2);
		ipcManager.unregisterApplication(destNamingInfo, difName);
	}
}