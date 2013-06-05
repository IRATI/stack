

public class ipcmd {

	static {
              System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));
              System.loadLibrary("rina_java");
	  }

	public static void main(String[] args){		
		System.out.println("Testing LIBRINA-IPCMANAGER");
	}
}