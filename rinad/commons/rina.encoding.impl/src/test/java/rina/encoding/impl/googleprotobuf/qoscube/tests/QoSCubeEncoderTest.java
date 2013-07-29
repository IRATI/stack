package rina.encoding.impl.googleprotobuf.qoscube.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeArrayEncoder;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeEncoder;
import rina.flowallocator.api.QoSCube;

/**
 * Test if the serialization/deserialization mechanisms for the QoSCube object work
 * @author eduardgrasa
 *
 */
public class QoSCubeEncoderTest {
	
	private QoSCube qosCube = null;
	private QoSCube qosCube2 = null;
	private QoSCube[] qosCubes = null;
	private QoSCubeEncoder qosCubeEncoder = null;
	private QoSCubeArrayEncoder qosCubeArrayEncoder = null;
	
	@Before
	public void setup(){
		qosCube = new QoSCube();
		qosCube.setAverageBandwidth(1000000000);
		qosCube.setAverageSDUBandwidth(950000000);
		qosCube.setDelay(500);
		qosCube.setJitter(200);
		qosCube.setMaxAllowableGapSdu(0);
		qosCube.setName("test");
		qosCube.setOrder(true);
		qosCube.setPartialDelivery(false);
		qosCube.setPeakBandwidthDuration(20000);
		qosCube.setPeakSDUBandwidthDuration(15000);
		qosCube.setQosId(1);
		qosCube.setUndetectedBitErrorRate(new Double("1E-9").doubleValue());
		
		qosCube2 = new QoSCube();
		qosCube2.setAverageBandwidth(10000000);
		qosCube2.setAverageSDUBandwidth(9500000);
		qosCube2.setDelay(50);
		qosCube2.setJitter(2000);
		qosCube2.setMaxAllowableGapSdu(0);
		qosCube.setName("test2");
		qosCube2.setOrder(false);
		qosCube2.setPartialDelivery(true);
		qosCube2.setPeakBandwidthDuration(200);
		qosCube2.setPeakSDUBandwidthDuration(100);
		qosCube.setQosId(2);
		qosCube2.setUndetectedBitErrorRate(new Double("1E-6").doubleValue());
		
		qosCubes = new QoSCube[]{qosCube, qosCube2};
		
		qosCubeEncoder = new QoSCubeEncoder();
		qosCubeArrayEncoder = new QoSCubeArrayEncoder();
	}
	
	@Test
	public void testSingle() throws Exception{
		byte[] serializedQoSCube = qosCubeEncoder.encode(qosCube);
		for(int i=0; i<serializedQoSCube.length; i++){
			System.out.print(serializedQoSCube[i] + " ");
		}
		System.out.println();
		
		QoSCube recoveredQoSCube = (QoSCube) qosCubeEncoder.decode(serializedQoSCube, QoSCube.class);
		Assert.assertEquals(qosCube.getAverageBandwidth(), recoveredQoSCube.getAverageBandwidth());
		Assert.assertEquals(qosCube.getAverageSDUBandwidth(), recoveredQoSCube.getAverageSDUBandwidth());
		Assert.assertEquals(qosCube.getDelay(), recoveredQoSCube.getDelay());
		Assert.assertEquals(qosCube.getJitter(), recoveredQoSCube.getJitter());
		Assert.assertEquals(qosCube.getMaxAllowableGapSdu(), recoveredQoSCube.getMaxAllowableGapSdu());
		Assert.assertEquals(qosCube.getName(), recoveredQoSCube.getName());
		Assert.assertEquals(qosCube.isOrder(), recoveredQoSCube.isOrder());
		Assert.assertEquals(qosCube.isPartialDelivery(), recoveredQoSCube.isPartialDelivery());
		Assert.assertEquals(qosCube.getPeakBandwidthDuration(), recoveredQoSCube.getPeakBandwidthDuration());
		Assert.assertEquals(qosCube.getPeakSDUBandwidthDuration(), recoveredQoSCube.getPeakSDUBandwidthDuration());
		Assert.assertEquals(qosCube.getQosId(), recoveredQoSCube.getQosId());
		Assert.assertEquals(qosCube.getUndetectedBitErrorRate(), recoveredQoSCube.getUndetectedBitErrorRate(), 0);
	}
	
	@Test
	public void testArray() throws Exception{
		byte[] encodedQosCubes = qosCubeArrayEncoder.encode(qosCubes);
		for(int i=0; i<encodedQosCubes.length; i++){
			System.out.print(encodedQosCubes[i] + " ");
		}
		System.out.println();
		
		QoSCube[] recoveredQoSCubes = (QoSCube[]) qosCubeArrayEncoder.decode(encodedQosCubes, QoSCube[].class);
		Assert.assertEquals(qosCubes[0].getAverageBandwidth(), recoveredQoSCubes[0].getAverageBandwidth());
		Assert.assertEquals(qosCubes[0].getAverageSDUBandwidth(), recoveredQoSCubes[0].getAverageSDUBandwidth());
		Assert.assertEquals(qosCubes[0].getDelay(), recoveredQoSCubes[0].getDelay());
		Assert.assertEquals(qosCubes[0].getJitter(), recoveredQoSCubes[0].getJitter());
		Assert.assertEquals(qosCubes[0].getMaxAllowableGapSdu(), recoveredQoSCubes[0].getMaxAllowableGapSdu());
		Assert.assertEquals(qosCubes[0].getName(), recoveredQoSCubes[0].getName());
		Assert.assertEquals(qosCubes[0].isOrder(), recoveredQoSCubes[0].isOrder());
		Assert.assertEquals(qosCubes[0].isPartialDelivery(), recoveredQoSCubes[0].isPartialDelivery());
		Assert.assertEquals(qosCubes[0].getPeakBandwidthDuration(), recoveredQoSCubes[0].getPeakBandwidthDuration());
		Assert.assertEquals(qosCubes[0].getPeakSDUBandwidthDuration(), recoveredQoSCubes[0].getPeakSDUBandwidthDuration());
		Assert.assertEquals(qosCubes[0].getQosId(), recoveredQoSCubes[0].getQosId());
		Assert.assertEquals(qosCubes[0].getUndetectedBitErrorRate(), recoveredQoSCubes[0].getUndetectedBitErrorRate(), 0);
		
		Assert.assertEquals(qosCubes[1].getAverageBandwidth(), recoveredQoSCubes[1].getAverageBandwidth());
		Assert.assertEquals(qosCubes[1].getAverageSDUBandwidth(), recoveredQoSCubes[1].getAverageSDUBandwidth());
		Assert.assertEquals(qosCubes[1].getDelay(), recoveredQoSCubes[1].getDelay());
		Assert.assertEquals(qosCubes[1].getJitter(), recoveredQoSCubes[1].getJitter());
		Assert.assertEquals(qosCubes[1].getMaxAllowableGapSdu(), recoveredQoSCubes[1].getMaxAllowableGapSdu());
		Assert.assertEquals(qosCubes[1].getName(), recoveredQoSCubes[1].getName());
		Assert.assertEquals(qosCubes[1].isOrder(), recoveredQoSCubes[1].isOrder());
		Assert.assertEquals(qosCubes[1].isPartialDelivery(), recoveredQoSCubes[1].isPartialDelivery());
		Assert.assertEquals(qosCubes[1].getPeakBandwidthDuration(), recoveredQoSCubes[1].getPeakBandwidthDuration());
		Assert.assertEquals(qosCubes[1].getPeakSDUBandwidthDuration(), recoveredQoSCubes[1].getPeakSDUBandwidthDuration());
		Assert.assertEquals(qosCubes[1].getQosId(), recoveredQoSCubes[1].getQosId());
		Assert.assertEquals(qosCubes[1].getUndetectedBitErrorRate(), recoveredQoSCubes[1].getUndetectedBitErrorRate(), 0);
	}
}