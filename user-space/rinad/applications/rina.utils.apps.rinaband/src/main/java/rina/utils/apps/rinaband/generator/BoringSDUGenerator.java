package rina.utils.apps.rinaband.generator;

/**
 * An SDU Generator that always returns the same type of SDU
 * @author eduardgrasa
 *
 */
public class BoringSDUGenerator extends BaseSDUGenerator{
	
	private byte[] sdu = null;
	
	public BoringSDUGenerator(int sduSize){
		super(SDUGenerator.NONE_PATTERN, sduSize);
		sdu = new byte[sduSize];
	}

	/**
	 * Assumes that the SDU will be written to the flow 
	 * before the call to getNextSDU is invoked (because 
	 * the SDU values are overriden every time this 
	 * function is called)
	 */
	public byte[] getNextSDU() {
		for(int i=0; i<sdu.length; i++){
			sdu[i] = 0x01;
		}
		
		return sdu;
	}

}
