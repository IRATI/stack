import eu.irati.librina.rina;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcessFactorySingleton;

public class ipcpd {

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
		
		printStatement("************ CLEANING UP IPC PROCESSES IN KERNEL ************");
		IPCProcessFactorySingleton ipcProcessFactory = rina.getIpcProcessFactory();
		ipcProcessFactory.destroy(1);
	}
}