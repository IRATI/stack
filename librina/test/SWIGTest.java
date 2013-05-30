import eu.irati.librina.*;

public class SWIGTest {

	static {
               //System.out.println("java.library.path = " + System.getProperties().getProperty("java.library.path"));

                System.loadLibrary("rina_java");
	  }

	public static void main(String[] args) {
		System.out.println("************ TESTING LIBRINA-APPLICATION ************");
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
		
		System.out.println("\nCALLING ALLOCATE FLOW REQUEST");
		int port_id = rina.allocate_flow_request(sourceAppName, destAppName, flowSpec);
		System.out.println("Flow allocated, port id is "+port_id);
		
		System.out.println("\nCALLING ALLOCATE FLOW RESPONSE");
		rina.allocate_flow_response(port_id, "Flow accepted");
		System.out.println("Accepted flow allocation");
		
		System.out.println("\nCALLING DEALLOCATE FLOW");
		rina.deallocate_flow(port_id);
		System.out.println("Flow deallocated");
		
		name_t difName = new name_t();
		difName.setProcess_name("rina-demo.DIF");
		System.out.println("\nCALLING REGISTER APPLICATION");
		rina.register_application(sourceAppName, difName);
		System.out.println("Application registered");
		
		System.out.println("\nCALLING UNREGISTER APPLICATION");
		rina.unregister_application(sourceAppName, difName);
		System.out.println("Application unregistered");
		
		buffer_t sdu = new buffer_t();
		byte[] byteArray = "This is test data to be sent".getBytes();
		sdu.setData(byteArray);
		sdu.setSize(byteArray.length);
		System.out.println("\nCALLING WRITE SDU");
		rina.write_sdu(port_id, sdu);
		System.out.println("Wrote SDU");
		
		event_t event = new event_t();
		System.out.println("\nCALLING EVENT WAIT");
		rina.ev_wait(event);
		System.out.println("Event wait called, got an event");
		System.out.println("Got an event of type " +event.getType());
		
		System.out.println("\nCALLING EVENT POLL");
		rina.ev_poll(event);
		System.out.println("Event poll called, got an event");
		System.out.println("Got an event of type " +event.getType());
		
		System.out.println("\nCALLING GET DIF PROPERTIES");
		SWIGTYPE_p_int sizepointer = rina.new_intp();
		dif_properties_t array = rina.get_dif_properties(difName, sizepointer);
		int members = rina.intp_value(sizepointer);
		rina.delete_intp(sizepointer);
		System.out.println("Get DIF properties called, got properties of "+members+" DIFs");
		if (members > 0){
			dif_properties_t allDIFProperties[] = new dif_properties_t[members];
			for(int i=0; i<members; i++){
				allDIFProperties[i] = rina.dif_properties_t_array_getitem(array, i);
				System.out.println("DIF name: "+allDIFProperties[i].getDif_name().getProcess_name());
				System.out.println("   Max SDU size: "+allDIFProperties[i].getMax_sdu_size());
				if (allDIFProperties[i].getQos_cubes() != null){
					qos_cube_t qos_array = allDIFProperties[i].getQos_cubes().getElements();
					qos_cube_t qosCube = null;
					for(int j=0; j<allDIFProperties[i].getQos_cubes().getSize(); j++){
						qosCube = rina.qos_cube_t_array_getitem(qos_array, j);
						System.out.println("   QoS cube name: "+qosCube.getName());
						System.out.println("      Id: "+qosCube.getId());
						System.out.println("      Delay: "+qosCube.getFlow_spec().getDelay());
						System.out.println("      Jitter: "+qosCube.getFlow_spec().getJitter());
					}
				}
			}
		}
		rina.delete_dif_properties_t_array(array);
		
		System.out.println("\n************ TESTING LIBRINA-IPCMANAGER ************");
		System.out.println("\nCALLING IPCM CREATE");
		name_t ipcProcessName = new name_t();
		ipcProcessName.setProcess_name("/ipcprocess/Barcelona.i2CAT");
		ipcProcessName.setProcess_instance("1");
		dif_type_t dif_type = dif_type_t.DIF_TYPE_SHIM_ETH;
		rina.ipcm_create(ipcProcessName, dif_type, 25);
		System.out.println("Created IPC Process");
		
		System.out.println("\nCALLING IPCM DESTROY");
		rina.ipcm_destroy(25);
		System.out.println("Destroyed IPC Process");
		
		System.out.println("\nCALLING IPCM ASSIGN");
		dif_configuration_t difConfiguration = new dif_configuration_t();
		difConfiguration.setDif_type(dif_type_t.DIF_TYPE_SHIM_ETH);
		difConfiguration.setDif_name(difName);
		difConfiguration.setMax_sdu_size(1500);
		array_of_qos_cube_t qos_cubes = new array_of_qos_cube_t();
		qos_cube_t array_head = rina.new_qos_cube_t_array(1);
		qos_cube_t value = new qos_cube_t();
		value.setId(2);
		value.setName("Unreliable");
		flow_spec_t flow_spec = new flow_spec_t();
		flow_spec.setDelay(-1);
		flow_spec.setJitter(-1);
		value.setFlow_spec(flow_spec);
		rina.qos_cube_t_array_setitem(array_head, 0, value);
		qos_cubes.setElements(array_head);
		qos_cubes.setSize(1);
		difConfiguration.setCubes(qos_cubes);
		specific_configuration_t specificConf = new specific_configuration_t();
		shim_eth_dif_config_t shim_eth_config = new shim_eth_dif_config_t();
		shim_eth_config.setDevice_name("eth0.4");
		specificConf.setShim_eth_dif_config(shim_eth_config);
		difConfiguration.setSpecific_conf(specificConf);
		rina.ipcm_assign(25, difConfiguration);
		System.out.println("Assigned IPC Process to DIF");
		rina.delete_qos_cube_t_array(array_head);
		
		System.out.println("\nCALLING IPCM NOTIFY REGISTER");
		rina.ipcm_notify_register(25, difName);
		System.out.println("Notified registartion to a DIF to IPC Process");
		
		System.out.println("\nCALLING IPCM NOTIFY UNREGISTER");
		rina.ipcm_notify_unregister(25, difName);
		System.out.println("Notified unregistartion from a DIF to IPC Process");
		
		System.out.println("\nCALLING IPCM ENROLL");
		name_t supportingDIF = new name_t();
		supportingDIF.setProcess_name("test.DIF");
		rina.ipcm_enroll(25, difName, supportingDIF);
		System.out.println("Made an IPC Process enroll to a DIF");
		
		System.out.println("\nCALLING IPCM DISCONNECT");
		name_t neigbhor = new name_t();
		neigbhor.setProcess_name("/ipcprocess/Pisa.Nextworks");
		neigbhor.setProcess_instance("1");
		rina.ipcm_disconnect(25, neigbhor);
		System.out.println("Made an IPC Process disconnect from a neighbor");
		
		System.out.println("\nCALLING IPCM REGISTER APP");
		rina.ipcm_register_app(sourceAppName, 25);
		System.out.println("Requested to register an application to a DIF");
		
		System.out.println("\nCALLING IPCM NOTIFY APP REGISTRATION");
		rina.ipcm_notify_app_registration(sourceAppName, "registration ok");
		System.out.println("Notified an application about its registration request");
		
		System.out.println("\nCALLING IPCM UNREGISTER APP");
		rina.ipcm_unregister_app(sourceAppName, 25);
		System.out.println("Requested to unregister an application fro a DIF");
		
		System.out.println("\nCALLING IPCM NOTIFY APP UNREGISTRATION");
		rina.ipcm_notify_app_unregistration(sourceAppName, "unregistration ok");
		System.out.println("Notified an application about its unregistration request");
		
		System.out.println("\nCALLING IPCM ALLOCATE FLOW");
		rina.ipcm_allocate_flow(sourceAppName, destAppName, flowSpec, port_id, 25);
		System.out.println("Requested an IPC Process to allocate a flow");
		
		System.out.println("\nCALLING IPCM NOTIFY FLOW ALLOCATION");
		rina.ipcm_notify_flow_allocation(sourceAppName, port_id, 25, "flow allocation ok");
		System.out.println("Notified an application about the allocation of a flow");
		
		System.out.println("\nCALLING IPCM QUERY RIB");
		sizepointer = rina.new_intp();
		rib_object_t ribObjectsArrayHead = rina.ipcm_query_rib(25, "myClass", "/dif", 0, "test=filter", sizepointer);
		members = rina.intp_value(sizepointer);
		System.out.println("Queried RIB of IPC Process, got "+members+" RIB objects back");
		rina.delete_intp(sizepointer);
		rib_object_t currentRIBObject = null;
		for(int i=0; i<members; i++){
			currentRIBObject = rina.rib_object_t_array_getitem(ribObjectsArrayHead, i);
			System.out.println("Object "+i+" class: "+currentRIBObject.getObject_class());
			System.out.println("Object "+i+" name: "+currentRIBObject.getObject_name());
			System.out.println("Object "+i+" instance: "+currentRIBObject.getObject_instance());
			switch(currentRIBObject.getValue_type().swigValue()){
			case 0:
				System.out.println("Object "+i+" value: "+currentRIBObject.getObject_value().getShort_value());
				break;
			case 1:
				System.out.println("Object "+i+" value: "+currentRIBObject.getObject_value().getInt_value());
				break;
			case 2:
				System.out.println("Object "+i+" value: "+currentRIBObject.getObject_value().getLong_value());
				break;
			case 3:
				System.out.println("Object "+i+" value: "+currentRIBObject.getObject_value().getFloat_value());
				break;
			case 4:
				System.out.println("Object "+i+" value: "+currentRIBObject.getObject_value().getDouble_value());
				break;
			case 5:
				System.out.println("Object "+i+" value: "+currentRIBObject.getObject_value().getString_value());
				break;
			case 6:
				System.out.println("Object "+i+" value: "+currentRIBObject.getObject_value().getEncoded_value());
				break;
			}
		}
		rina.delete_rib_object_t_array(ribObjectsArrayHead);
	}
}
