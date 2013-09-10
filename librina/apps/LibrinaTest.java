import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.rina;

public class LibrinaTest {

	static {
        System.loadLibrary("rina_java");
	}
	
	private static boolean debugEnabled = true;
	
	private static void printStatement(Object statement){
		if (debugEnabled){
			System.out.println(statement);
		}
	}

	public static void main(String[] args) throws IPCException {
		printStatement("************ TESTING LIBRINA ************");
		
		printStatement("\nCREATING SHIM DUMMY IPC PROCESS");
		IPCProcessFactorySingleton ipcProcessFactory = rina.getIpcProcessFactory();
		ApplicationProcessNamingInformation shimDummyName = 
				new ApplicationProcessNamingInformation("/ipcProcesses/i2CAT/Barcelona/dummy", "1");
		IPCProcess shimDummy = ipcProcessFactory.create(shimDummyName, "shim-dummy");
		printStatement("Created IPC Process with id " + shimDummy.getId());
		
		printStatement("\n ASSIGNING IPC PROCESS TO DIF");
		ApplicationProcessNamingInformation difName = 
				new ApplicationProcessNamingInformation("/difs/loopback.DIF", "1");
		DIFConfiguration difConfiguration = new DIFConfiguration();
		difConfiguration.setDifType("shim-dummy");
		difConfiguration.setDifName(difName);
		shimDummy.assignToDIF(difConfiguration);
		printStatement("Assigned IPC Process to DIF " + difName.getProcessName());
		
		printStatement("\n REGISTERING APPLICATION");
		ApplicationProcessNamingInformation appName = 
				new ApplicationProcessNamingInformation("/rina/utils/apps/rinaband", "1");
		appName.setEntityName("control");
		appName.setEntityInstance("1");
		shimDummy.registerApplication(appName);
		printStatement("Registered application: " + appName.getProcessName());
		
		printStatement("\n UNREGISTERING APPLICATION");
		shimDummy.unregisterApplication(appName);
		printStatement("Unregistered application: "+ appName.getProcessName());
		
		printStatement("\nDESTROYING SHIM DUMMY IPC PROCESS");
		ipcProcessFactory.destroy(shimDummy.getId());
		printStatement("Destroyed Shim dummy IPC Process");
	}
}