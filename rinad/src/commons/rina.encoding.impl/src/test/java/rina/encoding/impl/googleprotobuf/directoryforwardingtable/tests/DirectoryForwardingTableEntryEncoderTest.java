package rina.encoding.impl.googleprotobuf.directoryforwardingtable.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.directoryforwardingtable.DirectoryForwardingTableEntryArrayEncoder;
import rina.encoding.impl.googleprotobuf.directoryforwardingtable.DirectoryForwardingTableEntryEncoder;
import rina.flowallocator.api.DirectoryForwardingTableEntry;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Test if the serialization/deserialization mechanisms for the WhatevercastName object work
 * @author eduardgrasa
 *
 */
public class DirectoryForwardingTableEntryEncoderTest {
	
	private DirectoryForwardingTableEntry directoryForwardingTableEntry = null;
	private DirectoryForwardingTableEntry directoryForwardingTableEntry2 = null;
	private DirectoryForwardingTableEntry[] directoryForwardingTableEntries = null;
	private DirectoryForwardingTableEntryEncoder directoryForwardingTableEntryEncoder = null;
	private DirectoryForwardingTableEntryArrayEncoder directoryForwardingTableEntryArrayEncoder = null;
	
	@Before
	public void setup(){
		directoryForwardingTableEntry = new DirectoryForwardingTableEntry();
		directoryForwardingTableEntry.setAddress(34);
		directoryForwardingTableEntry.setApNamingInfo(new ApplicationProcessNamingInfo("test", "1"));
		directoryForwardingTableEntry.setTimestamp(232324134);
		
		directoryForwardingTableEntry2 = new DirectoryForwardingTableEntry();
		directoryForwardingTableEntry2.setAddress(125);
		directoryForwardingTableEntry2.setApNamingInfo(new ApplicationProcessNamingInfo("echoServer", "3"));
		directoryForwardingTableEntry2.setTimestamp(8698947856L);
		
		directoryForwardingTableEntries = new DirectoryForwardingTableEntry[]{directoryForwardingTableEntry, directoryForwardingTableEntry2};
		
		directoryForwardingTableEntryEncoder = new DirectoryForwardingTableEntryEncoder();
		directoryForwardingTableEntryArrayEncoder = new DirectoryForwardingTableEntryArrayEncoder();
	}
	
	@Test
	public void testSingle() throws Exception{
		byte[] encodedEntry = directoryForwardingTableEntryEncoder.encode(directoryForwardingTableEntry);
		for(int i=0; i<encodedEntry.length; i++){
			System.out.print(encodedEntry[i] + " ");
		}
		System.out.println();
		
		DirectoryForwardingTableEntry recoveredEntry = 
			(DirectoryForwardingTableEntry) directoryForwardingTableEntryEncoder.decode(encodedEntry, DirectoryForwardingTableEntry.class);
		Assert.assertEquals(directoryForwardingTableEntry.getApNamingInfo(), recoveredEntry.getApNamingInfo());
		Assert.assertEquals(directoryForwardingTableEntry.getAddress(), recoveredEntry.getAddress());
		Assert.assertEquals(directoryForwardingTableEntry.getTimestamp(), recoveredEntry.getTimestamp());
	}
	
	@Test
	public void testArray() throws Exception{
		byte[] encodedArray = directoryForwardingTableEntryArrayEncoder.encode(directoryForwardingTableEntries);
		for(int i=0; i<encodedArray.length; i++){
			System.out.print(encodedArray[i] + " ");
		}
		System.out.println();
		
		DirectoryForwardingTableEntry[] recoveredArray = 
			(DirectoryForwardingTableEntry[]) directoryForwardingTableEntryArrayEncoder.decode(encodedArray, DirectoryForwardingTableEntry[].class);
		Assert.assertEquals(directoryForwardingTableEntries[0].getApNamingInfo(), recoveredArray[0].getApNamingInfo());
		Assert.assertEquals(directoryForwardingTableEntries[0].getAddress(), recoveredArray[0].getAddress());
		Assert.assertEquals(directoryForwardingTableEntries[0].getTimestamp(), recoveredArray[0].getTimestamp());
		
		Assert.assertEquals(directoryForwardingTableEntries[1].getApNamingInfo(), recoveredArray[1].getApNamingInfo());
		Assert.assertEquals(directoryForwardingTableEntries[1].getAddress(), recoveredArray[1].getAddress());
		Assert.assertEquals(directoryForwardingTableEntries[1].getTimestamp(), recoveredArray[1].getTimestamp());
	}	
}