package rina.delimiting.impl.tests;

import java.util.ArrayList;
import java.util.List;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.delimiting.api.Delimiter;
import rina.delimiting.impl.DIFDelimiter;

/**
 * Basic test for the DIF delimiter
 * @author eduardgrasa
 *
 */
public class DIFDelimiterTest {
	
	private Delimiter delimiter = null;
	
	@Before
	public void setup(){
		delimiter = new DIFDelimiter();
	}
	
	@Test
	public void testDelimitSingleSdu(){
		byte[] sdu = new String("The rain in Spain falls mainly in the plain").getBytes();
		printArray(sdu);
		byte[] delimitedSdu = delimiter.getDelimitedSdu(sdu);
		printArray(delimitedSdu);
		List<byte[]> recoveredSdus = delimiter.getRawSdus(delimitedSdu);
		printArray(recoveredSdus.get(0));
		
		Assert.assertEquals(1, recoveredSdus.size());
		Assert.assertArrayEquals(sdu, recoveredSdus.get(0));
		
		sdu = new String("The Pouzin Society's purpose is to provide a forum for developing viable " +
				"solutions to the current Internet architecture crisis. Membership is open to qualified " +
				"members of the networking community, both academic and commercial. Please send an email to " +
				"info@pouzinsociety.org to introduce yourself and request more information").getBytes();
		printArray(sdu);
		delimitedSdu = delimiter.getDelimitedSdu(sdu);
		printArray(delimitedSdu);
		recoveredSdus = delimiter.getRawSdus(delimitedSdu);
		printArray(recoveredSdus.get(0));
		
		Assert.assertEquals(1, recoveredSdus.size());
		Assert.assertArrayEquals(sdu, recoveredSdus.get(0));
		System.out.println();
	}
	
	@Test
	public void testDelimitingMultipleSdus(){
		byte[] sdu1 = new String("The rain in Spain falls mainly in the plain").getBytes();
		printArray(sdu1);
		byte[] sdu2 = new String("The Pouzin Society's purpose is to provide a forum for developing viable " +
				"solutions to the current Internet architecture crisis. Membership is open to qualified " +
				"members of the networking community, both academic and commercial. Please send an email to " +
				"info@pouzinsociety.org to introduce yourself and request more information").getBytes();
		printArray(sdu2);
		
		List<byte[]> originalSdus = new ArrayList<byte[]>();
		originalSdus.add(sdu1);
		originalSdus.add(sdu2);
		byte[] delimitedSdu = delimiter.getDelimitedSdus(originalSdus);
		printArray(delimitedSdu);
		List<byte[]> recoveredSdus = delimiter.getRawSdus(delimitedSdu);
		printArray(recoveredSdus.get(0));
		printArray(recoveredSdus.get(1));
		
		Assert.assertEquals(2, recoveredSdus.size());
		Assert.assertArrayEquals(sdu1, recoveredSdus.get(0));
		Assert.assertArrayEquals(sdu2, recoveredSdus.get(1));
	}
	
	@Test
	public void testReadZeroLengthSDU(){
		byte[] toRead = new byte[4];
		toRead[0] = 0;
		int result = delimiter.readVarint32(toRead, 1);
		System.out.println("\n"+result);
		Assert.assertEquals(0, result);
	}
	
	
	private void printArray(byte[] array){
		for(int i=0; i<array.length; i++){
			System.out.print(array[i] + " ");
		}
		System.out.println();
	}

}
