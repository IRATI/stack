package rina.ipcprocess.impl;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

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
		}catch(Exception ex){
			log.error("Problems: " + ex.getMessage());
			ex.printStackTrace();
		}
	}

}
