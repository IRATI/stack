package rina.ipcprocess.impl.flowallocator.ribobjects;

import java.util.ArrayList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.QoSCube;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.api.IPCProcess;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class QoSCubeSetRIBObject extends BaseRIBObject{
	
	public static final String QOSCUBE_SET_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DIF + 
            RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + RIBObjectNames.FLOW_ALLOCATOR + 
            RIBObjectNames.SEPARATOR + RIBObjectNames.QOS_CUBES;

    public static final String QOSCUBE_SET_RIB_OBJECT_CLASS = "qoscube set";
    
    public static final String QOSCUBE_RIB_OBJECT_CLASS = "qoscube";

	private static final Log log = LogFactory.getLog(QoSCubeSetRIBObject.class);
	
	public QoSCubeSetRIBObject(IPCProcess ipcProcess){
		super(ipcProcess, QOSCUBE_SET_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), 
				QOSCUBE_SET_RIB_OBJECT_NAME);
		setRIBDaemon(ipcProcess.getRIBDaemon());
		setEncoder(ipcProcess.getEncoder());
	}
	
	@Override
	public RIBObject read() throws RIBDaemonException{
		return this;
	}
	
	@Override
	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		try{
			QoSCube[] qosCubes = (QoSCube[])
				this.getEncoder().decode(cdapMessage.getObjValue().getByteval(), QoSCube[].class);
			this.getRIBDaemon().create(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName(), 
					qosCubes, null);
		}catch(Exception ex){
			log.error(ex);
			ex.printStackTrace();
		}
	}
	
	@Override
	public synchronized void create(String objectClass, long objectInstance, String objectName, Object object) throws RIBDaemonException{
		if (object instanceof QoSCube){
			SimpleSetMemberRIBObject ribObject = new SimpleSetMemberRIBObject(getIPCProcess(), 
					QOSCUBE_RIB_OBJECT_CLASS, objectName, (QoSCube) object);
			ribObject.setRIBDaemon(getRIBDaemon());
			this.addChild(ribObject);
			getRIBDaemon().addRIBObject(ribObject);
			getIPCProcess().getDIFInformation().getDifConfiguration().getEfcpConfiguration().addQoSCube((QoSCube) object);
		}else if (object instanceof QoSCube[]){
			QoSCube[] cubes = (QoSCube[]) object;
			String candidateObjectName = null;
			
			for(int i=0; i<cubes.length; i++){
				candidateObjectName = this.getObjectName() + RIBObjectNames.SEPARATOR + cubes[i].getName();
				if (!this.hasChild(candidateObjectName)){
					SimpleSetMemberRIBObject ribObject = new SimpleSetMemberRIBObject(getIPCProcess(),  
							QOSCUBE_RIB_OBJECT_CLASS, candidateObjectName, cubes[i]);
					ribObject.setRIBDaemon(getRIBDaemon());
					this.addChild(ribObject);
					getRIBDaemon().addRIBObject(ribObject);
					getIPCProcess().getDIFInformation().getDifConfiguration().getEfcpConfiguration().addQoSCube(cubes[i]);
				}
			}
			
		}else{
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+object.getClass().getName()+") does not match object name "+objectName);
		}
	}

	@Override
	public synchronized void delete(Object objectValue) throws RIBDaemonException {
		String childName = null;
		List<String> childrenNames = new ArrayList<String>();
		
		for(int i=0; i<this.getChildren().size(); i++){
			childName = this.getChildren().get(i).getObjectName();
			childrenNames.add(childName);
			getRIBDaemon().delete(QOSCUBE_RIB_OBJECT_CLASS, childName);
			//TODO remove QoSCube from difConfiguration
		}
		
		for(int i=0; i<childrenNames.size(); i++){
			this.removeChild(childrenNames.get(i));
		}
	}
	
	@Override
	public synchronized Object getObjectValue(){
		QoSCube[] result = new QoSCube[this.getChildren().size()];
		for(int i=0; i<result.length; i++){
			result[i] = (QoSCube) this.getChildren().get(i).getObjectValue();
		}
		
		return result;
	}
}
