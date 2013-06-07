import eu.irati.librina.rina;

public class ipcpd {

	static {
              System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));
              System.loadLibrary("rina_java");
	  }
	
	public static final String HELP = "--help";
	public static final String VERSION = "--version";
	public static final String USAGE = "Usage: ipcmd [--<option1> <arg1> ...] ...";

	public static void main(String[] args){		
		System.out.println("Testing LIBRINA-IPCPROCESS");
		
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
	}
}