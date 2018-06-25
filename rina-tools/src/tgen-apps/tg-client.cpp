#include <tclap/CmdLine.h>
#include <random>
#include <queue>
#include <stdlib.h>
#include <time.h>

#include "test_client_base.h"

class DataClient : public TestClientBase {
public:
	DataClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance,
		   const std::string DIF, int TestDuration, unsigned int _packet_size,
		   const unsigned long long _ratebps, bool verbose, size_t max_sdu_size, unsigned int loss, unsigned int delay,
		   int FlowIdent, int QoSIdent) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			TestDuration, verbose, max_sdu_size, loss, delay, FlowIdent, QoSIdent) {
		if (_packet_size < sizeof(dataSDU)) {
			sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			sdu_size = BUFF_SIZE;
		} else {
			sdu_size = _packet_size;
		}

		long long interval_ns;
		interval_ns = 1000000000L;		// ns/s
		interval_ns *= sdu_size * 8L;	// b/B
		interval_ns /= _ratebps;			// b/s
		interval = std::chrono::nanoseconds(interval_ns);
	}
	virtual ~DataClient(){};

protected:
	int sdu_size;
	std::chrono::nanoseconds interval;

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t =
			std::chrono::system_clock::now();

		while (t < Endtime) {
			if (SendData(sdu_size) != sdu_size) return -1;
			t = std::chrono::system_clock::now() + interval;
			if (t > Endtime) t = Endtime;
			std::this_thread::sleep_until(t);
		};

		return 0;
	}
};

class Exponential : public TestClientBase {
public:
	Exponential(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance,
		    const std::string DIF, int TestDuration, unsigned int packet_size,
		    const unsigned long long avg_flow_rate_bps, bool verbose, size_t max_sdu_size, unsigned int loss, unsigned int delay,
			int FlowIdent, int QoSIdent) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			TestDuration, verbose, max_sdu_size, loss, delay, FlowIdent, QoSIdent) {

		sdu_size = packet_size;
		if (sdu_size < 0 || sdu_size < sizeof(dataSDU)) {
			sdu_size = sizeof(dataSDU);
		} else if (sdu_size > BUFF_SIZE) {
			sdu_size = BUFF_SIZE;
		}

		avg_iat = 1000000000.0; //ns
		avg_iat *= (sdu_size*8L);
		avg_iat /= avg_flow_rate_bps;
	}

protected:
	double avg_iat;
	int sdu_size;

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t =
			std::chrono::system_clock::now();

		std::default_random_engine generator(t.time_since_epoch().count());
		std::exponential_distribution<double> iat_distribution(1.0 / avg_iat); //nanoseconds

		while (std::chrono::system_clock::now() < Endtime) {
			if (SendData(sdu_size) != sdu_size) return -1;
			t += std::chrono::nanoseconds((long long int)iat_distribution(generator));
			if(t > Endtime) t = Endtime;
			std::this_thread::sleep_until(t);
		};

		return 0;
	}
};

class PoissonClient : public TestClientBase {
public:
	PoissonClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance,
		      const std::string DIF, int TestDuration, unsigned int avg_pdu_size,
		      const unsigned long long max_flow_rate_bps, const double load,  bool verbose, size_t max_sdu_size,
		      unsigned int loss, unsigned int delay, int FlowIdent, int QoSIdent) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			       TestDuration, verbose, max_sdu_size, loss, delay, FlowIdent, QoSIdent) {

		avg_iat = 8000000000.0; //ns
		avg_iat *= avg_pdu_size;
		avg_iat /= max_flow_rate_bps;

		avg_ht = avg_iat * load;

		RateBpns = max_flow_rate_bps;
		RateBpns /= 8000000000.0;
	}

protected:
	double avg_iat, avg_ht;
	double RateBpns;

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t =
			std::chrono::system_clock::now();

		std::default_random_engine generator(t.time_since_epoch().count());
		std::exponential_distribution<double> iat_distribution(1.0 / avg_iat); //nanoseconds
		std::exponential_distribution<double> ht_distribution(1.0 / avg_ht); //nanoseconds

		auto NextCreate = t;
		auto NextFlowFree = t;
		std::queue<std::chrono::time_point<std::chrono::system_clock> > QCreation;
		std::queue<double> QDuration;

		while (t < Endtime) {
			while(NextCreate <= t){
				QCreation.push(NextCreate);
				QDuration.push(ht_distribution(generator));
				NextCreate += std::chrono::nanoseconds((long long int)iat_distribution(generator));
			}

			while(NextFlowFree <= t && ! QCreation.empty()){
				auto ct = QCreation.front(); QCreation.pop();
				auto ht = QDuration.front(); QDuration.pop();

				if(NextFlowFree < ct){
					NextFlowFree = ct;
				}
				NextFlowFree += std::chrono::nanoseconds((long long int)ht);

				int sdu_size = (int)(RateBpns * ht);

				if (sdu_size < 0 || sdu_size < sizeof(dataSDU)) {
					sdu_size = sizeof(dataSDU);
				} else if (sdu_size > BUFF_SIZE) {
					sdu_size = BUFF_SIZE;
				}

				if (SendData(sdu_size) != sdu_size) return -1;
			}

			if(QCreation.empty()) {
				t = NextCreate;
			} else {
				t = NextFlowFree;
			}

			if(t > Endtime){
				break;
			}
			std::this_thread::sleep_until(t);
			t = std::chrono::system_clock::now();
		};

		return 0;
	}
};

class OnOffClient : public TestClientBase {
public:
	OnOffClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance,
		    const std::string DIF, int TestDuration, unsigned int _packet_size, const unsigned long long _ratebps,
		    const int _avg_ms_on, const int _avg_ms_off, bool verbose, size_t max_sdu_size, unsigned int loss, unsigned int delay,
			int FlowIdent, int QoSIdent) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF, TestDuration,
				verbose, max_sdu_size, loss, delay, FlowIdent, QoSIdent) {

		if (_packet_size < sizeof(dataSDU)) {
			sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			sdu_size = BUFF_SIZE;
		} else {
			sdu_size = _packet_size;
		}

		avg_ms_on = _avg_ms_on > 1 ? _avg_ms_on : 1;
		avg_ms_off = _avg_ms_off > 1 ? _avg_ms_off : 1;

		long long interval_ns;
		interval_ns = 1000000000L;		// ns/s
		interval_ns *= sdu_size * 8L;	// b/B
		interval_ns /= _ratebps;			// b/s
		interval = std::chrono::nanoseconds(interval_ns);
	}

protected:
	int sdu_size;
	int avg_ms_on, avg_ms_off;
	std::chrono::nanoseconds interval;

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t =
			std::chrono::system_clock::now();

		std::default_random_engine generator(t.time_since_epoch().count());
		std::exponential_distribution<double> on_distribution(1.0 / avg_ms_on);
		std::exponential_distribution<double> off_distribution(1.0 / avg_ms_off);

		bool startOff = (rand() % (avg_ms_on + avg_ms_off) <= avg_ms_off);
		long int  phase_duration;
		if (startOff) {
			phase_duration = (long int)off_distribution(generator);
			t += std::chrono::milliseconds(phase_duration);
			if (t > Endtime) t = Endtime;
			std::this_thread::sleep_until(t);
		}

		while (t < Endtime) {
			phase_duration = (long int)on_distribution(generator);
			auto change = t + std::chrono::milliseconds(phase_duration);
			if (change > Endtime) change = Endtime;

			long int sent = 0;
			while (std::chrono::system_clock::now() < change ) {
				if (SendData(sdu_size) != sdu_size) return -1;
				sent++;
				t += interval;
				if (t > Endtime) t = Endtime;
				std::this_thread::sleep_until(t);
			}

			t = std::chrono::system_clock::now();
			if (change != Endtime) {
				phase_duration = (long int)off_distribution(generator);
				t += std::chrono::milliseconds(phase_duration);
				if (t > Endtime) t = Endtime;

				std::this_thread::sleep_until(t);
			}
		};
		return 0;
	}
};

class VideoClient : public TestClientBase {
public:
	VideoClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance,
			const std::string DIF, int TestDuration, unsigned int _packet_size,
			const unsigned long long _ratebps, bool verbose, size_t max_sdu_size, unsigned int loss, unsigned int delay,
			int FlowIdent, int QoSIdent) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF, TestDuration, verbose, max_sdu_size, loss, delay,
				FlowIdent, QoSIdent) {

		if (_packet_size < sizeof(dataSDU)) {
			sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			sdu_size = BUFF_SIZE;
		} else {
			sdu_size = _packet_size;
		}

		long long interval_ns;
		interval_ns = 1000000000L;		// ns/s
		interval_ns *= sdu_size * 8L;	// b/B
		interval_ns /= _ratebps;			// b/s
		interval = std::chrono::nanoseconds(interval_ns);
	}

protected:
	int sdu_size;
	std::chrono::nanoseconds interval;

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t =
			std::chrono::system_clock::now();

		while (std::chrono::system_clock::now() < Endtime) {
			if (SendData(sdu_size) != sdu_size) return -1;
			t += interval;
			if (t > Endtime) t = Endtime;
			std::this_thread::sleep_until(t);
		};

		return 0;
	}
};

class VoiceClient : public TestClientBase {
public:
	VoiceClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance,
		    const std::string DIF, int TestDuration, float HZ, bool verbose, size_t max_sdu_size,
		    unsigned int loss, unsigned int delay, int FlowIdent, int QoSIdent) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			TestDuration, verbose, max_sdu_size, loss, delay, FlowIdent, QoSIdent) {

		setON(320, 3000, 1000);
		setOFF(20, 6000, 2000);

		long long interval_ns;
		interval_ns = 1000000000L;		// ns/s
		interval_ns /= HZ;				// 1/s
		interval = std::chrono::nanoseconds(interval_ns);
	}

	void setON(unsigned int _packet_size, int min_ms, int var_ms) {
		if (_packet_size < sizeof(dataSDU)) {
			on_sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			on_sdu_size = BUFF_SIZE;
		} else {
			on_sdu_size = _packet_size;
		}

		if (min_ms <= 0) {
			min_ms = 1;
		}
		if (var_ms < 0) {
			var_ms = 0;
		}

		min_ms_on = min_ms;
		var_ms_on = var_ms;
	}

	void setOFF(unsigned int _packet_size, int min_ms, int var_ms) {
		if (_packet_size == 0) {
			off_sdu_size = 0;
		} else if (_packet_size < sizeof(dataSDU)) {
			off_sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			off_sdu_size = BUFF_SIZE;
		} else {
			off_sdu_size = _packet_size;
		}

		if (min_ms <= 0) {
			min_ms = 1;
		}
		if (var_ms < 0) {
			var_ms = 0;
		}

		min_ms_off = min_ms;
		var_ms_off = var_ms;
	}

protected:
	int on_sdu_size, off_sdu_size;
	int min_ms_on, var_ms_on;
	int min_ms_off, var_ms_off;
	std::chrono::nanoseconds interval;

	int RunFlow() {
		unsigned int sdu_size;
		std::chrono::time_point<std::chrono::system_clock> t =
			std::chrono::system_clock::now();
		auto change = t;

		bool isOn = (rand() % (min_ms_on + min_ms_off) <= min_ms_on);

		if (isOn) {
			sdu_size = on_sdu_size;
			change += std::chrono::milliseconds(rand() % min_ms_on);
		} else {
			sdu_size = off_sdu_size;
			change += std::chrono::milliseconds(rand() % min_ms_off);
		}

		while (std::chrono::system_clock::now() < Endtime) {
			if (t > change) {
				if (isOn) {
					isOn = false;
					sdu_size = off_sdu_size;
					change = t + std::chrono::milliseconds(min_ms_off + rand() % var_ms_off);
				} else {
					isOn = true;
					sdu_size = on_sdu_size;
					change = t + std::chrono::milliseconds(min_ms_on + rand() % var_ms_on);
				}
			}
			if (sdu_size > 0) {
				if (SendData(sdu_size) != sdu_size) return -1;
			}
			t += interval;
			std::this_thread::sleep_until(t);
		};

		return 0;
	}
};

int main(int argc, char ** argv) {
	std::string Name, Instance;
	std::string ServerName, ServerInstance;
	std::string DIF;
	int FlowIdent, QoSIdent, TestDuration;
	unsigned int PacketSize;
	unsigned long long RateBPS;
	int AVG_ms_ON, AVG_ms_OFF;
	int VAR_ms_ON, PacketSizeOff, VAR_ms_OFF;
	float Hz;
	double Load;
	bool verb;
	int max_ssize;
	unsigned int loss, delay;
	std::string type;

	try {
		TCLAP::CmdLine cmd("TgClient", ' ', "2.0");

		//Base params
		TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = Exponential", false, "Exponential", "string");
		TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::ValueArg<std::string> sName_a("m", "sname", "Server process name, default = TgServer", false, "TgServer", "string");
		TCLAP::ValueArg<std::string> sInstance_a("j", "sinstance", "Server process instance, default = 1",false, "1", "string");
		TCLAP::ValueArg<std::string> DIF_a("D", "dif", "DIF to use, empty for any DIF, default = \"\"",false, "", "string");
		TCLAP::ValueArg<std::string> Type_a("t", "type", "client type (options=data,exp,video,voice,poisson,onoff), default = data",false, "data", "string");
		TCLAP::ValueArg<int> TestDuration_a("d", "duration", "Test duration in s, default = 10",false, 10, "int");
		TCLAP::ValueArg<unsigned int> PacketSize_a("S", "pSize", "Packet size in B, default 1000B", false, 1000, "unsigned int");
		TCLAP::ValueArg<float> RateMBPS_a("M", "Mbps", "Average Rate in Mbps, default = 10.0",false, 10.0f, "float");
		TCLAP::SwitchArg verbose("v", "verbose", "display more debug output, default = false",false);
		TCLAP::ValueArg<int> max_sdu_size("s", "max_sdu_size", "max sdu size, default = 1400", false, 1400, "int");
		TCLAP::ValueArg<unsigned int> delay_a("E", "delay", "Max delay in microseconds, deafult = 0", false, 0, "unsigned int");
		TCLAP::ValueArg<unsigned int> loss_a("L", "loss", "Max loss in NUM/10000, deafult = 10000", false, 10000, "unsigned int");
		TCLAP::ValueArg<int> AVG_ms_ON_a("O", "dOn", "Average duration of On interval in ms, default 1000 ms", false, 1000, "int");
		TCLAP::ValueArg<int> AVG_ms_OFF_a("F", "dOff", "Average duration of Off interval in ms, default 1000 ms",false, 1000, "int");
		TCLAP::ValueArg<double> Load_a("l", "load", "Average flow load, default 1.0", false, 1.0, "double");
		TCLAP::ValueArg<int> VAR_ms_ON_a("U", "vOn", "Variation of duration of On interval in ms, default 1000 ms", false, 1000, "int");
		TCLAP::ValueArg<int> PacketSizeOff_a("u", "fSize", "Packet size in B when OFF, default 1000B", false, 20, "unsigned int");
		TCLAP::ValueArg<int> VAR_ms_OFF_a("f", "vOff", "Variation of duration of Off interval in ms, default 1000 ms", false, 6000, "int");
		TCLAP::ValueArg<float> HZ_a("z", "hz", "Frame Rate flow, default = 50.0Hz", false, 50.0f, "float");
        TCLAP::ValueArg<int> FlowIdent_a("I", "flowid", "Unique flow identifier, default = 0",false, 0, "int");
        TCLAP::ValueArg<int> QoSIdent_a("q", "qosid", "QoS identifier, default = 0",false, 0, "int");


		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(sName_a);
		cmd.add(sInstance_a);
		cmd.add(TestDuration_a);
		cmd.add(PacketSize_a);
		cmd.add(RateMBPS_a);
		cmd.add(DIF_a);
		cmd.add(verbose);
		cmd.add(max_sdu_size);
		cmd.add(delay_a);
		cmd.add(loss_a);
		cmd.add(Type_a);
		cmd.add(AVG_ms_ON_a);
		cmd.add(AVG_ms_OFF_a);
		cmd.add(Load_a);
		cmd.add(HZ_a);
		cmd.add(VAR_ms_ON_a);
		cmd.add(PacketSizeOff_a);
		cmd.add(VAR_ms_OFF_a);
		cmd.add(FlowIdent_a);
		cmd.add(QoSIdent_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		ServerName = sName_a.getValue();
		ServerInstance = sInstance_a.getValue();
		DIF = DIF_a.getValue();
		TestDuration = TestDuration_a.getValue();
		PacketSize = PacketSize_a.getValue();
		float RateMBPS = RateMBPS_a.getValue();
		AVG_ms_ON = AVG_ms_ON_a.getValue();
		AVG_ms_OFF = AVG_ms_OFF_a.getValue();
		Load = Load_a.getValue();
		RateBPS = 1000000; // 1Mbps
		RateBPS *= RateMBPS;
		verb = verbose.getValue();
		max_ssize = max_sdu_size.getValue();
		loss = loss_a.getValue();
		delay = delay_a.getValue();
		type = Type_a.getValue();
		Hz = HZ_a.getValue();
		VAR_ms_ON = VAR_ms_ON_a.getValue();
		PacketSizeOff = PacketSizeOff_a.getValue();
		VAR_ms_OFF = VAR_ms_OFF_a.getValue();
		FlowIdent = FlowIdent_a.getValue();
		QoSIdent = QoSIdent_a.getValue();
	}
	catch (TCLAP::ArgException &e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}

	if (type == "data") {
		DataClient App(Name, Instance, ServerName, ServerInstance, DIF,
			TestDuration, PacketSize, RateBPS, verb, max_ssize, loss, delay, FlowIdent, QoSIdent);
		return App.Run();
	}

	if (type == "exp") {
		Exponential App(Name, Instance, ServerName, ServerInstance, DIF,
			TestDuration, PacketSize, RateBPS,
			verb, max_ssize, loss, delay, FlowIdent, QoSIdent);
		return App.Run();
	}

	if (type == "video") {
		VideoClient App(Name, Instance, ServerName, ServerInstance, DIF,
				TestDuration, PacketSize, RateBPS, verb, max_ssize,
				loss, delay, FlowIdent, QoSIdent);
			return App.Run();
	}

	if (type == "poisson") {
		PoissonClient App(Name, Instance, ServerName, ServerInstance, DIF,
				TestDuration, PacketSize, RateBPS, Load, verb, max_ssize,
				loss, delay, FlowIdent, QoSIdent);
		return App.Run();
	}

	if (type == "voice") {
		VoiceClient App(Name, Instance, ServerName, ServerInstance, DIF,
			        TestDuration, Hz, verb,
				max_ssize, loss, delay, FlowIdent, QoSIdent);
		App.setON(PacketSize, AVG_ms_ON, VAR_ms_ON);
		App.setOFF(PacketSizeOff, AVG_ms_OFF, VAR_ms_OFF);
		return App.Run();
	}

	if (type == "onoff") {
		OnOffClient App(Name, Instance, ServerName, ServerInstance, DIF,
				TestDuration, PacketSize, RateBPS, AVG_ms_ON, AVG_ms_OFF,
				verb, max_ssize, loss, delay, FlowIdent, QoSIdent);
		return App.Run();
	}

	std::cerr << "Unknown client type: " << type << std::endl;
	return -1;
}
