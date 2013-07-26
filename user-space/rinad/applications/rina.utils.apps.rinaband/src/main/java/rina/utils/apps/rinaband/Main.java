package rina.utils.apps.rinaband;

import java.util.Arrays;

/**
 * Here the various options you can specify when starting RINAband as either a client or a server.

-role (client|server) -client -server

Which role the application will take in the test

-appname AP -instance AI

The AP and AI that RINAband will register as (if in the SERVER role) or connect to (if in the CLIENT role).

-loglevel LEVEL

Implementation-dependant specification of verbosity.

Here the various options you can specify when starting RINAband as a client.

-flows N

How many data flows to open

-sender (client|server|both)

Which end of the test will write SDUs. This implicitly specifies which end of the test will read SDUs

-sdusize N

The size of a single SDU

-count N

How many SDUs to transmit in each direction on each flow.

-qos (reliable|unreliable)

This is TBD. It is intended to allow us to specify the QoS parameters that can be included in an M_CREATE flow.

-pattern (none|increment)

Controls whether a specific pattern of data is to be written into the transmitted SDUs. Doing so will allow the 
application to compare received data with what is expected. Checking the data tests the integrity of data transfer 
through the involved IPC processes; not checking the data may allow a higher raw transfer rate, since the system 
is doing less processing.

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
	public static final String APINSTANCE = "instance";
	public static final String APNAME = "apname";
	public static final String LOGLEVEL = "logLevel";
	public static final String FLOWS = "flows";
	public static final String SENDER = "sender";
	public static final String SDUSIZE = "sdusize";
	public static final String COUNT = "count";
	public static final String QOS = "qos";
	public static final String PATTERN = "pattern";
	public static final String SERVER = "server";
	public static final String CLIENT = "client";
	public static final String BOTH = "both";
	public static final String RELIABLE = "reliable";
	public static final String UNRELIABLE = "unreliable";
	public static final String NONE = "none";
	public static final String INCREMENT = "increment";
	
	public static final int DEFAULT_NUMBER_OF_FLOWS = 20;
	public static final int DEFAULT_SDU_SIZE_IN_BYTES = 1500;
	public static final int DEFAULT_SDU_COUNT = 5000;
	public static final String DEFAULT_ROLE = SERVER;
	public static final String DEFAULT_SENDER = CLIENT;
	public static final String DEFAULT_AP_NAME = "rina.utils.apps.rinaband";
	public static final String DEFAULT_AP_INSTANCE = "1";
	public static final String DEFAULT_QOS = RELIABLE;
	public static final String DEFAULT_PATTERN = NONE;
	
	public static final String USAGE = "java -jar rina.utils.apps.rinaband [-role] (client|server) [-apname] AP [-instance] AI " +
			"[-logLevel] logLevel [-flows] num_flows [-sender] (client|server|both) [-sdusize] sduSizeInBytes [-count] num_sdus " + 
			"[-qos] (reliable|unreliable) [-pattern] (none|increment)";
	public static final String DEFAULTS = "The defaults are: role=server; apname=rina.utils.apps.rinaband; instance=1; logLevel=INFO; "+
			"sender=client; sdusize=1500; count=5000; qos=reliable; pattern=none";
	
	public static void main(String[] args){
		System.out.println(Arrays.toString(args));
		
		int numberOfFlows = DEFAULT_NUMBER_OF_FLOWS;
		int sduSizeInBytes = DEFAULT_SDU_SIZE_IN_BYTES;
		int sduCount = DEFAULT_SDU_COUNT;
		boolean server = false;
		String sender = DEFAULT_SENDER;
		String apName = DEFAULT_AP_NAME;
		String apInstance = DEFAULT_AP_INSTANCE;
		String qos = DEFAULT_QOS;
		String pattern = DEFAULT_PATTERN;
		String logLevel = null;
		
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
			}else if (args[i].equals(ARGUMENT_SEPARATOR + APNAME)){
				apName = args[i+1];
			}else if (args[i].equals(ARGUMENT_SEPARATOR + APINSTANCE)){
				apInstance = args[i+1];
			}else if (args[i].equals(ARGUMENT_SEPARATOR + LOGLEVEL)){
				logLevel = args[i+1];
			}else if (args[i].equals(ARGUMENT_SEPARATOR + SENDER)){
				sender = args[i+1];
				if (!sender.equals(CLIENT) && 
						!sender.equals(SERVER) && 
						!sender.equals(BOTH)){
					showErrorAndExit(SENDER);
				}
			}else if (args[i].equals(ARGUMENT_SEPARATOR + FLOWS)){
				try{
					numberOfFlows = Integer.parseInt(args[i+1]);
					if (numberOfFlows <1){
						showErrorAndExit(FLOWS);
					}
				}catch(Exception ex){
					showErrorAndExit(FLOWS);
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
			}else if (args[i].equals(ARGUMENT_SEPARATOR + QOS)){
				qos = args[i+1];
				if (!qos.equals(RELIABLE) && 
						!qos.equals(UNRELIABLE)){
					showErrorAndExit(QOS);
				}
			}else if (args[i].equals(ARGUMENT_SEPARATOR + PATTERN)){
				pattern = args[i+1];
				if (!pattern.equals(NONE) && 
						!pattern.equals(INCREMENT)){
					showErrorAndExit(PATTERN);
				}
			}else{
				System.out.println("Wrong argument.\nUsage: "
						+USAGE+"\n"+DEFAULTS);
				System.exit(-1);
			}
			
			i = i+2;
		}
		
		TestInformation testInformation = new TestInformation();
		testInformation.setQos(qos);
		testInformation.setPattern(pattern);
		testInformation.setNumberOfFlows(numberOfFlows);
		testInformation.setNumberOfSDUs(sduCount);
		testInformation.setSduSize(sduSizeInBytes);
		if(sender.equals(CLIENT) || sender.equals(BOTH)){
			testInformation.setClientSendsSDUs(true);
		}
		if (sender.equals(SERVER) || sender.equals(BOTH)){
			testInformation.setServerSendsSDUs(true);
		}
		
		RINABand rinaBand = new RINABand(testInformation, server, apName, apInstance);
		rinaBand.execute();
	}
	
	public static void showErrorAndExit(String parameterName){
		System.out.println("Wrong value for argument "+parameterName+".\nUsage: "
				+USAGE+"\n"+DEFAULTS);
		System.exit(-1);
	}

}
