import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.DIFType;
import eu.irati.librina.FlowRequest;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.IPCProcessPointerVector;
import eu.irati.librina.QoSCube;
import eu.irati.librina.QoSCubeVector;
import eu.irati.librina.rina;

public class ipcmd {

	static {
              System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));
              System.out.println("java.class.path = " + System.getProperties().getProperty("java.class.path"));
              System.loadLibrary("rina_java");
	  }
	
	public static final String HELP = "--help";
	public static final String VERSION = "--version";
	public static final String USAGE = "Usage: ipcmd [--<option1> <arg1> ...] ...";

	public static void main(String[] args) throws IPCException{		
		System.out.println("************ TESTING LIBRINA-IPCMANAGER ************");
		
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
		
		IPCProcessFactorySingleton ipcProcessFactory = rina.getIpcProcessFactory();

		System.out.println("\nCREATING IPC PROCESSES");
		ApplicationProcessNamingInformation ipcProcessName1 = new ApplicationProcessNamingInformation("/ipcProcesses/i2CAT/Barcelona", "1");
		ApplicationProcessNamingInformation ipcProcessName2 = new ApplicationProcessNamingInformation("/ipcProcesses/i2CAT/Castefa", "1"); 
		IPCProcess ipcProcess1 = ipcProcessFactory.create(ipcProcessName1, DIFType.DIF_TYPE_NORMAL);
		System.out.println("Created IPC Process with id " + ipcProcess1.getId());
		IPCProcess ipcProcess2 = ipcProcessFactory.create(ipcProcessName2, DIFType.DIF_TYPE_SHIM_ETHERNET);
		System.out.println("Created IPC Process with id " + ipcProcess2.getId());
		
		System.out.println("\nLISTING IPC PROCESSES");
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		for(int i=0; i<ipcProcesses.size(); i++){
			System.out.println("IPC Process id: "+ipcProcesses.get(i).getId() + "; Type: "+ipcProcesses.get(i).getType());
			System.out.println("Naming information. \n"+ipcProcesses.get(i).getName());
		}
		
		ApplicationProcessNamingInformation difName = new ApplicationProcessNamingInformation("/difs/Test.DIF", "");
		
		System.out.println("\nASSIGNING IPC PROCESS TO DIF");
		DIFConfiguration difConfiguration = new DIFConfiguration();
		difConfiguration.setDifName(difName);
		QoSCubeVector qosCubes = new QoSCubeVector();
		qosCubes.add(new QoSCube("reliable", 1));
		qosCubes.add(new QoSCube("unreliable", 2));
		difConfiguration.setQosCubes(qosCubes);
		ipcProcess1.assignToDIF(difConfiguration);
		
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
		
		System.out.println("\nREGISTERING APPLICATIONs TO IPC PROCESS");
		ipcProcess1.registerApplication(sourceNamingInfo);
		ipcProcess2.registerApplication(destNamingInfo);
		
		System.out.println("\nUNREGISTERING APPLICATIONs FROM IPC PROCESS");
		ipcProcess1.unregisterApplication(sourceNamingInfo);
		ipcProcess2.unregisterApplication(destNamingInfo);
		
		System.out.println("\nREQUESTING ALLOCATION OF FLOW TO IPC PROCESS");
		FlowRequest flowRequest = new FlowRequest();
		flowRequest.setSourceApplicationName(sourceNamingInfo);
		flowRequest.setDestinationApplicationName(destNamingInfo);
		flowRequest.setFlowSpecification(new FlowSpecification());
		flowRequest.setPortId(230);
		ipcProcess1.allocateFlow(flowRequest);
		
		System.out.println("\nDESTROYING IPC PROCESSES");
		ipcProcessFactory.destroy(ipcProcess1.getId());
		ipcProcessFactory.destroy(ipcProcess2.getId());
		
		ApplicationManagerSingleton applicationManager = rina.getApplicationManager();
		
		System.out.println("\nNOTIFY APPLICATION ABOUT SUCCESSFUL REGISTRATION");
		applicationManager.applicationRegistered(23, "ok");
		
		System.out.println("\nNOTIFY APPLICATION ABOUT SUCCESSFUL UNREGISTRATION");
		applicationManager.applicationUnregistered(34, "ok");
		
		System.out.println("\nNOTIFY APPLICATION ABOUT SUCCESSFUL FLOW ALLOCATION");
		applicationManager.flowAllocated(13, 434, 3, "ok", difName);
	}
}