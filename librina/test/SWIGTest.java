import eu.irati.librina.*;

public class SWIGTest {

	static {
               //System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));

                System.loadLibrary("rina_java");
	  }

	public static void main(String[] args) {
		name_t sourceAppName = new name_t();
		sourceAppName.setProcess_name("test/application/name/source");
		sourceAppName.setProcess_instance("12");
		sourceAppName.setEntity_name("main");
		sourceAppName.setEntity_instance("1");
		
		name_t destAppName = new name_t();
		destAppName.setProcess_name("test/application/name/destination");
		destAppName.setProcess_instance("23");
		destAppName.setEntity_name("main");
		destAppName.setEntity_instance("2");
		
		uint_range_t aux = null;
		flow_spec_t flowSpec = new flow_spec_t();
		aux = new uint_range_t();
		aux.setMin_value(10000);
		aux.setMax_value(20000);
		flowSpec.setAverage_bandwidth(aux);
		flowSpec.setDelay(50);
		flowSpec.setJitter(25);
		flowSpec.setOrdered_delivery(1);
		flowSpec.setPartial_delivery(1);
		flowSpec.setUndetected_bit_error_rate(0.00000000009);
		
		System.out.println("Calling allocate flow request");
		int port_id = rina.allocate_flow_request(sourceAppName, destAppName, flowSpec);
		System.out.println("Flow allocated, port id is "+port_id);
		
		System.out.println("Calling allocate flow response");
		rina.allocate_flow_response(port_id, "Flow accepted");
		System.out.println("Accepted flow allocation");
		
		System.out.println("Calling deallocate flow");
		rina.deallocate_flow(port_id);
		System.out.println("Flow deallocated");
		
		name_t difName = new name_t();
		difName.setProcess_name("rina-demo.DIF");
		System.out.println("Calling register application");
		rina.register_application(sourceAppName, difName);
		System.out.println("Application registered");
		
		System.out.println("Calling unregister application");
		rina.unregister_application(sourceAppName, difName);
		System.out.println("Application unregistered");
		
		buffer_t sdu = new buffer_t();
		byte[] byteArray = "This is test data to be sent".getBytes();
		sdu.setData(byteArray);
		sdu.setSize(byteArray.length);
		System.out.println("Calling write SDU");
		rina.write_sdu(port_id, sdu);
		System.out.println("Wrote SDU");
		
		event_t event = new event_t();
		System.out.println("Calling event wait");
		rina.ev_wait(event);
		System.out.println("Event wait called, got an event");
		System.out.println("Got an event of type " +event.getType());
		
		System.out.println("Calling event poll");
		rina.ev_poll(event);
		System.out.println("Event poll called, got an event");
		System.out.println("Got an event of type " +event.getType());
		
		dif_properties_t array = rina.new_dif_properties_t_array(10);
		System.out.println("Calling get DIF properties");
		int members = rina.get_dif_properties(difName, array);
		System.out.println("Get DIF properties called, got properties of "+members+" DIFs");
		if (members > 0){
			dif_properties_t allDIFProperties[] = new dif_properties_t[members];
			for(int i=0; i<members; i++){
				allDIFProperties[i] = rina.dif_properties_t_array_getitem(array, i);
				System.out.println("DIF name: "+allDIFProperties[i].getDif_name().getProcess_name());
				System.out.println("   Max SDU size: "+allDIFProperties[i].getMax_sdu_size());
			}
		}
		
	}
}
