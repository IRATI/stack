package rina.pduftg.api;

import eu.irati.librina.DIFConfiguration;
import rina.ipcprocess.api.IPCProcess;

public interface PDUFTableGenerator {
	public void setIPCProcess(IPCProcess ipcProcess);
	public void setDIFConfiguration(DIFConfiguration difConfiguration);
	public PDUFTGeneratorPolicy getPDUFTGeneratorPolicy();
}
