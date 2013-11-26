package rina.ipcprocess.impl.flowallocator.validation;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.IPCException;

/**
 * Contains the logic to validate an allocate request
 * @author eduardgrasa
 *
 */
public class AllocateRequestValidator {
	
	public void validateAllocateRequest(FlowInformation flowInformation) throws IPCException{
		validateApplicationProcessNamingInfo(flowInformation.getLocalAppName());
		validateApplicationProcessNamingInfo(flowInformation.getRemoteAppName());
	}
	
	/** 
	 * Validates the AP naming info. The current validation just requires application 
	 * process name not to be null
	 * @param APnamingInfo
	 * @throws Exception
	 */
	private void validateApplicationProcessNamingInfo(
			ApplicationProcessNamingInformation apNamingInfo) throws IPCException{
		if (apNamingInfo == null){
			throw new IPCException("Malformed Allocate request. Missing application process naming information");
		}
		
		if (apNamingInfo.getProcessName() == null || 
				apNamingInfo.getProcessName().equals("")){
			throw new IPCException("Malformed Allocate request. " +
					"Missing application process name");
		}
	}

}
