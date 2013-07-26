package rina.encoding.impl.googleprotobuf.neighbor.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.neighbor.NeighborArrayEncoder;
import rina.encoding.impl.googleprotobuf.neighbor.NeighborEncoder;
import rina.enrollment.api.Neighbor;


/**
 * Test if the serialization/deserialization mechanisms for the WhatevercastName object work
 * @author eduardgrasa
 *
 */
public class NeighborEncoderTest {
	
	private Neighbor neighbor = null;
	private Neighbor neighbor2 = null;
	private Neighbor[] neighbors = null;
	private NeighborEncoder neighborEncoder = null;
	private NeighborArrayEncoder neighborArrayEncoder = null;
	
	@Before
	public void setup(){
		neighbor = new Neighbor();
		neighbor.setApplicationProcessName("B");
		neighbor.setApplicationProcessInstance("1234");
		neighbor.setAddress(12);
		
		neighbor2 = new Neighbor();
		neighbor2.setApplicationProcessName("C");
		neighbor2.setApplicationProcessInstance("5678");
		neighbor2.setAddress(242);
		
		neighbors = new Neighbor[]{neighbor, neighbor2};
		
		neighborEncoder = new NeighborEncoder();
		neighborArrayEncoder = new NeighborArrayEncoder();
	}
	
	@Test
	public void testSingle() throws Exception{
		byte[] encodedNeighbor = neighborEncoder.encode(neighbor);
		for(int i=0; i<encodedNeighbor.length; i++){
			System.out.print(encodedNeighbor[i] + " ");
		}
		System.out.println();
		
		Neighbor recoveredN = (Neighbor) neighborEncoder.decode(encodedNeighbor, Neighbor.class);
		Assert.assertEquals(neighbor.getApplicationProcessName(), recoveredN.getApplicationProcessName());
		Assert.assertEquals(neighbor.getApplicationProcessInstance(), recoveredN.getApplicationProcessInstance());
		Assert.assertEquals(neighbor.getAddress(), recoveredN.getAddress());
	}
	
	@Test
	public void testArray() throws Exception{
		byte[] encodedArray = neighborArrayEncoder.encode(neighbors);
		for(int i=0; i<encodedArray.length; i++){
			System.out.print(encodedArray[i] + " ");
		}
		System.out.println();
		
		Neighbor[] recoveredNs = (Neighbor[]) neighborArrayEncoder.decode(encodedArray, Neighbor[].class);
		Assert.assertEquals(neighbors[0].getApplicationProcessName(), recoveredNs[0].getApplicationProcessName());
		Assert.assertEquals(neighbors[0].getApplicationProcessInstance(), recoveredNs[0].getApplicationProcessInstance());
		Assert.assertEquals(neighbors[0].getAddress(), recoveredNs[0].getAddress());
		
		Assert.assertEquals(neighbors[1].getApplicationProcessName(), recoveredNs[1].getApplicationProcessName());
		Assert.assertEquals(neighbors[1].getApplicationProcessInstance(), recoveredNs[1].getApplicationProcessInstance());
		Assert.assertEquals(neighbors[1].getAddress(), recoveredNs[1].getAddress());
	}	
}