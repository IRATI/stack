package rina.ipcprocess.impl.ribobjects;

import java.util.ArrayList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.applicationprocess.api.WhatevercastName;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.impl.IPCProcess;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class WhatevercastNameSetRIBObject extends BaseRIBObject{
	
	private static final Log log = LogFactory.getLog(WhatevercastNameSetRIBObject.class);
	private IPCProcess ipcProcess = null;
	
	public WhatevercastNameSetRIBObject(){
		super(WhatevercastName.WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), 
				WhatevercastName.WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME);
		this.ipcProcess = IPCProcess.getInstance();
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
			WhatevercastName[] whatevercastNames = (WhatevercastName[])
				this.getEncoder().decode(cdapMessage.getObjValue().getByteval(), WhatevercastName[].class);
			this.getRIBDaemon().create(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName(), 
					whatevercastNames, null);
		}catch(Exception ex){
			log.error(ex);
			ex.printStackTrace();
		}
	}
	
	@Override
	public synchronized void create(String objectClass, long objectInstance, String objectName, Object object) throws RIBDaemonException{
		if (object instanceof WhatevercastName){
			SimpleSetMemberRIBObject ribObject = new SimpleSetMemberRIBObject(
					WhatevercastName.WHATEVERCAST_NAME_RIB_OBJECT_CLASS, objectName, (WhatevercastName) object);
			this.addChild(ribObject);
			getRIBDaemon().addRIBObject(ribObject);
		}else if (object instanceof WhatevercastName[]){
			WhatevercastName[] whatevercastNames = (WhatevercastName[]) object;
			String candidateObjectName = null;
			
			for(int i=0; i<whatevercastNames.length; i++){
				candidateObjectName = this.getObjectName() + RIBObjectNames.SEPARATOR + whatevercastNames[i].getRule();
				if (!this.hasChild(candidateObjectName)){
					SimpleSetMemberRIBObject ribObject = new SimpleSetMemberRIBObject(
							WhatevercastName.WHATEVERCAST_NAME_RIB_OBJECT_CLASS, candidateObjectName, whatevercastNames[i]);
					this.addChild(ribObject);
					getRIBDaemon().addRIBObject(ribObject);
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
			getRIBDaemon().delete(WhatevercastName.WHATEVERCAST_NAME_RIB_OBJECT_CLASS, childName);
		}
		
		for(int i=0; i<childrenNames.size(); i++){
			this.removeChild(childrenNames.get(i));
		}
	}
	
	@Override
	public synchronized Object getObjectValue(){
		WhatevercastName[] result = new WhatevercastName[this.getChildren().size()];
		for(int i=0; i<result.length; i++){
			result[i] = (WhatevercastName) this.getChildren().get(i).getObjectValue();
		}
		
		return result;
	}
}
