package rina.encoding.impl.googleprotobuf.qoscube;

import eu.irati.librina.QoSCube;
import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeMessage.qosCube_t;

public class QoSCubeEncoder implements Encoder{

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(QoSCube.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		QoSCubeMessage.qosCube_t gpbQoSCube = QoSCubeMessage.qosCube_t.parseFrom(serializedObject);
		return convertGPBToModel(gpbQoSCube);
	}
	
	public static QoSCube convertGPBToModel(qosCube_t gpbQoSCube){
		QoSCube qosCube = new QoSCube(GPBUtils.getString(gpbQoSCube.getName()), 
				gpbQoSCube.getQosId());
		qosCube.setAverageBandwidth(gpbQoSCube.getAverageBandwidth());
		qosCube.setAverageSduBandwidth(gpbQoSCube.getAverageSDUBandwidth());
		qosCube.setDelay(gpbQoSCube.getDelay());
		qosCube.setJitter(gpbQoSCube.getJitter());
		qosCube.setMaxAllowableGap(gpbQoSCube.getMaxAllowableGapSdu());
		qosCube.setOrderedDelivery(gpbQoSCube.getOrder());
		qosCube.setPartialDelivery(gpbQoSCube.getPartialDelivery());
		qosCube.setPeakBandwidthDuration(gpbQoSCube.getPeakBandwidthDuration());
		qosCube.setPeakSduBandwidthDuration(gpbQoSCube.getPeakSDUBandwidthDuration());
		qosCube.setUndetectedBitErrorRate(gpbQoSCube.getUndetectedBitErrorRate());
		qosCube.setEfcpPolicies(GPBUtils.getConnectionPolicies(gpbQoSCube.getEfcpPolicies()));
		
		return qosCube;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof QoSCube)){
			throw new Exception("This is not the encoder for objects of type " + QoSCube.class.toString());
		}
		
		QoSCube qosCube = (QoSCube) object;
		return convertModelToGPB(qosCube).toByteArray();
	}
	
	public static qosCube_t convertModelToGPB(QoSCube qosCube){
		QoSCubeMessage.qosCube_t gpbQoSCube = QoSCubeMessage.qosCube_t.newBuilder().
			setAverageBandwidth(qosCube.getAverageBandwidth()).
			setAverageSDUBandwidth(qosCube.getAverageSduBandwidth()).
			setDelay((int)qosCube.getDelay()).
			setJitter((int)qosCube.getJitter()).
			setMaxAllowableGapSdu(qosCube.getMaxAllowableGap()).
			setName(GPBUtils.getGPBString(qosCube.getName())).
			setOrder(qosCube.isOrderedDelivery()).
			setPartialDelivery(qosCube.isPartialDelivery()).
			setPeakBandwidthDuration((int)qosCube.getPeakBandwidthDuration()).
			setPeakSDUBandwidthDuration((int)qosCube.getPeakSduBandwidthDuration()).
			setQosId(qosCube.getId()).
			setUndetectedBitErrorRate(qosCube.getUndetectedBitErrorRate()).
			setEfcpPolicies(GPBUtils.getConnectionPoliciesType(qosCube.getEfcpPolicies())).
			build();
		
		return gpbQoSCube;
	}

}
