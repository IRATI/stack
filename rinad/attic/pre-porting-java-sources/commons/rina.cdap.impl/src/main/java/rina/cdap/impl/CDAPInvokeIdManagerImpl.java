package rina.cdap.impl;

import java.util.ArrayList;
import java.util.List;

import rina.cdap.api.CDAPInvokeIdManager;

/**
 * It will always try to use short invokeIds (as close to 1 as possible)
 * 
 * TODO: implement housekeeping of invokeIds (remove them from the usedInvokeIds 
 * space after X units of time)
 * 
 * @author eduardgrasa
 *
 */
public class CDAPInvokeIdManagerImpl implements CDAPInvokeIdManager{

	private List<Integer> usedInvokeIds = null;
	
	public CDAPInvokeIdManagerImpl(){
		usedInvokeIds = new ArrayList<Integer>();
	}
	
	public synchronized void freeInvokeId(int invokeId) {
		usedInvokeIds.remove(new Integer(invokeId));
	}

	public synchronized int getInvokeId() {
		int candidate = 1;
		while (usedInvokeIds.contains(new Integer(candidate))){
			candidate = candidate + 1;
		}
		
		usedInvokeIds.add(new Integer(candidate));
		return candidate;
	}

	public synchronized void reserveInvokeId(int invokeId) {
		usedInvokeIds.add(new Integer(invokeId));
	}

}
