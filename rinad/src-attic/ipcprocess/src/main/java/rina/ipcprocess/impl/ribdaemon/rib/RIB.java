package rina.ipcprocess.impl.ribdaemon.rib;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;

/**
 * Stores the RIB information
 */
public class RIB{
    private Map<String, RIBObject> rib = null;
     
    /**
     * Default ctor.
     */
    public RIB(){
    	rib = new ConcurrentHashMap<String, RIBObject>();
    }
    
    /**
     * Given an objectname of the form "substring\0substring\0...substring" locate 
     * the RIBObject that corresponds to it
     * @param objectName
     * @return
     */
    public RIBObject getRIBObject(String objectName) throws RIBDaemonException{
    	RIBObject ribObject = null;
    	
    	synchronized(this){
    		ribObject = rib.get(objectName);
    	}
    	
    	if (ribObject == null){
    		throw new RIBDaemonException(
    				RIBDaemonException.OBJECTNAME_NOT_PRESENT_IN_THE_RIB, 
    				"Could not find an object named "+objectName+" in the RIB");
    	}
    	
    	return ribObject;
    }
    
    public synchronized void addRIBObject(RIBObject ribObject) throws RIBDaemonException{
    	if (rib.get(ribObject.getObjectName()) != null){
    		throw new RIBDaemonException(RIBDaemonException.OBJECT_ALREADY_EXISTS, 
    				"There is already an object with objectname "+
    				ribObject.getObjectName()+" in the RIB.");
    	}
    	
    	rib.put(ribObject.getObjectName(), ribObject);
    }
    
    public synchronized RIBObject removeRIBObject(String objectName){
    	return rib.remove(objectName);
    }
    
    public List<RIBObject> getRIBObjects(){
    	List<RIBObject> result = null;
    	Iterator<String> iterator = null;
    	
    	synchronized(this){
    		result = new ArrayList<RIBObject>();
    		iterator = rib.keySet().iterator();
    		while (iterator.hasNext()){
    			String objectName = iterator.next();
    			result.add(rib.get(objectName));
    		}
    	}
    	
    	Collections.sort(result, new RIBObjectComparator());
    	return result;
    }
}
