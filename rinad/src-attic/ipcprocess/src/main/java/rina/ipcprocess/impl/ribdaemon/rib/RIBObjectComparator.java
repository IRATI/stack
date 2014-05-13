package rina.ipcprocess.impl.ribdaemon.rib;

import java.util.Comparator;

import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

public class RIBObjectComparator implements Comparator<RIBObject>{

	public int compare(RIBObject o1, RIBObject o2) {
		String[] splitted1 = o1.getObjectName().split(RIBObjectNames.SEPARATOR);
		String[] splitted2 = o2.getObjectName().split(RIBObjectNames.SEPARATOR);
		
		int iterations = Math.min(splitted1.length, splitted2.length);
		
		for(int i=0; i<iterations; i++){
			if (!splitted1[i].equals(splitted2[i])){
				return splitted1[i].compareTo(splitted2[i]);
			}
		}
		
		return splitted1.length - splitted2.length;
	}

}
