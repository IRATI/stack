package rina.encoding.impl.googleprotobuf.qoscube;

import rina.encoding.api.BaseEncoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeMessage.qosCube_t;
import rina.flowallocator.api.QoSCube;

public class QoSCubeEncoder extends BaseEncoder{

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(QoSCube.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		QoSCubeMessage.qosCube_t gpbQoSCube = QoSCubeMessage.qosCube_t.parseFrom(serializedObject);
		return convertGPBToModel(gpbQoSCube);
	}
	
	public static QoSCube convertGPBToModel(qosCube_t gpbQoSCube){
		QoSCube qosCube = new QoSCube();
		qosCube.setAverageBandwidth(gpbQoSCube.getAverageBandwidth());
		qosCube.setAverageSDUBandwidth(gpbQoSCube.getAverageSDUBandwidth());
		qosCube.setDelay(gpbQoSCube.getDelay());
		qosCube.setJitter(gpbQoSCube.getJitter());
		qosCube.setMaxAllowableGapSdu(gpbQoSCube.getMaxAllowableGapSdu());
		qosCube.setName(GPBUtils.getString(gpbQoSCube.getName()));
		qosCube.setOrder(gpbQoSCube.getOrder());
		qosCube.setPartialDelivery(gpbQoSCube.getPartialDelivery());
		qosCube.setPeakBandwidthDuration(gpbQoSCube.getPeakBandwidthDuration());
		qosCube.setPeakSDUBandwidthDuration(gpbQoSCube.getPeakSDUBandwidthDuration());
		qosCube.setQosId(gpbQoSCube.getQosId());
		qosCube.setUndetectedBitErrorRate(gpbQoSCube.getUndetectedBitErrorRate());
		
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
			setAverageSDUBandwidth(qosCube.getAverageSDUBandwidth()).
			setDelay(qosCube.getDelay()).
			setJitter(qosCube.getJitter()).
			setMaxAllowableGapSdu(qosCube.getMaxAllowableGapSdu()).
			setName(GPBUtils.getGPBString(qosCube.getName())).
			setOrder(qosCube.isOrder()).
			setPartialDelivery(qosCube.isPartialDelivery()).
			setPeakBandwidthDuration(qosCube.getPeakBandwidthDuration()).
			setPeakSDUBandwidthDuration(qosCube.getPeakSDUBandwidthDuration()).
			setQosId(qosCube.getQosId()).
			setUndetectedBitErrorRate(qosCube.getUndetectedBitErrorRate()).
			build();
		
		return gpbQoSCube;
	}

}
