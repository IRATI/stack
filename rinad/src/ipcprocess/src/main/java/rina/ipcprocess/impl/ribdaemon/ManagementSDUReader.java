package rina.ipcprocess.impl.ribdaemon;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.KernelIPCProcessSingleton;
import eu.irati.librina.ReadManagementSDUResult;

/**
 * Reads sdus from the Kernel IPC Process, and
 * passes them to the RIB Daemon
 * @author eduardgrasa
 */
public class ManagementSDUReader implements Runnable{
	
	private KernelIPCProcessSingleton kernelIPCProcess = null;
	private RIBDaemonImpl ribDaemon = null;
	private int maxSDUSize = 0;
	private static final Log log = LogFactory.getLog(ManagementSDUReader.class);
	
	public ManagementSDUReader(KernelIPCProcessSingleton kernelIPCProcess, 
			RIBDaemonImpl ribDaemon, int maxSDUSize){
		this.kernelIPCProcess = kernelIPCProcess;
		this.ribDaemon = ribDaemon;
		this.maxSDUSize = maxSDUSize;
	}

	@Override
	public void run() {
		byte[] buffer = new byte[maxSDUSize];
		byte[] sdu = null;
		ReadManagementSDUResult result = null;
		
		log.info("Starting Management SDU reader...");
		
		while(true){
			try{
				result = kernelIPCProcess.readManagementSDU(buffer, maxSDUSize);
			}catch(Exception ex){
				log.error("Problems reading management SDU");
			}
			
			sdu = new byte[result.getBytesRead()];
			for(int i=0; i<sdu.length; i++){
				sdu[i] = buffer[i];
			}
			
			ribDaemon.cdapMessageDelivered(sdu, result.getPortId());
		}
	}
	
}
