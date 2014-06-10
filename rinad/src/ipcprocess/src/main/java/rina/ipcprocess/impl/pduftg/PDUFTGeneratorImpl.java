package rina.ipcprocess.impl.pduftg;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.PDUFTableGeneratorConfiguration;

import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.pduftg.linkstate.LinkStatePDUFTGeneratorPolicyImpl;
import rina.pduftg.api.PDUFTGeneratorPolicy;
import rina.pduftg.api.PDUFTableGenerator;

public class PDUFTGeneratorImpl implements PDUFTableGenerator {
	
	private static final Log log = LogFactory.getLog(PDUFTGeneratorImpl.class);
	/**
	 * The IPC process
	 */
	protected IPCProcess ipcProcess = null;
	
	/**
	 * Configurations of the PDUFT generator
	 */
	protected PDUFTableGeneratorConfiguration pduftGeneratorConfiguration = null;
	
	PDUFTGeneratorPolicy pduFTGeneratorPolicy = null;
	
	/**
	 * Constructor
	 */
	public PDUFTGeneratorImpl () {
		log.info("PDUFTGenerator Created");
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
	}
	
	public void setDIFConfiguration(DIFConfiguration difConfiguration) {
		PDUFTableGeneratorConfiguration pduftgConfig = difConfiguration.getPduFTableGeneratorConfiguration();
		
		if (!pduftgConfig.getPduFtGeneratorPolicy().getName().equals("LinkState")) {
			log.warn("Unsupported PDU Forwarding Table Generation policy: "+pduftgConfig.getPduFtGeneratorPolicy().getName() 
					+ "; using 'LinkState' instead." );
		}
		
		pduFTGeneratorPolicy = new LinkStatePDUFTGeneratorPolicyImpl();
		pduFTGeneratorPolicy.setIPCProcess(ipcProcess);
		pduFTGeneratorPolicy.setDIFConfiguration(pduftgConfig);
	}
	
	public PDUFTGeneratorPolicy getPDUFTGeneratorPolicy() {
		return pduFTGeneratorPolicy;
	}
}
