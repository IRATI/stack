package rina.utils.apps.echo.utils;

import eu.irati.librina.ApplicationProcessNamingInformation;

public class Utils {

	public static String getApplicationNamingInformationCode(
			ApplicationProcessNamingInformation name){
		String result = "-";
		if (name.getProcessName() != null){
			result = result + name.getProcessName();
		}
		result = result + "-";
		if (name.getProcessInstance() != null){
			result = result + name.getProcessInstance();
		}
		result = result + "-";
		if (name.getEntityName() != null){
			result = result + name.getEntityName();
		}
		result = result + "-";
		if (name.getEntityInstance() != null){
			result = result + name.getEntityInstance();
		}
		result = result + "-";
		
		return result;
	}

}
