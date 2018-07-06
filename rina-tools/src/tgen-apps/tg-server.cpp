#include <chrono>
#include <map>
#include <sstream>
#include <tclap/CmdLine.h>

#include "ra_base_server.h"
#include "test_commons.h"

struct flow_log {
	flow_log() {
		count = 0;
		data = 0;
		maxLat = 0;
		minLat = 9999999999;
		latCount = 0;
		seq_id = 0;
		flowId = 0;
		QoSId = 0;
	}

	void process(long long t, dataSDU * sdu) {
		//seq_id = sdu->SeqId;
		count++;
		data += sdu->Size;
		long long lat = t - sdu->SendTime;
		if (lat > maxLat) maxLat = lat;
		if (lat < minLat) minLat = lat;
		latCount += lat;
	}

	int QoSId;
	int flowId;

	int seq_id;
	long long count, data;

	long long maxLat;
	long long minLat;
	long double latCount;
};

struct qos_log {
	qos_log() {
		count = 0;
		data = 0;
		total = 0;
		maxLat = 0;
		minLat = 9999999999;
		latCount = 0;
	}
	void process(flow_log * f) {
		count += f->count;
		total += f->seq_id;
		data += f->data;
		if (f->maxLat > maxLat) maxLat = f->maxLat;
		if (f->minLat < minLat) minLat = f->minLat;
		latCount += f->latCount;
	}

	long long count, total, data;

	long long maxLat;
	long long minLat;
	long double latCount;
};

class TgServer : public ra::BaseServer {
public:
	TgServer(const std::string& Name, const std::string& Instance,
		  bool verbose, const std::string& type) :
		BaseServer(Name, Instance, verbose) {
		RegisterTimeoutMs = 1000;
		ListenTimeoutMs = -1;
		Count = 0;
		server_type = type;
	}

	virtual ~TgServer(){};

protected:

	int HandleFlow(const int Fd) {
		if (server_type == "log") {
			return HandleFlow_log(Fd);
		}

		if (server_type == "drop") {
			return HandleFlow_drop(Fd);
		}

		if (server_type == "dump") {
			return HandleFlow_dump(Fd);
		}

		return -1;
	}

private:
	std::string server_type;
	int Count;
	std::vector<flow_log*> flow_logs;
	std::map<int, qos_log> qos_logs;
	std::mutex Mt;

	void logger_t() {
		int fCount;
		std::chrono::seconds s1(1);

		do {
			std::this_thread::sleep_for(s1);
			Mt.lock();
			fCount = Count;
			Mt.unlock();
			if (verbose)
				std::cout << "Flows in progress " << fCount << std::endl;
		} while (fCount > 0);

		Mt.lock();

		if (verbose)
			std::cout << "Currently no flows in progress, print log:" << std::endl << std::endl;

		//Process log
		long long tCount = 0, tData = 0;

		for (auto f : flow_logs) {
			tCount += f->count;
			tData += f->data;
			qos_logs[f->QoSId].process(f);
		}

		std::cout << "Total :: " << tCount << " Packets | " << (tData / 125000.0) << " Mb" << std::endl;
		std::cout << "# Statistics per flow:" <<std::endl;
		std::cout << "#\tFlowID (QoSID) | Rcv Packets | Snd Packets | % Lost Packets | Rcv Bytes | Min Latency | Avg Latency | Max Latency" << std::endl;
		for (flow_log * f : flow_logs) {
			long long c = f->count;
			long long t = f->seq_id;
			long double l = t - c;

			std::cout << "\t" << f->flowId << " (" << (int)f->QoSId << ") | "
				<< c << " | " << t << " | " << (l*100.0 / t) << " % | " << f->data << " B"
				<< " |" << (f->minLat / 1000000.0)
				<< " | " << (f->latCount / (1000000.0*c))
				<< " | " << (f->maxLat / 1000000.0)
				<< std::endl;
		}

		std::cout << "# Statistics per QoS ID:" <<std::endl;
		std::cout << "#\t(QoSID) | Rcv Packets | Snd Packets | % Lost Packets | Min Latency | Avg Latency | Max Latency" << std::endl;
		for (auto qd : qos_logs) {
			int QoSId = qd.first;
			qos_log & q = qd.second;

			long long l = q.total - q.count;

			std::cout << "\t(" << QoSId << ") | "
				<< q.count << " | " << q.total << "  | " << (100.0*l / q.total) << " % | "
				<< (q.minLat / 1000000.0) << "  | " << (q.latCount / (1000000 * q.count))<< "  | " << (q.maxLat / 1000000.0)
				<< std::endl;

		}

		for (flow_log * f : flow_logs) {
			delete f;
		}

		Count = 0;
		flow_logs.clear();
		qos_logs.clear();
		Mt.unlock();
	}

	flow_log * process_first_sdu(const int Fd) {
		union {
			ra::byte_t Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};

		if (ra::ReadDataTimeout(Fd, Buffer, TIMEOUT_MS) <= 0) {
			std::cerr << "No data received during the first second of lifetime" << std::endl;
			return NULL;
		}

		if ((int)InitData.Flags & SDU_FLAG_INIT == 0) {
			std::cerr << "First received packet not with INIT flag" << std::endl;
			return NULL;
		}

		if (verbose)
			std::cout << "Started Flow " << Fd << std::endl;

		if (write(Fd, Buffer, InitData.Size) != (int)InitData.Size) {
			std::cerr << "First packet ECHO failed" << std::endl;
			return NULL;
		}

		flow_log * Flow = new flow_log();
		Flow->QoSId = InitData.QoSId;
		Flow->flowId = InitData.FlowId;

		return (Flow);
	}

	int HandleFlow_log(const int Fd) {
		int ReadSize;
		bool start_logger;
		union {
			ra::byte_t Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};

		flow_log * Flow = process_first_sdu(Fd);
		if (Flow == NULL)
			return -1;

		Mt.lock();
		start_logger = (Count <= 0);
		Count++;
		Mt.unlock();

		if (start_logger) {
			std::thread t(&TgServer::logger_t, this);
			t.detach();
		}

		for (;;) {
			ReadSize = ra::ReadData(Fd, Buffer);

			Mt.lock();

			if (ReadSize <= 0) {
				Count--;
				std::cout << "Not received last SDU. Discarding flow" << std::endl;
				Mt.unlock();
				return -1;
			}

			if ((int)Data.Flags & SDU_FLAG_FIN) {
				Count--;
				flow_logs.push_back(Flow);
				Flow->seq_id = Data.SeqId - 1;
				Mt.unlock();
				break;
			}

			Flow->process(std::chrono::duration_cast<std::chrono::microseconds>(
							std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
							&Data);
			Mt.unlock();

		}

		if (write(Fd, Buffer, Data.Size) != (int) Data.Size) {
			std::cerr << "Last packet ECHO failed" << std::endl;
			return -1;
		}
		return 0;
	}

	int HandleFlow_drop(const int Fd) {
		int ReadSize;
		union {
			ra::byte_t Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};

		flow_log * Flow = process_first_sdu(Fd);
		if (Flow == NULL)
			return -1;

		for (;;) {
			ReadSize = ra::ReadData(Fd, Buffer);
			if (ReadSize <= 0) {
				return -1;
			}
			if ((int)Data.Flags & SDU_FLAG_FIN) {
				if (verbose)
					std::cout << "Ended Flow " << Fd << std::endl;
				break;
			}
		}

		if (write(Fd, Buffer, Data.Size) != (int) Data.Size) {
			std::cerr << "Last packet ECHO failed" << std::endl;
			return -1;
		}

		return 0;
	}

	int HandleFlow_dump(const int Fd) {
		int ReadSize;
		union {
			ra::byte_t Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};
		long long latency;
		bool start_logger;
		flow_log * Flow = process_first_sdu(Fd);
		if (Flow == NULL)
			return -1;


		for (;;) {
			if(ra::ReadData(Fd, Buffer) <= 0) {
				return -1;
			}

			auto now = std::chrono::high_resolution_clock::now();
			long long current_time = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

			latency = (long long)(current_time - (Data.SendTime));

			if ((int)Data.Flags & SDU_FLAG_FIN) {
				Mt.lock();
				std::stringstream stream;
				stream << Flow->flowId << " |  " << Data.SeqId  << " | " << Data.Size
								  << "B | " << latency <<" | END"<< std::endl;
				std::cout << stream.str()<<std::flush;
				Mt.unlock();
				break;
			}

			Mt.lock();
			std::stringstream stream;
			stream << Flow->flowId << " |  " << Data.SeqId  << " | " << Data.Size
					<< "B | " << latency << std::endl;
			std::cout << stream.str()<<std::flush;
			Mt.unlock();
		}

		if (write(Fd, Buffer, Data.Size) != (int) Data.Size) {
			std::cerr << "Last packet ECHO failed" << std::endl;
			return -1;
		}

		return 0;
	}
};


int main(int argc, char ** argv) {
	std::string Name, Instance, type;
	std::vector<std::string> DIFs;
	bool verb;

	try {
		TCLAP::CmdLine cmd("TgServer", ' ', "2.0");

		TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = TgServer", false, "TgServer", "string");
		TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::ValueArg<std::string> type_a("t","type","Server type (options = log, dump, drop), default = log", false, "log", "string");
		TCLAP::UnlabeledMultiArg<std::string> DIFs_a("difs","DIFs to use, empty for any DIF", false, "string");
		TCLAP::SwitchArg verbose("v", "verbose", "display more debug output, default = false",false);

		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(DIFs_a);
		cmd.add(verbose);
		cmd.add(type_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		DIFs = DIFs_a.getValue();
		verb = verbose.getValue();
		type = type_a.getValue();
	}
	catch (TCLAP::ArgException &e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}

	TgServer App(Name, Instance, verb, type);
	for (auto DIF : DIFs) {
		App.AddToDIF(DIF);
	}

	return App.Run();
}
