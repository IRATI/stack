package rina.utils.apps.echo;

import java.util.Arrays;

/**
 * Here the various options you can specify when starting Echo as either a client or a server.

-role (client|server) -client -server

Which role the application will take in the test

-sdusize N

The size of a single SDU

-count N

All of the above can have implementation-dependant defaults. For example, the default for -appname might be "RINAband" 
and the default for -instance might be the DIF address of the IPC Process.
 * @author eduardgrasa
 *
 */
public class Main {

	static {
		System.loadLibrary("rina_java");
	}
	
	public static final String ARGUMENT_SEPARATOR = "-";
	public static final String ROLE = "role";
	public static final String SDUSIZE = "sdusize";
	public static final String COUNT = "count";
	public static final String SERVER = "server";
	public static final String CLIENT = "client";
	
	public static final int DEFAULT_SDU_SIZE_IN_BYTES = 1500;
	public static final int DEFAULT_SDU_COUNT = 5000;
	public static final String DEFAULT_ROLE = SERVER;
	public static final String DEFAULT_AP_NAME = "rina.utils.apps.echo";
	public static final String DEFAULT_AP_INSTANCE = "1";
	
	public static final String USAGE = "java -jar rina.utils.apps.echo [-role] (client|server)" +
			"[-sdusize] sduSizeInBytes [-count] num_sdus ";
	public static final String DEFAULTS = "The defaults are: role=server; sdusize=1500; count=5000";
	
	public static void main(String[] args){
		System.out.println(Arrays.toString(args));
		
		int sduSizeInBytes = DEFAULT_SDU_SIZE_IN_BYTES;
		int sduCount = DEFAULT_SDU_COUNT;
		boolean server = false;
		String apName = DEFAULT_AP_NAME;
		String apInstance = DEFAULT_AP_INSTANCE;
		
		int i=0;
		while(i<args.length){
			if (args[i].equals(ARGUMENT_SEPARATOR + ROLE)){
				if (args[i+1].equals(CLIENT)){
					server = false;
				}else if (args[i+1].equals(SERVER)){
					server = true;
				}else{
					showErrorAndExit(ROLE);
				}
			}else if (args[i].equals(ARGUMENT_SEPARATOR + SDUSIZE)){
				try{
					sduSizeInBytes = Integer.parseInt(args[i+1]);
					if (sduSizeInBytes <1){
						showErrorAndExit(SDUSIZE);
					}
				}catch(Exception ex){
					showErrorAndExit(SDUSIZE);
				}
			}else if (args[i].equals(ARGUMENT_SEPARATOR + COUNT)){
				try{
					sduCount = Integer.parseInt(args[i+1]);
					if (sduCount <1){
						showErrorAndExit(COUNT);
					}
				}catch(Exception ex){
					showErrorAndExit(COUNT);
				}
			}else{
				System.out.println("Wrong argument.\nUsage: "
						+USAGE+"\n"+DEFAULTS);
				System.exit(-1);
			}
			
			i = i+2;
		}
		
		Echo echo = new Echo(server, apName, apInstance, sduCount, sduSizeInBytes);
		echo.execute();
	}
	
	public static void showErrorAndExit(String parameterName){
		System.out.println("Wrong value for argument "+parameterName+".\nUsage: "
				+USAGE+"\n"+DEFAULTS);
		System.exit(-1);
	}

}
