import eu.irati.librina.*;

public class SWIGTest {

	static {
                // String path = "/home/francesco/Lavoro/Nextworks/Progetti/EU/FP7/IRATI/repo/irati/src/user-space/librina/.env/lib/";
                //
                // //System.load(path + "librina_java.so");
                // //System.load(path + "librina.so");

                System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));

                System.loadLibrary("rina_java");
	  }

	public static void main(String[] args) {
		System.out.println("Calling symbol");
		rina.deallocate_flow(23);
		System.out.println("Symbol called");
	}
}
