package rina.utils.apps.rinaband.protobuf;

import rina.utils.apps.rinaband.StatisticsInformation;
import rina.utils.apps.rinaband.protobuf.RINABandStatisticsMessage.RINAband_statistics_t;

/**
 * Encodes and decodes RINABand Test messages
 * @author eduardgrasa
 *
 */
public class RINABandStatisticsMessageEncoder {

	public static StatisticsInformation decode(byte[] encodedObject) throws Exception {
		RINAband_statistics_t gpbRinaBandStats = RINAband_statistics_t.parseFrom(encodedObject);
		
		StatisticsInformation statsInformation = new StatisticsInformation();
		statsInformation.setClientTimeFirstSDUSent(gpbRinaBandStats.getClientTimeFirstSDUSent());
		statsInformation.setClientTimeLastSDUSent(gpbRinaBandStats.getClientTimeLastSDUSent());
		statsInformation.setClientTimeFirstSDUReceived(gpbRinaBandStats.getClientTimeFirstSDUReceived());
		statsInformation.setClientTimeLastSDUReceived(gpbRinaBandStats.getClientTimeLastSDUReceived());
		statsInformation.setServerTimeFirstSDUSent(gpbRinaBandStats.getServerTimeFirstSDUSent());
		statsInformation.setServerTimeLastSDUSent(gpbRinaBandStats.getServerTimeLastSDUSent());
		statsInformation.setServerTimeFirstSDUReceived(gpbRinaBandStats.getServerTimeFirstSDUReceived());
		statsInformation.setServerTimeLastSDUReceived(gpbRinaBandStats.getServerTimeLastSDUReceived());

		return statsInformation;
	}
	
	public static byte[] encode(StatisticsInformation statsInformation) throws Exception {
		RINAband_statistics_t gpbRinaBandStats = RINAband_statistics_t.newBuilder().
			setClientTimeFirstSDUSent(statsInformation.getClientTimeFirstSDUSent()).
			setClientTimeLastSDUSent(statsInformation.getClientTimeLastSDUSent()).
			setClientTimeFirstSDUReceived(statsInformation.getClientTimeFirstSDUReceived()).
			setClientTimeLastSDUReceived(statsInformation.getClientTimeLastSDUReceived()).
			setServerTimeFirstSDUSent(statsInformation.getServerTimeFirstSDUSent()).
			setServerTimeLastSDUSent(statsInformation.getServerTimeLastSDUSent()).
			setServerTimeFirstSDUReceived(statsInformation.getServerTimeFirstSDUReceived()).
			setServerTimeLastSDUReceived(statsInformation.getServerTimeLastSDUReceived()).
			build();
		
		return gpbRinaBandStats.toByteArray();
	}
}
