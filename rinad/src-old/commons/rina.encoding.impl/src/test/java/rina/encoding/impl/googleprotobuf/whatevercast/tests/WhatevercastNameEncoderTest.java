package rina.encoding.impl.googleprotobuf.whatevercast.tests;

import java.util.ArrayList;
import java.util.List;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.applicationprocess.api.WhatevercastName;
import rina.encoding.impl.googleprotobuf.whatevercast.WhatevercastNameArrayEncoder;
import rina.encoding.impl.googleprotobuf.whatevercast.WhatevercastNameEncoder;

/**
 * Test if the serialization/deserialization mechanisms for the WhatevercastName object work
 * @author eduardgrasa
 *
 */
public class WhatevercastNameEncoderTest {
	
	private WhatevercastName whatevercastName = null;
	private WhatevercastName whatevercastName2 = null;
	private WhatevercastName[] whatevercastNames = null;
	private WhatevercastNameEncoder whatevercastNameEncoder = null;
	private WhatevercastNameArrayEncoder whatevercastNameArrayEncoder = null;
	
	@Before
	public void setup(){
		whatevercastName = new WhatevercastName();
		whatevercastName.setName("RINA-Demo-all.DIF");
		whatevercastName.setRule("all members");
		List<byte[]> setMembers = new ArrayList<byte[]>();
		setMembers.add(new byte[]{0x01});
		setMembers.add(new byte[]{0x02});
		whatevercastName.setSetMembers(setMembers);
		
		whatevercastName2 = new WhatevercastName();
		whatevercastName2.setName("RINA-Demo-near.DIF");
		whatevercastName2.setRule("nearest member");
		setMembers = new ArrayList<byte[]>();
		setMembers.add(new byte[]{0x03});
		setMembers.add(new byte[]{0x04});
		whatevercastName2.setSetMembers(setMembers);
		
		whatevercastNames = new WhatevercastName[]{whatevercastName, whatevercastName2};
		
		whatevercastNameEncoder = new WhatevercastNameEncoder();
		whatevercastNameArrayEncoder = new WhatevercastNameArrayEncoder();
	}
	
	@Test
	public void testSingle() throws Exception{
		byte[] serializedWN = whatevercastNameEncoder.encode(whatevercastName);
		for(int i=0; i<serializedWN.length; i++){
			System.out.print(serializedWN[i] + " ");
		}
		System.out.println();
		
		WhatevercastName recoveredWhatevercastName = (WhatevercastName) whatevercastNameEncoder.decode(serializedWN, WhatevercastName.class);
		Assert.assertEquals(whatevercastName.getName(), recoveredWhatevercastName.getName());
		Assert.assertEquals(whatevercastName.getRule(), recoveredWhatevercastName.getRule());
		Assert.assertEquals(whatevercastName.getSetMembers().size(), recoveredWhatevercastName.getSetMembers().size());
		for(int i=0; i<whatevercastName.getSetMembers().size(); i++){
			Assert.assertArrayEquals(whatevercastName.getSetMembers().get(i), recoveredWhatevercastName.getSetMembers().get(i));
		}
	}
	
	@Test
	public void testArray() throws Exception{
		byte[] encodedArray = whatevercastNameArrayEncoder.encode(whatevercastNames);
		for(int i=0; i<encodedArray.length; i++){
			System.out.print(encodedArray[i] + " ");
		}
		System.out.println();
		
		WhatevercastName[] recoveredWhatevercastNames = 
			(WhatevercastName[]) whatevercastNameArrayEncoder.decode(encodedArray, WhatevercastName[].class);
		Assert.assertEquals(whatevercastNames[0].getName(), recoveredWhatevercastNames[0].getName());
		Assert.assertEquals(whatevercastNames[0].getRule(), recoveredWhatevercastNames[0].getRule());
		Assert.assertEquals(whatevercastNames[0].getSetMembers().size(), recoveredWhatevercastNames[0].getSetMembers().size());
		for(int i=0; i<whatevercastNames[0].getSetMembers().size(); i++){
			Assert.assertArrayEquals(whatevercastNames[0].getSetMembers().get(i), recoveredWhatevercastNames[0].getSetMembers().get(i));
		}
		
		Assert.assertEquals(whatevercastNames[1].getName(), recoveredWhatevercastNames[1].getName());
		Assert.assertEquals(whatevercastNames[1].getRule(), recoveredWhatevercastNames[1].getRule());
		Assert.assertEquals(whatevercastNames[1].getSetMembers().size(), recoveredWhatevercastNames[1].getSetMembers().size());
		for(int i=0; i<whatevercastNames[1].getSetMembers().size(); i++){
			Assert.assertArrayEquals(whatevercastNames[1].getSetMembers().get(i), recoveredWhatevercastNames[1].getSetMembers().get(i));
		}
	}
}