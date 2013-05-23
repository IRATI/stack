import eu.irati.librina.*;

public class SWIGTest {

	static {
                // System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));

                System.loadLibrary("rina_java");
	  }

	public static void main(String[] args) {
		System.out.println("Calling symbol");
		rina.deallocate_flow(23);
		System.out.println("Symbol called");
	}
}
