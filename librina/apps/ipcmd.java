import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.FlowRequestEvent;
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
        System.loadLibrary("rina_java");
}

public static final String HELP = "--help";
public static final String VERSION = "--version";
public static final String DEBUG = "--debug";
public static final String USAGE = "Usage: ipcmd [--<option1> <arg1> ...] ...";

private static boolean debugEnabled = false;

private static void printStatement(Object statement){
	if (debugEnabled){
		System.out.println(statement);
	}
}

public static void main(String[] args) throws IPCException{		
	for(int i=0; i<args.length; i++){
		if(args[i].equals(HELP)){
			System.out.println(USAGE);
			System.out.println("Options: ");
			System.out.println("	help: Displays the program help. ");
			System.out.println("	version: Displays the library version number.");
			System.out.println("	debug: Shows debug output.");
			System.exit(0);
		}else if (args[i].equals(VERSION)){
			System.out.println("Librina version "+rina.getVersion());
			System.exit(0);
		}else if (args[i].equals(DEBUG)){
			debugEnabled = true;
		}else{
			System.out.println(USAGE);
		}
	}
	
	printStatement("************ TESTING LIBRINA-IPCMANAGER ************");
	
	IPCProcessFactorySingleton ipcProcessFactory = rina.getIpcProcessFactory();

	printStatement("\nCREATING IPC PROCESSES");
	ApplicationProcessNamingInformation ipcProcessName1 = new ApplicationProcessNamingInformation("/ipcProcesses/i2CAT/Barcelona", "1");
	ApplicationProcessNamingInformation ipcProcessName2 = new ApplicationProcessNamingInformation("/ipcProcesses/i2CAT/Castefa", "1"); 
	IPCProcess ipcProcess1 = ipcProcessFactory.create(ipcProcessName1, "normal");
	printStatement("Created IPC Process with id " + ipcProcess1.getId());
	IPCProcess ipcProcess2 = ipcProcessFactory.create(ipcProcessName2, "shim-ethernet");
	printStatement("Created IPC Process with id " + ipcProcess2.getId());
	
	printStatement("\nLISTING IPC PROCESSES");
	IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
	for(int i=0; i<ipcProcesses.size(); i++){
		printStatement("IPC Process id: "+ipcProcesses.get(i).getId() + "; Type: "+ipcProcesses.get(i).getType());
		printStatement("Naming information. \n"+ipcProcesses.get(i).getName());
	}
	
	ApplicationProcessNamingInformation difName = new ApplicationProcessNamingInformation("/difs/Test.DIF", "");
	
	printStatement("\nASSIGNING IPC PROCESS TO DIF");
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
	
	printStatement("\nREGISTERING APPLICATIONs TO IPC PROCESS");
	ipcProcess1.registerApplication(sourceNamingInfo);
	ipcProcess2.registerApplication(destNamingInfo);
	
	printStatement("\nUNREGISTERING APPLICATIONs FROM IPC PROCESS");
	ipcProcess1.unregisterApplication(sourceNamingInfo);
	ipcProcess2.unregisterApplication(destNamingInfo);
	
	printStatement("\nREQUESTING ALLOCATION OF FLOW TO IPC PROCESS");
	FlowRequestEvent flowRequestEvent = new FlowRequestEvent(
			new FlowSpecification(), true, sourceNamingInfo, destNamingInfo, 25);
	ipcProcess1.allocateFlow(flowRequestEvent);
	
	printStatement("\nDESTROYING IPC PROCESSES");
	ipcProcessFactory.destroy(ipcProcess1.getId());
	ipcProcessFactory.destroy(ipcProcess2.getId());
	
	ApplicationManagerSingleton applicationManager = rina.getApplicationManager();
	
	printStatement("\nNOTIFY APPLICATION ABOUT SUCCESSFUL REGISTRATION");
	ApplicationRegistrationInformation info = new ApplicationRegistrationInformation(
			ApplicationRegistrationType.APPLICATION_REGISTRATION_SINGLE_DIF);
	ApplicationRegistrationRequestEvent appRegRequestEvent = new 
			ApplicationRegistrationRequestEvent(sourceNamingInfo, info, 45);
	applicationManager.applicationRegistered(appRegRequestEvent, difName, 0, "");
	
	printStatement("\nNOTIFY APPLICATION ABOUT SUCCESSFUL UNREGISTRATION");
	ApplicationUnregistrationRequestEvent appUnregRequestEvent = new 
			ApplicationUnregistrationRequestEvent(sourceNamingInfo, difName, 37);
	applicationManager.applicationUnregistered(appUnregRequestEvent, 0, "ok");
	
	printStatement("\nNOTIFY APPLICATION ABOUT SUCCESSFUL FLOW ALLOCATION");
	applicationManager.flowAllocated(flowRequestEvent, "");
}
}