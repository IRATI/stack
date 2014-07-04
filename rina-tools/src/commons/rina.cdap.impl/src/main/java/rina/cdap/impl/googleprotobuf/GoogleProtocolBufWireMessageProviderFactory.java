package rina.cdap.impl.googleprotobuf;

import rina.cdap.impl.WireMessageProvider;
import rina.cdap.impl.WireMessageProviderFactory;

public class GoogleProtocolBufWireMessageProviderFactory implements WireMessageProviderFactory {

	public WireMessageProvider createWireMessageProvider() {
		return new GoogleProtocolBufWireMessageProvider();
	}

}