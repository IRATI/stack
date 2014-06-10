package rina.ipcprocess.impl.ecfp;

import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.DataTransferConstants;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.ipcprocess.api.IPCProcess;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

public class DataTransferConstantsRIBObject extends BaseRIBObject{

	public static final String DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME = 
			RIBObjectNames.SEPARATOR + RIBObjectNames.DIF + 
			RIBObjectNames.SEPARATOR + RIBObjectNames.IPC + RIBObjectNames.SEPARATOR + 
			RIBObjectNames.DATA_TRANSFER+ RIBObjectNames.SEPARATOR + RIBObjectNames.CONSTANTS;

	public static final String DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS = "datatransfercons";

	public DataTransferConstantsRIBObject(IPCProcess ipcProcess) {
		super(ipcProcess, DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), 
				DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME);
		setRIBDaemon(ipcProcess.getRIBDaemon());
		setEncoder(ipcProcess.getEncoder());
	}

	@Override
	public RIBObject read() throws RIBDaemonException{
		return this;
	}

	@Override
	public void read(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		try{
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(this.getEncoder().encode(getObjectValue()));
			CDAPSessionManager cdapSessionManager = getIPCProcess().getCDAPSessionManager();
			CDAPMessage responseMessage = cdapSessionManager.getReadObjectResponseMessage(cdapSessionDescriptor.getPortId(), 
					null, this.getObjectClass(), this.getObjectInstance(), this.getObjectName(), objectValue, 0, null, 
					cdapMessage.getInvokeID());
			this.getRIBDaemon().sendMessage(responseMessage, cdapSessionDescriptor.getPortId(), null);
		}catch(Exception ex){
			throw new RIBDaemonException(RIBDaemonException.PROBLEMS_SENDING_CDAP_MESSAGE, ex.getMessage());
		}
	}

	@Override
	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		if (cdapMessage.getObjValue() == null){
			throw new RIBDaemonException(RIBDaemonException.OBJECT_VALUE_IS_NULL, "Object value is null");
		}

		try{
			DataTransferConstants value = (DataTransferConstants) this.getEncoder().
					decode(cdapMessage.getObjValue().getByteval(), DataTransferConstants.class);
			this.getRIBDaemon().create(cdapMessage.getObjClass(), cdapMessage.getObjInst(), 
					cdapMessage.getObjName(), value, null);
		}catch(Exception ex){
			throw new RIBDaemonException(RIBDaemonException.PROBLEMS_DECODING_OBJECT, ex.getMessage());
		}
	}

	/**
	 * In this case create has the semantics of update 
	 */
	@Override
	public void create(String objectClass, long objectInstance, String objectName, Object value) throws RIBDaemonException{
		this.write(value);
	}

	@Override
	public void write(Object value) throws RIBDaemonException {
		DataTransferConstants candidate = null;

		if (!(value instanceof DataTransferConstants)){
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Value is not an instance of "+DataTransferConstants.class.getName());
		}

		candidate = (DataTransferConstants) value;
		DataTransferConstants objectValue = null;
		
		DIFInformation difInformation = getIPCProcess().getDIFInformation();
		if (difInformation == null){
			difInformation = new DIFInformation();
			DIFConfiguration difConfiguration = new DIFConfiguration();
			difInformation.setDifConfiguration(difConfiguration);
			objectValue = new DataTransferConstants();
			difConfiguration.getEfcpConfiguration().setDataTransferConstants(objectValue);
		} else {
			objectValue = difInformation.getDifConfiguration().getEfcpConfiguration().getDataTransferConstants();
		}
		
		objectValue.setAddressLength(candidate.getAddressLength());
		objectValue.setCepIdLength(candidate.getCepIdLength());
		objectValue.setDifIntegrity(candidate.isDifIntegrity());
		objectValue.setLengthLength(candidate.getLengthLength());
		objectValue.setMaxPduLifetime(candidate.getMaxPduLifetime());
		objectValue.setMaxPduSize(candidate.getMaxPduSize());
		objectValue.setPortIdLength(candidate.getPortIdLength());
		objectValue.setQosIdLenght(candidate.getQosIdLenght());
		objectValue.setSequenceNumberLength(candidate.getSequenceNumberLength());        
	}

	@Override
	public Object getObjectValue() {
		DIFInformation difInformation = getIPCProcess().getDIFInformation();
		if (difInformation != null) {
			return difInformation.getDifConfiguration().getEfcpConfiguration().getDataTransferConstants();
		}
		
		return null;
	}
}
