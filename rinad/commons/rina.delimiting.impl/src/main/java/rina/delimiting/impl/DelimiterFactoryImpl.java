package rina.delimiting.impl;

import rina.delimiting.api.Delimiter;
import rina.delimiting.api.DelimiterFactory;

public class DelimiterFactoryImpl implements DelimiterFactory{

	public Delimiter createDelimiter(String delimiterType) {
		if (delimiterType.equals(DelimiterFactory.DIF)){
			return new DIFDelimiter();
		}
		
		throw new RuntimeException("Unknown delimiter type "+delimiterType);
	}

}
