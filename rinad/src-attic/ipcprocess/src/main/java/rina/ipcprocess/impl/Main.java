package rina.ipcprocess.impl;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;

public class Main {
	
	static {
		System.loadLibrary("rina_java");
	}

	private static final Log log = LogFactory.getLog(Main.class);
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		try{
			log.info("Instantiating IPC Process ...");
			
			if (args.length != 4){
				log.error("Don't have enough arguments, exiting");
				System.exit(-1);
			}
			ApplicationProcessNamingInformation namingInfo = 
					new ApplicationProcessNamingInformation();
			namingInfo.setProcessName(args[0]);
			namingInfo.setProcessInstance(args[1]);
			int ipcProcessId = Integer.parseInt(args[2]);
			long ipcManagerPort = Long.parseLong(args[3]);
			IPCProcessImpl ipcProcess = new IPCProcessImpl();
			ipcProcess.initialize(namingInfo, ipcProcessId, ipcManagerPort);
			ipcProcess.executeEventLoop();
		}catch(Exception ex){
			log.error("Problems: " + ex.getMessage() + ". Closing down IPC Process.");
			System.exit(-1);
		}
	}

}
