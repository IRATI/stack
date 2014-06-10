package rina.utils.apps.echo;

import java.util.Arrays;

import eu.irati.librina.ApplicationProcessNamingInformation;

/**
 * Here the various options you can specify when starting Echo as either a client or a server.
 
-sappname AP -sinstance AI

The AP and AI that Echo app will register as (if in the SERVER role) or connect to (if in the CLIENT role).

-cappname AP -cinstance AI

The AP and AI that the Echo client will use

-role (client|server) -client -server

Which role the application will take in the test

-sdusize N

The size of a single SDU

-count N

Timeout to consider the test completed

-timeout T

Rate at which to send (in Mbps)

-rate R

Max allowable SDU gap (in SDUs, a value of -1 is a 'don't care')

-gap G

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
	public static final String SAPINSTANCE = "sinstance";
	public static final String SAPNAME = "sapname";
	public static final String CAPINSTANCE = "cinstance";
	public static final String CAPNAME = "capname";
	public static final String TIMEOUT = "timeout";
    public static final String RATE = "rate";
    public static final String GAP = "gap";
	
	public static final int DEFAULT_SDU_SIZE_IN_BYTES = 1500;
	public static final int DEFAULT_SDU_COUNT = 5000;
	public static final String DEFAULT_ROLE = SERVER;
	public static final String DEFAULT_SERVER_AP_NAME = "rina.utils.apps.echo.server";
	public static final String DEFAULT_CLIENT_AP_NAME = "rina.utils.apps.echo.client";
	public static final String DEFAULT_AP_INSTANCE = "1";
	public static final int DEFAULT_TIMEOUT_IN_MS = 2000;
    public static final int DEFAULT_RATE_IN_MBPS = 1000;
    public static final int DEFAULT_MAX_ALLOWABLE_GAP_IN_SDUS = -1;
	
	public static final String USAGE = "java -jar rina.utils.apps.echo [-role] (client|server)" +
			"[-sapname] serverApName [-sinstance] serverApInstance [-capname] clientApName " + 
			"[-cinstance] clientApInstance [-sdusize] sduSizeInBytes [-count] num_sdus " +
                        "[-timeout] timeout_in_milliseconds [-rate] rate_in_mbps " + 
			            "[-gap] max_allowable_gap_in_sdus";
	public static final String DEFAULTS = "The defaults are: role=server;  sapname=rina.utils.apps.echo.server; " + 
			"sinstance=1; capname=rina.utils.apps.echo.client; cinstance=1; sdusize=1500; count=5000; timeout=2000; rate=1000; gap=-1";
	
	public static void main(String[] args){
		System.out.println(Arrays.toString(args));
		
		int sduSizeInBytes = DEFAULT_SDU_SIZE_IN_BYTES;
		int sduCount = DEFAULT_SDU_COUNT;
		boolean server = false;
		String serverApName = DEFAULT_SERVER_AP_NAME;
		String clientApName = DEFAULT_CLIENT_AP_NAME;
		String serverApInstance = DEFAULT_AP_INSTANCE;
		String clientApInstance = DEFAULT_AP_INSTANCE;
		int timeout = DEFAULT_TIMEOUT_IN_MS;
        int rate = DEFAULT_RATE_IN_MBPS;
        int gap = DEFAULT_MAX_ALLOWABLE_GAP_IN_SDUS;
		
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
			}else if (args[i].equals(ARGUMENT_SEPARATOR + SAPNAME)){
				serverApName = args[i+1];
			}else if (args[i].equals(ARGUMENT_SEPARATOR + SAPINSTANCE)){
				serverApInstance = args[i+1];
			}else if (args[i].equals(ARGUMENT_SEPARATOR + CAPNAME)){
				clientApName = args[i+1];
			}else if (args[i].equals(ARGUMENT_SEPARATOR + CAPINSTANCE)){
				clientApInstance = args[i+1];
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
			}else if (args[i].equals(ARGUMENT_SEPARATOR + TIMEOUT)){
				try{
					timeout = Integer.parseInt(args[i+1]);
					if (timeout < 100){
						showErrorAndExit(TIMEOUT);
					}
				}catch(Exception ex){
					showErrorAndExit(TIMEOUT);
				}
			}else if (args[i].equals(ARGUMENT_SEPARATOR + RATE)){
				try{
					rate = Integer.parseInt(args[i+1]);
					if (rate <= 0){
						showErrorAndExit(RATE);
					}
				}catch(Exception ex){
					showErrorAndExit(RATE);
				}
			}else if (args[i].equals(ARGUMENT_SEPARATOR + GAP)){
				try{
					gap = Integer.parseInt(args[i+1]);
					if (rate <= -1){
						showErrorAndExit(GAP);
					}
				}catch(Exception ex){
					showErrorAndExit(GAP);
				}
			}else{
				System.out.println("Wrong argument.\nUsage: "
						+USAGE+"\n"+DEFAULTS);
				System.exit(-1);
			}

			i = i+2;
		}
		
		ApplicationProcessNamingInformation serverAPNamingInfo = 
				new ApplicationProcessNamingInformation(serverApName, serverApInstance);
		ApplicationProcessNamingInformation clientApNamingInfo = 
				new ApplicationProcessNamingInformation(clientApName, clientApInstance);
		
		Echo echo = new Echo(server, serverAPNamingInfo, clientApNamingInfo, sduCount, 
                                     sduSizeInBytes, timeout, rate, gap);
		echo.execute();
	}
	
	public static void showErrorAndExit(String parameterName){
		System.out.println("Wrong value for argument "+parameterName+".\nUsage: "
				+USAGE+"\n"+DEFAULTS);
		System.exit(-1);
	}

}
