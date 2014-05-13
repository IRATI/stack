package rina.utils.apps.rinaband.generator;

/**
 * Generates SDUs representing 4-bytes encoded integers starting at 0 
 * and incrementing from there (until it reaches Integer.MAX_VALUE, then 
 * it starts again at 0).
 * @author eduardgrasa
 *
 */
public class IncrementSDUGenerator extends BaseSDUGenerator{
	
	/**
	 * The next SDU to return
	 */
	private byte[] sdu = null;
	
	/**
	 * The next integer
	 */
	private int counter = 0;
	
	public IncrementSDUGenerator(int sduSize){
		super(SDUGenerator.INCREMENT_PATTERN, sduSize);
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
			sdu[i] = this.getNextByte();
		}
		
		return sdu;
	}
	
	private byte getNextByte(){
		byte nextByte = (byte) counter;
		
		if (counter < 255){
			counter ++;
		}else{
			counter = 0;
		}
		
		return nextByte;
	}

}
